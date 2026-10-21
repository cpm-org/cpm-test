// Running one test case.
#pragma once

#include "context.hpp"
#include "subcase_tracker.hpp"

#include <cpm/test/registry.hpp>
#include <cpm/test/reporter.hpp>
#include <cpm/test/results.hpp>

#include <atomic>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::test::detail {

// Checks counted across test cases and threads.
struct CheckCounter {
    std::atomic<long long> passed{0};
    std::atomic<long long> failed{0};

    long long total() const noexcept { return passed + failed; }
};

// Runs a test case pass after pass until its subcases are done, as the context its code reports to. Passes the
// reporter one call at a time, though checks may come from several threads.
class TestCaseExecution final : public Context {
public:
    TestCaseExecution(const TestCase& test, Reporter& reporter, CheckCounter& checks) noexcept;

    Outcome run();

    void check_passed() override;
    void check_failed(Location where, std::string message, std::string detail) override;
    void skipped(Location where, std::string reason) override;
    bool enter_subcase(std::string_view name, Location where) override;
    void leave_subcase(bool unwinding) override;

private:
    enum class PassEnd { normally, by_exception, by_skip };

    PassEnd run_pass();
    void forget_caught_exceptions() noexcept;
    void report_escaped_exception();
    void report_too_many_passes();
    void record(const Failure& failure);  // with mutex_ held
    std::vector<std::string> path_to(const std::vector<std::string>& subcases) const;

    // Every pass that enters a subcase finishes one, so this is only reached when subcase names or their order change
    // from pass to pass.
    static constexpr int max_passes = 100'000;

    const TestCase& test_;
    Reporter& reporter_;
    CheckCounter& checks_;
    std::mutex mutex_;  // guards reporter_, subcases_ and failed_
    SubcaseTracker subcases_;
    bool failed_ = false;
};

}  // namespace cpm::test::detail
