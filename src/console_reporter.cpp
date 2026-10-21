#include <cpm/test/reporter.hpp>

#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::test {
namespace {

// "test case" > "subcase" > "nested subcase"
std::string describe_path(const std::vector<std::string>& path) {
    std::string out;
    for (const std::string& name : path) out += std::format("{}\"{}\"", out.empty() ? "" : " > ", name);
    return out;
}

std::string_view describe(Outcome outcome) {
    switch (outcome) {
    case Outcome::passed: return "passed";
    case Outcome::failed: return "failed";
    case Outcome::skipped: return "skipped";
    }
    return "?";
}

std::string count(long long n, std::string_view noun) { return std::format("{} {}{}", n, noun, n == 1 ? "" : "s"); }

// "test cases: 3 passed, 1 failed; checks: 20 passed, 2 failed", leaving out counts of zero after "passed".
std::string summary_line(const Summary& summary) {
    std::string cases = std::format("{} passed", summary.passed);
    if (summary.failed > 0) cases += std::format(", {} failed", summary.failed);
    if (summary.skipped > 0) cases += std::format(", {} skipped", summary.skipped);
    std::string checks = std::format("{} passed", summary.checks_passed);
    if (summary.checks_failed > 0) checks += std::format(", {} failed", summary.checks_failed);
    return std::format("test cases: {}; checks: {}", cases, checks);
}

}  // namespace

ConsoleReporter::ConsoleReporter(std::ostream& out, bool verbose) noexcept : out_(out), verbose_(verbose) {}

void ConsoleReporter::failure(const Failure& failure) {
    out_ << std::format("{}:{}: error: {}\n", failure.where.file, failure.where.line, failure.message);
    if (!failure.detail.empty()) out_ << "  " << failure.detail << '\n';
    if (!failure.path.empty()) out_ << "  in: " << describe_path(failure.path) << '\n';
    for (const std::string& message : failure.info) out_ << "  info: " << message << '\n';
}

void ConsoleReporter::skipped(const Skip& skip) {
    out_ << std::format("{}:{}: note: skipped: {}\n", skip.where.file, skip.where.line, skip.reason);
    if (!skip.path.empty()) out_ << "  in: " << describe_path(skip.path) << '\n';
}

void ConsoleReporter::test_case_finished(const TestCaseResult& result) {
    if (!verbose_) return;
    out_ << std::format("{}: {} ({})\n", describe(result.outcome), result.name, count(result.checks, "check"));
}

void ConsoleReporter::run_finished(const Summary& summary) {
    out_ << summary_line(summary) << '\n';
    out_.flush();
}

}  // namespace cpm::test
