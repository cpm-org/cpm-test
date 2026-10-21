// A Reporter that keeps every event, for tests that check what a run reports rather than how it is printed.
#pragma once

#include <cpm/test/reporter.hpp>
#include <cpm/test/run.hpp>

#include <sstream>
#include <string>
#include <vector>

class RecordingReporter final : public cpm::test::Reporter {
public:
    std::vector<cpm::test::Failure> failures;
    std::vector<cpm::test::Skip> skips;
    std::vector<cpm::test::TestCaseResult> results;
    cpm::test::Summary summary;
    int runs_finished = 0;

    void failure(const cpm::test::Failure& failure) override { failures.push_back(failure); }
    void skipped(const cpm::test::Skip& skip) override { skips.push_back(skip); }
    void test_case_finished(const cpm::test::TestCaseResult& result) override { results.push_back(result); }
    void run_finished(const cpm::test::Summary& run_summary) override {
        summary = run_summary;
        ++runs_finished;
    }

    // What a ConsoleReporter would have printed for the failures and skips, to show when a test of them fails.
    std::string text() const {
        std::ostringstream out;
        cpm::test::ConsoleReporter console(out);
        for (const cpm::test::Failure& failure : failures) console.failure(failure);
        for (const cpm::test::Skip& skip : skips) console.skipped(skip);
        return std::move(out).str();
    }
};

// Runs `registry` as a run of its own, inside the current test case.
inline RecordingReporter record(const cpm::test::Registry& registry, const cpm::test::Options& options = {}) {
    RecordingReporter reporter;
    cpm::test::run(registry, options, reporter);
    return reporter;
}
