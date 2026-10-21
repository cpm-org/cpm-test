// CHECK, REQUIRE, INFO, FAIL and SKIP, as a run reports them.
#include "support/recording_reporter.hpp"

#include <cpm/test.hpp>

#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using cpm::test::Outcome;
using Strings = std::vector<std::string>;

TEST_CASE("passing checks are counted and report nothing") {
    cpm::test::Registry registry;
    registry.add("passes", [] {
        CHECK(1 + 1 == 2);
        CHECK_FALSE(1 + 1 == 3);
        REQUIRE(2 > 1);
        REQUIRE_FALSE(false);
    });
    const RecordingReporter recorded = record(registry);
    INFO("reported:\n{}", recorded.text());
    CHECK(recorded.failures.empty());
    CHECK(recorded.summary.passed == 1);
    CHECK(recorded.summary.checks_passed == 4);
    CHECK(recorded.summary.exit_code() == 0);
    REQUIRE(recorded.results.size() == 1);
    CHECK(recorded.results[0].name == "passes");
    CHECK(recorded.results[0].checks == 4);
}

TEST_CASE("a failing CHECK reports where, what, the values and the test case, and the test case carries on") {
    cpm::test::Registry registry;
    int line = 0;
    bool carried_on = false;
    registry.add("adds", [&] {
        const int two = 2;
        line = __LINE__ + 1;
        CHECK(two + 2 == 5);
        carried_on = true;
    });
    const RecordingReporter recorded = record(registry);
    INFO("reported:\n{}", recorded.text());
    CHECK(carried_on);
    REQUIRE(recorded.failures.size() == 1);
    const cpm::test::Failure& failure = recorded.failures[0];
    CHECK(failure.where.file.ends_with("checks.test.cpp"));
    CHECK(failure.where.line == line);
    CHECK(failure.message == "CHECK(two + 2 == 5) failed");
    CHECK(failure.detail == "values: 4 == 5");
    CHECK(failure.path == Strings{"adds"});
    CHECK(recorded.summary.failed == 1);
    CHECK(recorded.summary.exit_code() == 1);
}

TEST_CASE("a failing REQUIRE ends the test case, and the next one still runs") {
    cpm::test::Registry registry;
    bool reached = false;
    registry.add("stops", [&] {
        REQUIRE(1 == 2);
        reached = true;
    });
    registry.add("runs", [] { CHECK(true); });
    const RecordingReporter recorded = record(registry);
    CHECK_FALSE(reached);
    REQUIRE(recorded.results.size() == 2);
    CHECK(recorded.results[0].outcome == Outcome::failed);
    CHECK(recorded.results[1].outcome == Outcome::passed);
}

TEST_CASE("every comparison reports its operator and both values") {
    cpm::test::Registry registry;
    registry.add("compares", [] {
        const int one = 1;
        const int two = 2;
        CHECK(one != one);
        CHECK(two < one);
        CHECK(two <= one);
        CHECK(one > two);
        CHECK(one >= two);
        CHECK_FALSE(one == one);
        CHECK(one & two);
        CHECK(one ^ one);
        CHECK(0 | 0);
    });
    const RecordingReporter recorded = record(registry);
    Strings details;
    for (const cpm::test::Failure& failure : recorded.failures) details.push_back(failure.detail);
    CHECK(details == Strings{"values: 1 != 1", "values: 2 < 1", "values: 2 <= 1", "values: 1 > 2", "values: 1 >= 2",
                             "values: 1 == 1", "values: 1 & 2", "values: 1 ^ 1", "values: 0 | 0"});
}

TEST_CASE("comparisons run as written, once") {
    int calls = 0;
    const auto next = [&] { return ++calls; };
    CHECK(next() == 1);
    CHECK(calls == 1);

    const std::vector<int> v{1, 2, 3};
    CHECK(v.size() == 3);  // mixed signedness compiles without warnings
    CHECK(-1 < 0);
    const unsigned flags = 6;
    CHECK(flags & 4u);
    CHECK((flags & 1u) == 0u);
    const int* null = nullptr;
    CHECK_FALSE(null);
    CHECK((1 == 1 && 2 == 2));  // parenthesised: checked, though not decomposed
}

TEST_CASE("an exception thrown by a checked expression fails the check, and the test case carries on") {
    cpm::test::Registry registry;
    bool carried_on = false;
    registry.add("throws", [&] {
        const auto boom = []() -> int { throw std::runtime_error("boom"); };
        CHECK(boom() == 1);
        carried_on = true;
    });
    const RecordingReporter recorded = record(registry);
    CHECK(carried_on);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].message == "CHECK(boom() == 1) failed");
    CHECK(recorded.failures[0].detail == "exception: \"boom\"");
}

