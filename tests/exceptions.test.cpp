#include "support/inner.hpp"

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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(contains(inner.output, "error: CHECK_THROWS(no_throw()) failed\n  exception: none\n"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.checks_failed == 2);
    CHECK(contains(inner.output, "error: CHECK_THROWS_AS(throw_runtime(\"bad\"), std::logic_error) failed\n"
                                 "  exception of another type: \"bad\"\n"));
    CHECK(contains(inner.output, "error: CHECK_THROWS_AS(no_throw(), std::logic_error) failed\n  exception: none\n"));
}

TEST_CASE("CHECK_THROWS_WITH compares the exception's message") {
    CHECK_THROWS_WITH(throw_runtime("bad input"), "bad input");
    CHECK_THROWS_WITH(throw_string(), std::string("a string"));

    cpm::test::Registry registry;
    registry.add("t", [] {
        CHECK_THROWS_WITH(throw_runtime("bad input"), "good input");
        CHECK_THROWS_WITH(throw 7, "seven");
    });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(inner.summary.checks_failed == 2);
    CHECK(contains(inner.output, "CHECK_THROWS_WITH(throw_runtime(\"bad input\"), \"good input\") failed\n"
                                 "  exception: \"bad input\"\n"));
    CHECK(contains(inner.output, "  exception: (unknown type)\n"));
}

TEST_CASE("CHECK_NOTHROW fails with the exception") {
    CHECK_NOTHROW(no_throw());

    cpm::test::Registry registry;
    registry.add("t", [] { CHECK_NOTHROW(throw_runtime("oops")); });
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(contains(inner.output, "error: CHECK_NOTHROW(throw_runtime(\"oops\")) failed\n  exception: \"oops\"\n"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(reached == 0);
    CHECK(inner.summary.failed == 4);
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK_FALSE(reached);
    CHECK(inner.summary.checks_failed == 1);
    CHECK_FALSE(contains(inner.output, "exception:"));
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
    const Inner inner = run_inner(registry);
    CHECK_FALSE(reached);
    CHECK(inner.summary.failed == 1);
}
