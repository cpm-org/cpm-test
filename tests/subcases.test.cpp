// SUBCASE as test cases use it. subcase_tracker.test.cpp tests the bookkeeping on its own.
#include "support/recording_reporter.hpp"

#include <cpm/test.hpp>

#include <stdexcept>
#include <string>
#include <vector>

using Strings = std::vector<std::string>;

TEST_CASE("each subcase runs in its own pass, with the code around it") {
    std::string trace;
    cpm::test::Registry registry;
    registry.add("nested", [&] {
        trace += "T";
        SUBCASE("a") {
            trace += "a";
            SUBCASE("x") trace += "x";
            SUBCASE("y") trace += "y";
        }
        SUBCASE("b") trace += "b";
        trace += ".";
    });
    const RecordingReporter recorded = record(registry);
    CHECK(trace == "Tax.Tay.Tb.");
    CHECK(recorded.summary.passed == 1);
}

TEST_CASE("a failing REQUIRE in a subcase ends that pass only") {
    std::string trace;
    cpm::test::Registry registry;
    registry.add("t", [&] {
        SUBCASE("a") {
            trace += "a";
            REQUIRE(1 == 2);
            trace += "!";
        }
        SUBCASE("b") trace += "b";
    });
    const RecordingReporter recorded = record(registry);
    CHECK(trace == "ab");
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].path == Strings{"t", "a"});
}

TEST_CASE("an exception in a nested subcase doesn't keep its siblings from running") {
    std::string trace;
    cpm::test::Registry registry;
    registry.add("t", [&] {
        SUBCASE("a") {
            SUBCASE("x") {
                trace += "x";
                throw std::runtime_error("x failed");
            }
            SUBCASE("y") trace += "y";
        }
        SUBCASE("b") trace += "b";
    });
    const RecordingReporter recorded = record(registry);
    CHECK(trace == "xyb");
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].message == "unexpected exception");
    CHECK(recorded.failures[0].path == Strings{"t", "a", "x"});
}

TEST_CASE("failures in subcases report the whole path") {
    cpm::test::Registry registry;
    registry.add("outer", [] {
        SUBCASE("middle") {
            SUBCASE("inner") CHECK(1 == 0);
        }
    });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].path == Strings{"outer", "middle", "inner"});
}

// The same through TEST_CASE and the library's main(), as a user would write it.
TEST_CASE("a string grows and shrinks") {
    std::string s = "ab";
    REQUIRE(s.size() == 2);
    SUBCASE("grows") {
        s += 'c';
        CHECK(s == "abc");
    }
    SUBCASE("shrinks") {
        s.pop_back();
        CHECK(s == "a");
    }
    CHECK(s.size() != 2);
}
