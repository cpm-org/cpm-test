// CHECK_THROWS and its kind, and how a failed REQUIRE travels through code under test.
#include "support/recording_reporter.hpp"

#include <cpm/test.hpp>

#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void throw_runtime(const char* message) { throw std::runtime_error(message); }
[[noreturn]] void throw_string() { throw std::string("a string"); }
int no_throw() { return 1; }

}  // namespace

TEST_CASE("CHECK_THROWS passes on any exception and fails without one") {
    CHECK_THROWS(throw_runtime("x"));
    CHECK_THROWS(throw_string());

    cpm::test::Registry registry;
    registry.add("t", [] { CHECK_THROWS(no_throw()); });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].message == "CHECK_THROWS(no_throw()) failed");
    CHECK(recorded.failures[0].detail == "exception: none");
}

TEST_CASE("CHECK_THROWS_AS passes on the type or a type derived from it") {
    CHECK_THROWS_AS(throw_runtime("x"), std::runtime_error);
    CHECK_THROWS_AS(throw_runtime("x"), std::exception);
    CHECK_THROWS_AS(throw_string(), std::string);

    cpm::test::Registry registry;
    registry.add("t", [] {
        CHECK_THROWS_AS(throw_runtime("bad"), std::logic_error);
        CHECK_THROWS_AS(no_throw(), std::logic_error);
    });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 2);
    CHECK(recorded.failures[0].message == "CHECK_THROWS_AS(throw_runtime(\"bad\"), std::logic_error) failed");
    CHECK(recorded.failures[0].detail == "exception of another type: \"bad\"");
    CHECK(recorded.failures[1].detail == "exception: none");
}

TEST_CASE("CHECK_THROWS_WITH compares the exception's message") {
    CHECK_THROWS_WITH(throw_runtime("bad input"), "bad input");
    CHECK_THROWS_WITH(throw_string(), std::string("a string"));

    cpm::test::Registry registry;
    registry.add("t", [] {
        CHECK_THROWS_WITH(throw_runtime("bad input"), "good input");
        CHECK_THROWS_WITH(throw 7, "seven");
    });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 2);
    CHECK(recorded.failures[0].message == "CHECK_THROWS_WITH(throw_runtime(\"bad input\"), \"good input\") failed");
    CHECK(recorded.failures[0].detail == "exception: \"bad input\"");
    CHECK(recorded.failures[1].detail == "exception: (unknown type)");
}

TEST_CASE("CHECK_NOTHROW fails with the exception") {
    CHECK_NOTHROW(no_throw());

    cpm::test::Registry registry;
    registry.add("t", [] { CHECK_NOTHROW(throw_runtime("oops")); });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].detail == "exception: \"oops\"");
}

TEST_CASE("the REQUIRE forms end the test case when they fail") {
    cpm::test::Registry registry;
    int reached = 0;
    registry.add("throws", [&] {
        REQUIRE_THROWS(no_throw());
        ++reached;
    });
    registry.add("throws as", [&] {
        REQUIRE_THROWS_AS(no_throw(), std::exception);
        ++reached;
    });
    registry.add("throws with", [&] {
        REQUIRE_THROWS_WITH(throw_runtime("a"), "b");
        ++reached;
    });
    registry.add("nothrow", [&] {
        REQUIRE_NOTHROW(throw_runtime("a"));
        ++reached;
    });
    const RecordingReporter recorded = record(registry);
    CHECK(reached == 0);
    CHECK(recorded.summary.failed == 4);
}

namespace {

bool helper_that_requires() {
    REQUIRE(1 == 2);
    return true;
}

}  // namespace

TEST_CASE("a REQUIRE failing inside a checked expression ends the test case instead of failing as an exception") {
    cpm::test::Registry registry;
    bool reached = false;
    registry.add("t", [&] {
        CHECK(helper_that_requires());
        CHECK_NOTHROW(helper_that_requires());
        reached = true;
    });
    const RecordingReporter recorded = record(registry);
    CHECK_FALSE(reached);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].message == "REQUIRE(1 == 2) failed");
}

TEST_CASE("code under test that catches std::exception lets a failed REQUIRE through") {
    cpm::test::Registry registry;
    bool reached = false;
    registry.add("t", [&] {
        try {
            REQUIRE(false);
        } catch (const std::exception&) {
        }
        reached = true;
    });
    const RecordingReporter recorded = record(registry);
    CHECK_FALSE(reached);
    CHECK(recorded.summary.failed == 1);
}
