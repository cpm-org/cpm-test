// Where the events of a test run go. Implement Reporter for another output format.
#pragma once

#include <cpm/test/results.hpp>

#include <iosfwd>

namespace cpm::test {

// Receives what happens during a run. run() makes one call at a time, though checks made on other threads report
// from those threads.
class Reporter {
public:
    virtual ~Reporter() = default;

    virtual void failure(const Failure& failure) = 0;
    virtual void skipped(const Skip& skip) = 0;
    virtual void test_case_finished(const TestCaseResult& result) = 0;
    virtual void run_finished(const Summary& summary) = 0;

protected:
    Reporter() = default;
    Reporter(const Reporter&) = default;
    Reporter& operator=(const Reporter&) = default;
};

// Writes failures and skips as compiler-style diagnostics (file:line: error: ...), then a summary line; with
// `verbose`, also a line for every test case.
class ConsoleReporter final : public Reporter {
public:
    explicit ConsoleReporter(std::ostream& out, bool verbose = false) noexcept;

    void failure(const Failure& failure) override;
    void skipped(const Skip& skip) override;
    void test_case_finished(const TestCaseResult& result) override;
    void run_finished(const Summary& summary) override;

private:
    std::ostream& out_;
    bool verbose_;
};

}  // namespace cpm::test
