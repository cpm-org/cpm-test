#include "support/inner.hpp"

#include <cpm/test.hpp>

#include <atomic>
#include <format>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

TEST_CASE("passing checks are counted, and only the summary is printed") {
    cpm::test::Registry registry;
    registry.add("passes", [] {
        CHECK(1 + 1 == 2);
        CHECK_FALSE(1 + 1 == 3);
        REQUIRE(2 > 1);
        REQUIRE_FALSE(false);
    });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.passed == 1);
    CHECK(inner.summary.failed == 0);
    CHECK(inner.summary.checks_passed == 4);
    CHECK(inner.summary.exit_code() == 0);
    CHECK(inner.output == "test cases: 1 passed; checks: 4 passed\n");
}

TEST_CASE("a failing CHECK prints where, what, the values and the test case, and the test case carries on") {
    cpm::test::Registry registry;
    int line = 0;
    bool carried_on = false;
    registry.add("adds", [&] {
        const int two = 2;
        line = __LINE__ + 1;
        CHECK(two + 2 == 5);
        carried_on = true;
    });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(carried_on);
    CHECK(inner.summary.failed == 1);
    CHECK(inner.summary.checks_failed == 1);
    CHECK(inner.summary.exit_code() == 1);
    CHECK(contains(inner.output,
                   std::format(":{}: error: CHECK(two + 2 == 5) failed\n  values: 4 == 5\n  in: \"adds\"\n", line)));
    CHECK(contains(inner.output, "test cases: 0 passed, 1 failed; checks: 0 passed, 1 failed\n"));
}

TEST_CASE("a failing REQUIRE ends the test case, and the next one still runs") {
    cpm::test::Registry registry;
    bool reached = false;
    registry.add("stops", [&] {
        REQUIRE(1 == 2);
        reached = true;
    });
    registry.add("runs", [] { CHECK(true); });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK_FALSE(reached);
    CHECK(inner.summary.failed == 1);
    CHECK(inner.summary.passed == 1);
    CHECK(contains(inner.output, "error: REQUIRE(1 == 2) failed\n  values: 1 == 2\n"));
}

TEST_CASE("every comparison shows its operator and both values") {
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
    });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.checks_failed == 7);
    for (const char* values : {"1 != 1", "2 < 1", "2 <= 1", "1 > 2", "1 >= 2", "1 == 1", "1 & 2"}) {
        CHECK(contains(inner.output, std::format("  values: {}\n", values)));
    }
    CHECK(contains(inner.output, "CHECK_FALSE(one == one) failed"));
}

TEST_CASE("comparisons run as written, once") {
    int calls = 0;
    const auto next = [&] { return ++calls; };
    CHECK(next() == 1);
    CHECK(calls == 1);

    const std::vector<int> v{1, 2, 3};
    CHECK(v.size() == 3);  // mixed signedness compiles without warnings
    CHECK(-1 < 0);
    unsigned flags = 6;
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(carried_on);
    CHECK(inner.summary.checks_failed == 1);
    CHECK(contains(inner.output, "error: CHECK(boom() == 1) failed\n  exception: \"boom\"\n"));
}

TEST_CASE("INFO and CAPTURE show their messages with failures in their scope, formatted at the failure") {
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(contains(inner.output, "  values: 7 == 8\n  in: \"informs\"\n  info: i is 7\n  info: name = \"x\"\n"));
    CHECK(contains(inner.output, "  info: i is 7\n  info: name = \"x\"\n  info: inner\n"));
    // By the last failure "inner" is out of scope.
    CHECK(contains(inner.output,
                   "  values: 7 == 9\n  in: \"informs\"\n  info: i is 7\n  info: name = \"x\"\ntest cases: "));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(after_fail_check);
    CHECK_FALSE(after_fail);
    CHECK(inner.summary.checks_failed == 2);
    CHECK(contains(inner.output, ": error: first 1\n  in: \"fails\"\n"));
    CHECK(contains(inner.output, ": error: second\n  in: \"fails\"\n"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK_FALSE(reached);
    CHECK(inner.summary.skipped == 1);
    CHECK(inner.summary.failed == 1);
    CHECK(contains(inner.output, ": note: skipped: no network on CI\n  in: \"skips\"\n"));
    CHECK(contains(inner.output, "test cases: 0 passed, 1 failed, 1 skipped;"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.failed == 2);
    CHECK(contains(inner.output, ": error: unexpected exception\n  exception: \"three\"\n  in: \"escapes\"\n  info: i = 3\n"));
    CHECK(contains(inner.output, ": error: unexpected exception\n  exception: (unknown type)\n  in: \"throws an int\"\n"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.checks_passed == 4000);
    CHECK(inner.summary.checks_failed == 1);
    CHECK(contains(inner.output, "  values: 2 == 3\n  in: \"threads\"\n"));
}

TEST_CASE("fail_fast stops after the first failing test case") {
    cpm::test::Registry registry;
    registry.add("one", [] { CHECK(true); });
    registry.add("two", [] { CHECK(false); });
    registry.add("three", [] { CHECK(true); });
    cpm::test::Options options;
    options.fail_fast = true;
    const Inner inner = run_inner(registry, options);
    CHECK(inner.summary.passed == 1);
    CHECK(inner.summary.failed == 1);
}

TEST_CASE("runs nest: checks go to the innermost run, and the outer one carries on") {
    std::atomic<int> inner_failures{0};
    cpm::test::Registry registry;
    registry.add("inner", [&] {
        CHECK(false);
        ++inner_failures;
    });
    const Inner inner = run_inner(registry);
    CHECK(inner.summary.checks_failed == 1);
    CHECK(inner_failures == 1);
    CHECK(true);  // reported to this, the outer run
}
