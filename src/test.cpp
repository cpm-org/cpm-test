#include <cpm/test.hpp>

#include <algorithm>
#include <atomic>
#include <iostream>
#include <iterator>
#include <mutex>
#include <set>

namespace cpm::test {
namespace {

// A subcase on the current path, identified across passes by where it is and its name.
struct Frame {
    std::string name;
    std::string file;
    int line = 0;
};

// Each pass enters at least one subcase it hasn't finished, so passes run out unless subcase names or order change
// between passes.
constexpr int max_passes = 100'000;

struct Run {
    explicit Run(std::ostream& output) : out(output) {}

    std::ostream& out;
    std::mutex mutex;  // guards the output and everything below except the counters
    std::atomic<long long> checks_passed{0};
    std::atomic<long long> checks_failed{0};

    const TestCase* test = nullptr;  // the test case running now
    bool failed = false;             // whether it has failed

    // A pass runs the test case once and enters at most one unfinished subcase at each depth.
    std::vector<Frame> path;           // the subcases entered and not yet left in this pass
    std::vector<char> entered_child;   // [depth]: a subcase at this depth was entered in this pass
    std::vector<char> incomplete;      // [depth]: a subcase at this depth still has to run
    std::set<std::string> finished;    // keys of the subcase paths that have run completely
    bool entered_any = false;          // whether this pass entered a subcase
    std::vector<Frame> exception_path; // where the exception ending this pass was thrown
    bool exception_path_set = false;
};

std::atomic<Run*> current_run{nullptr};

// INFO messages in scope on this thread, outermost first.
thread_local std::vector<const detail::InfoEntry*> infos;
// Messages of the INFOs an exception left while unwinding, innermost first, for when it escapes the test case.
thread_local std::vector<std::string> unwound_infos;

std::string key(const std::vector<Frame>& path) {
    std::string out;
    for (const Frame& frame : path) out += std::format("{}:{}:{}\x1f", frame.file, frame.line, frame.name);
    return out;
}

std::string describe_path(const TestCase& test, const std::vector<Frame>& path) {
    std::string out = std::format("\"{}\"", test.name);
    for (const Frame& frame : path) out += std::format(" > \"{}\"", frame.name);
    return out;
}

std::string info_message(const detail::InfoEntry* entry) noexcept {
    try {
        return entry->message(entry->context);
    } catch (...) {
        return "(this INFO threw while being formatted)";
    }
}

std::vector<std::string> current_infos() {
    std::vector<std::string> out;
    out.reserve(infos.size());
    for (const detail::InfoEntry* entry : infos) out.push_back(info_message(entry));
    return out;
}

std::string describe_exception(const std::optional<std::string>& message) {
    return message ? detail::quote(*message) : std::string("(unknown type)");
}

// One failure: where and what, a detail line, the test case and subcase path, then the INFO messages. The caller
// holds run.mutex.
void write_failure(Run& run, std::string_view location, std::string_view what, std::string_view detail,
                   const std::vector<Frame>& path, const std::vector<std::string>& messages) {
    run.out << location << ": error: " << what << '\n';
    if (!detail.empty()) run.out << "  " << detail << '\n';
    if (run.test != nullptr) run.out << "  in: " << describe_path(*run.test, path) << '\n';
    for (const std::string& message : messages) run.out << "  info: " << message << '\n';
    run.failed = true;
}

void report_failed_check(const char* file, int line, std::string_view what, std::string_view detail) {
    // INFO messages are user code, so they are formatted before taking the lock.
    const std::vector<std::string> messages = current_infos();
    Run* const run = current_run.load();
    if (run == nullptr) {
        std::cerr << std::format("{}:{}: error: {} (outside of a test run)\n", file, line, what);
        return;
    }
    run->checks_failed.fetch_add(1, std::memory_order_relaxed);
    const std::lock_guard lock(run->mutex);
    write_failure(*run, std::format("{}:{}", file, line), what, detail, run->path, messages);
}

void report_exception(Run& run, const TestCase& test) {
    const std::string detail = "exception: " + describe_exception(detail::current_exception_message());
    const std::vector<std::string> messages(unwound_infos.rbegin(), unwound_infos.rend());
    const std::lock_guard lock(run.mutex);
    write_failure(run, std::format("{}:{}", test.file, test.line), "unexpected exception", detail,
                  run.exception_path, messages);
}

enum class Outcome { passed, failed, skipped };

Outcome run_case(Run& run, const TestCase& test) {
    {
        const std::lock_guard lock(run.mutex);
        run.test = &test;
        run.failed = false;
        run.finished.clear();
    }
    bool skipped = false;
    for (int pass = 1;; ++pass) {
        {
            const std::lock_guard lock(run.mutex);
            run.path.clear();
            run.entered_child.assign(1, 0);
            run.incomplete.assign(1, 0);
            run.entered_any = false;
            run.exception_path.clear();
            run.exception_path_set = false;
        }
        unwound_infos.clear();
        bool threw = false;
        try {
            test.body();
        } catch (const detail::Stop& stop) {
            threw = true;
            skipped = stop.skipped;
        } catch (...) {
            threw = true;
            report_exception(run, test);
        }
        unwound_infos.clear();
        if (skipped) break;

        const std::lock_guard lock(run.mutex);
        // An exception may have ended the pass before it reached the remaining subcases, so they get another pass.
        if (run.incomplete[0] == 0 && !(threw && run.entered_any)) break;
        if (pass == max_passes) {
            write_failure(run, std::format("{}:{}", test.file, test.line),
                          std::format("subcases still unfinished after {} passes", max_passes),
                          "hint: subcase names and order must be the same in every pass", {}, {});
            break;
        }
    }
    const std::lock_guard lock(run.mutex);
    return run.failed ? Outcome::failed : skipped ? Outcome::skipped : Outcome::passed;
}

bool matches(std::string_view pattern, std::string_view text) {
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string_view::npos;  // the last '*' seen, to backtrack to
    std::size_t mark = 0;                        // where in text that '*' started matching
    while (t < text.size()) {
        if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            mark = t;
        } else if (p < pattern.size() && pattern[p] == text[t]) {
            ++p;
            ++t;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            t = ++mark;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

bool selected(const std::string& name, const Options& options) {
    const auto any = [&](const std::vector<std::string>& patterns) {
        return std::ranges::any_of(patterns, [&](const std::string& pattern) { return matches(pattern, name); });
    };
    return (options.include.empty() || any(options.include)) && !any(options.exclude);
}

std::string plural(long long count, std::string_view word) {
    return std::format("{} {}{}", count, word, count == 1 ? "" : "s");
}

std::string summary_line(const Summary& summary) {
    std::string cases = std::format("{} passed", summary.passed);
    if (summary.failed > 0) cases += std::format(", {} failed", summary.failed);
    if (summary.skipped > 0) cases += std::format(", {} skipped", summary.skipped);
    std::string checks = std::format("{} passed", summary.checks_passed);
    if (summary.checks_failed > 0) checks += std::format(", {} failed", summary.checks_failed);
    return std::format("test cases: {}; checks: {}", cases, checks);
}

void append_escaped(std::string& out, char c, char quote) {
    switch (c) {
    case '\\': out += "\\\\"; break;
    case '\n': out += "\\n"; break;
    case '\r': out += "\\r"; break;
    case '\t': out += "\\t"; break;
    case '\0': out += "\\0"; break;
    default:
        if (c == quote) {
            out += '\\';
            out += c;
        } else if (static_cast<unsigned char>(c) < 0x20 || c == '\x7f') {
            out += std::format("\\x{:02x}", static_cast<unsigned>(static_cast<unsigned char>(c)));
        } else {
            out += c;
        }
    }
}

constexpr std::string_view usage =
    "usage: TEST [options] [pattern...]\n"
    "\n"
    "Runs the test cases whose names match a pattern, or all of them. In a pattern, * matches any\n"
    "text. A pattern that starts with ~ leaves out the test cases it matches.\n"
    "\n"
    "Options:\n"
    "  -l, --list       List the test cases instead of running them\n"
    "  -v, --verbose    Print a line for every test case, not only for failures\n"
    "      --fail-fast  Stop after the first test case that fails\n"
    "  -h, --help       Show this help\n"
    "\n"
    "Exit status: 0 when every test case passed or was skipped, 1 when one failed, 2 for a bad\n"
    "command line, 77 when none passed (so `cpm test` counts the program as skipped).\n";

}  // namespace

void Registry::add(TestCase test) { cases_.push_back(std::move(test)); }

void Registry::add(std::string name, std::function<void()> body, std::source_location where) {
    add(TestCase{std::move(name), where.file_name(), static_cast<int>(where.line()), std::move(body)});
}

Registry& registry() {
    static Registry instance;
    return instance;
}

Summary run(const Registry& registry, const Options& options, std::ostream& out) {
    Run state(out);
    // Runs nest (a test of test tools can run a registry of its own), so the outer one comes back afterwards.
    struct Restore {
        Run* previous;
        ~Restore() { current_run.store(previous); }
    } const restore{current_run.exchange(&state)};

    Summary summary;
    for (const TestCase& test : registry.cases()) {
        if (!selected(test.name, options)) continue;
        const long long before = state.checks_passed + state.checks_failed;
        const Outcome outcome = run_case(state, test);
        const long long checks = state.checks_passed + state.checks_failed - before;
        switch (outcome) {
        case Outcome::passed: ++summary.passed; break;
        case Outcome::failed: ++summary.failed; break;
        case Outcome::skipped: ++summary.skipped; break;
        }
        if (options.verbose) {
            const std::string_view word =
                outcome == Outcome::passed ? "passed" : outcome == Outcome::failed ? "failed" : "skipped";
            const std::lock_guard lock(state.mutex);
            out << std::format("{}: {} ({})\n", word, test.name, plural(checks, "check"));
        }
        if (outcome == Outcome::failed && options.fail_fast) break;
    }
    {
        const std::lock_guard lock(state.mutex);
        state.test = nullptr;
    }
    summary.checks_passed = state.checks_passed;
    summary.checks_failed = state.checks_failed;
    out << summary_line(summary) << '\n';
    out.flush();
    return summary;
}

int main(const Registry& registry, const std::vector<std::string>& args, std::ostream& out) {
    Options options;
    bool list = false;
    for (const std::string& arg : args) {
        if (arg == "-h" || arg == "--help") {
            out << usage;
            return 0;
        } else if (arg == "-l" || arg == "--list") {
            list = true;
        } else if (arg == "-v" || arg == "--verbose") {
            options.verbose = true;
        } else if (arg == "--fail-fast") {
            options.fail_fast = true;
        } else if (arg.size() > 1 && arg[0] == '-') {
            out << std::format("error: unknown option '{}'\n  hint: --help lists the options\n", arg);
            return 2;
        } else if (arg.starts_with('~')) {
            options.exclude.push_back(arg.substr(1));
        } else {
            options.include.push_back(arg);
        }
    }

    const auto& cases = registry.cases();
    const auto chosen = std::ranges::count_if(cases, [&](const TestCase& test) { return selected(test.name, options); });
    if (chosen == 0 && !options.include.empty()) {
        std::string patterns;
        for (const std::string& pattern : options.include) patterns += (patterns.empty() ? "'" : ", '") + pattern + "'";
        out << std::format("error: no test case matches {}\n  hint: --list shows their names\n", patterns);
        return 2;
    }
    if (list) {
        for (const TestCase& test : cases) {
            if (selected(test.name, options)) out << test.name << '\n';
        }
        return 0;
    }
    return run(registry, options, out).exit_code();
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    if (argc > 1) args.assign(argv + 1, argv + argc);
    return main(registry(), args, std::cout);
}

namespace detail {

std::string quote(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    out += '"';
    for (const char c : text) append_escaped(out, c, '"');
    out += '"';
    return out;
}

std::string quote(char c) {
    std::string out = "'";
    append_escaped(out, c, '\'');
    out += '\'';
    return out;
}

std::optional<std::string> current_exception_message() {
    try {
        throw;
    } catch (const std::exception& e) {
        return std::string(e.what());
    } catch (const std::string& s) {
        return s;
    } catch (const char* s) {
        return std::string(s != nullptr ? s : "");
    } catch (...) {
        return std::nullopt;
    }
}

void Assertion::fail_with_exception(std::string_view prefix) {
    fail(std::string(prefix) + describe_exception(current_exception_message()));
}

void Assertion::check_exception_message(std::string_view expected) {
    const std::optional<std::string> message = current_exception_message();
    if (message && *message == expected) {
        pass();
    } else {
        fail("exception: " + describe_exception(message));
    }
}

void Assertion::finish() {
    // A check that runs normally means any exception thrown earlier was caught, so its INFOs no longer apply.
    if (!unwound_infos.empty() && std::uncaught_exceptions() == 0) unwound_infos.clear();
    if (passed_) {
        if (Run* const run = current_run.load(std::memory_order_relaxed)) {
            run->checks_passed.fetch_add(1, std::memory_order_relaxed);
        }
        return;
    }
    report_failed_check(file_, line_, std::format("{}({}) failed", macro_, expression_), detail_);
    if (kind_ == Kind::require) throw Stop{};
}

bool add_test_case(const char* name, void (*body)(), const char* file, int line) {
    registry().add(TestCase{name, file, line, body});
    return true;
}

Subcase::Subcase(std::string_view name, const char* file, int line)
    : entered_(false), uncaught_(std::uncaught_exceptions()) {
    Run* const run = current_run.load();
    if (run == nullptr) return;
    const std::lock_guard lock(run->mutex);
    if (run->test == nullptr) return;
    const std::size_t depth = run->path.size();
    std::vector<Frame> candidate = run->path;
    candidate.push_back(Frame{std::string(name), file, line});
    if (run->finished.contains(key(candidate))) return;
    if (run->entered_child[depth] != 0) {
        // A sibling ran in this pass; this one waits for the next.
        run->incomplete[depth] = 1;
        return;
    }
    run->entered_child[depth] = 1;
    run->path = std::move(candidate);
    run->entered_child.push_back(0);
    run->incomplete.push_back(0);
    run->entered_any = true;
    entered_ = true;
}

Subcase::~Subcase() {
    if (!entered_) return;
    Run* const run = current_run.load();
    if (run == nullptr) return;
    const bool unwinding = std::uncaught_exceptions() > uncaught_;
    const std::lock_guard lock(run->mutex);
    if (unwinding && !run->exception_path_set) {
        run->exception_path = run->path;
        run->exception_path_set = true;
    }
    const std::size_t depth = run->path.size();  // this subcase's children are at this depth
    // After an exception, a subcase that entered a child may have children the exception kept it from reaching, so
    // it runs again; one that entered none can't get any further.
    const bool done = unwinding ? run->entered_child[depth] == 0 : run->incomplete[depth] == 0;
    if (done) {
        run->finished.insert(key(run->path));
    } else {
        run->incomplete[depth - 1] = 1;
    }
    run->path.pop_back();
    run->entered_child.pop_back();
    run->incomplete.pop_back();
}

void push_info(const InfoEntry* entry) { infos.push_back(entry); }

void pop_info(const InfoEntry* entry, bool unwinding) noexcept {
    if (unwinding) {
        try {
            unwound_infos.push_back(info_message(entry));
        } catch (...) {
        }
    }
    if (!infos.empty() && infos.back() == entry) {
        infos.pop_back();
    } else if (const auto it = std::find(infos.rbegin(), infos.rend(), entry); it != infos.rend()) {
        infos.erase(std::next(it).base());
    }
}

void fail_and_continue(const char* file, int line, std::string message) {
    report_failed_check(file, line, message, {});
}

void fail_and_stop(const char* file, int line, std::string message) {
    fail_and_continue(file, line, std::move(message));
    throw Stop{};
}

void skip(const char* file, int line, std::string reason) {
    if (Run* const run = current_run.load()) {
        const std::lock_guard lock(run->mutex);
        run->out << std::format("{}:{}: note: skipped: {}\n", file, line, reason);
        if (run->test != nullptr) run->out << "  in: " << describe_path(*run->test, run->path) << '\n';
    }
    throw Stop{true};
}

}  // namespace detail
}  // namespace cpm::test