TEST_CASE("INFO and CAPTURE go with failures in their scope, formatted at the failure") {
    cpm::test::Registry registry;
    registry.add("informs", [] {
        int i = 0;
        INFO("i is {}", i);
        const std::string name = "x";
        CAPTURE(name);
        i = 7;
        CHECK(i == 8);
        {
            INFO("inner");
            CHECK(false);
        }
        CHECK(i == 9);
    });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 3);
    CHECK(recorded.failures[0].info == Strings{"i is 7", "name = \"x\""});
    CHECK(recorded.failures[1].info == Strings{"i is 7", "name = \"x\"", "inner"});
    CHECK(recorded.failures[2].info == Strings{"i is 7", "name = \"x\""});
}

TEST_CASE("FAIL ends the test case, FAIL_CHECK carries on") {
    cpm::test::Registry registry;
    bool after_fail_check = false;
    bool after_fail = false;
    registry.add("fails", [&] {
        FAIL_CHECK("first {}", 1);
        after_fail_check = true;
        FAIL("second");
        after_fail = true;
    });
    const RecordingReporter recorded = record(registry);
    CHECK(after_fail_check);
    CHECK_FALSE(after_fail);
    REQUIRE(recorded.failures.size() == 2);
    CHECK(recorded.failures[0].message == "first 1");
    CHECK(recorded.failures[0].detail.empty());
    CHECK(recorded.failures[1].message == "second");
    CHECK(recorded.summary.checks_failed == 2);
}

TEST_CASE("SKIP ends the test case and counts it as skipped, unless it already failed") {
    cpm::test::Registry registry;
    bool reached = false;
    registry.add("skips", [&] {
        SKIP("no network on {}", "CI");
        reached = true;
    });
    registry.add("fails, then skips", [] {
        CHECK(false);
        SKIP("too late");
    });
    const RecordingReporter recorded = record(registry);
    CHECK_FALSE(reached);
    REQUIRE(recorded.skips.size() == 2);
    CHECK(recorded.skips[0].reason == "no network on CI");
    CHECK(recorded.skips[0].path == Strings{"skips"});
    REQUIRE(recorded.results.size() == 2);
    CHECK(recorded.results[0].outcome == Outcome::skipped);
    CHECK(recorded.results[1].outcome == Outcome::failed);
}

TEST_CASE("an exception escaping a test case fails it, with the INFOs it left behind") {
    cpm::test::Registry registry;
    registry.add("escapes", [] {
        for (int i = 0; i < 5; ++i) {
            INFO("i = {}", i);
            if (i == 3) throw std::logic_error("three");
        }
    });
    registry.add("throws an int", [] { throw 42; });
    const RecordingReporter recorded = record(registry);
    INFO("reported:\n{}", recorded.text());
    REQUIRE(recorded.failures.size() == 2);
    CHECK(recorded.failures[0].message == "unexpected exception");
    CHECK(recorded.failures[0].detail == "exception: \"three\"");
    CHECK(recorded.failures[0].path == Strings{"escapes"});
    CHECK(recorded.failures[0].info == Strings{"i = 3"});
    CHECK(recorded.failures[1].detail == "exception: (unknown type)");
}

TEST_CASE("an exception that is caught leaves no INFOs behind for a later one") {
    cpm::test::Registry registry;
    registry.add("t", [] {
        try {
            INFO("caught");
            throw 1;
        } catch (int) {
        }
        CHECK(true);
        throw std::runtime_error("later");
    });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].info.empty());
}

TEST_CASE("checks from other threads are counted and reported") {
    cpm::test::Registry registry;
    registry.add("threads", [] {
        std::vector<std::thread> threads;
        for (int t = 0; t < 4; ++t) {
            threads.emplace_back([t] {
                for (int i = 0; i < 1000; ++i) CHECK(i >= 0);
                if (t == 2) CHECK(t == 3);
            });
        }
        for (std::thread& thread : threads) thread.join();
    });
    const RecordingReporter recorded = record(registry);
    CHECK(recorded.summary.checks_passed == 4000);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].detail == "values: 2 == 3");
    CHECK(recorded.failures[0].path == Strings{"threads"});
}

TEST_CASE("fail_fast stops after the first failing test case") {
    cpm::test::Registry registry;
    registry.add("one", [] { CHECK(true); });
    registry.add("two", [] { CHECK(false); });
    registry.add("three", [] { CHECK(true); });
    cpm::test::Options options;
    options.fail_fast = true;
    const RecordingReporter recorded = record(registry, options);
    CHECK(recorded.results.size() == 2);
    CHECK(recorded.runs_finished == 1);
}

TEST_CASE("runs nest: checks go to the innermost run, and the outer one carries on") {
    cpm::test::Registry registry;
    registry.add("inner", [] { CHECK(false); });
    const RecordingReporter recorded = record(registry);
    CHECK(recorded.summary.checks_failed == 1);
    CHECK(true);  // reported to this, the outer run
}
