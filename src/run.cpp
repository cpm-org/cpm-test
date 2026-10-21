#include "filter.hpp"
#include "test_case_execution.hpp"

#include <cpm/test/run.hpp>

namespace cpm::test {

Summary run(const Registry& registry, const Options& options, Reporter& reporter) {
    detail::CheckCounter checks;
    Summary summary;
    for (const TestCase* test : detail::Filter(options).select(registry)) {
        const long long before = checks.total();
        detail::TestCaseExecution execution(*test, reporter, checks);
        const Outcome outcome = execution.run();
        summary.add(outcome);
        reporter.test_case_finished(TestCaseResult{test->name, outcome, checks.total() - before});
        if (outcome == Outcome::failed && options.fail_fast) break;
    }
    summary.checks_passed = checks.passed;
    summary.checks_failed = checks.failed;
    reporter.run_finished(summary);
    return summary;
}

Summary run(const Registry& registry, const Options& options, std::ostream& out) {
    ConsoleReporter reporter(out);
    return run(registry, options, reporter);
}

}  // namespace cpm::test
