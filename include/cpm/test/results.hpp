// What a test run produces: failures, skips, the outcome of each test case, and the summary.
#pragma once

#include <string>
#include <vector>

namespace cpm::test {

struct Location {
    std::string file;
    int line = 0;
};

// A failed check, a FAIL, or an exception that escaped a test case.
struct Failure {
    Location where;
    std::string message;            // e.g. "CHECK(a == b) failed"
    std::string detail;             // e.g. "values: 1 == 2"; empty when there is nothing to add
    std::vector<std::string> path;  // the test case's name, then those of the subcases it was in
    std::vector<std::string> info;  // the INFO messages in scope, outermost first
};

// A test case that SKIP ended.
struct Skip {
    Location where;
    std::string reason;
    std::vector<std::string> path;
};

enum class Outcome { passed, failed, skipped };

struct TestCaseResult {
    std::string name;
    Outcome outcome = Outcome::passed;
    long long checks = 0;
};

struct Summary {
    int passed = 0;  // test cases
    int failed = 0;
    int skipped = 0;
    long long checks_passed = 0;
    long long checks_failed = 0;

    void add(Outcome outcome) noexcept {
        switch (outcome) {
        case Outcome::passed: ++passed; break;
        case Outcome::failed: ++failed; break;
        case Outcome::skipped: ++skipped; break;
        }
    }

    // 1 when something failed, 77 (skipped, as `cpm test` reads it) when nothing passed, otherwise 0.
    int exit_code() const noexcept { return failed > 0 ? 1 : passed == 0 ? 77 : 0; }
};

}  // namespace cpm::test
