#include "filter.hpp"

#include <cpm/test/command_line.hpp>

#include <format>
#include <iostream>
#include <utility>

namespace cpm::test {
namespace {

constexpr std::string_view usage_text =
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

// The test cases `options` choose. Patterns that choose none are more likely a typo than a wish to run nothing.
std::vector<const TestCase*> choose(const Registry& registry, const Options& options) {
    std::vector<const TestCase*> chosen = detail::Filter(options).select(registry);
    if (chosen.empty() && !options.include.empty()) {
        std::string patterns;
        for (const std::string& pattern : options.include) {
            patterns += std::format("{}'{}'", patterns.empty() ? "" : ", ", pattern);
        }
        throw UsageError(std::format("no test case matches {}", patterns), "--list shows their names");
    }
    return chosen;
}

}  // namespace

UsageError::UsageError(const std::string& message, std::string hint)
    : std::runtime_error(message), hint_(std::move(hint)) {}

CommandLine parse_command_line(const std::vector<std::string>& args) {
    CommandLine command;
    bool list = false;
    bool help = false;
    for (const std::string& arg : args) {
        if (arg == "-h" || arg == "--help") {
            help = true;
        } else if (arg == "-l" || arg == "--list") {
            list = true;
        } else if (arg == "-v" || arg == "--verbose") {
            command.verbose = true;
        } else if (arg == "--fail-fast") {
            command.options.fail_fast = true;
        } else if (arg.size() > 1 && arg.starts_with('-')) {
            throw UsageError(std::format("unknown option '{}'", arg), "--help lists the options");
        } else if (arg.starts_with('~')) {
            command.options.exclude.push_back(arg.substr(1));
        } else {
            command.options.include.push_back(arg);
        }
    }
    command.action = help ? CommandLine::Action::help : list ? CommandLine::Action::list : CommandLine::Action::run;
    return command;
}

std::string_view usage() noexcept { return usage_text; }

int main(const Registry& registry, const std::vector<std::string>& args, std::ostream& out) {
    try {
        const CommandLine command = parse_command_line(args);
        if (command.action == CommandLine::Action::help) {
            out << usage();
            return 0;
        }
        const std::vector<const TestCase*> chosen = choose(registry, command.options);
        if (command.action == CommandLine::Action::list) {
            for (const TestCase* test : chosen) out << test->name << '\n';
            return 0;
        }
        ConsoleReporter reporter(out, command.verbose);
        return run(registry, command.options, reporter).exit_code();
    } catch (const UsageError& error) {
        out << std::format("error: {}\n  hint: {}\n", error.what(), error.hint());
        return 2;
    }
}

int main(int argc, char* argv[]) {
    std::vector<std::string> args;
    if (argc > 1) args.assign(argv + 1, argv + argc);
    return main(registry(), args, std::cout);
}

}  // namespace cpm::test
