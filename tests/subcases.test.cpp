#include "support/inner.hpp"

#include <cpm/test.hpp>

#include <stdexcept>
#include <string>

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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(trace == "Tax.Tay.Tb.");
    CHECK(inner.summary.passed == 1);
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(trace == "ab");
    CHECK(inner.summary.failed == 1);
    CHECK(contains(inner.output, "  values: 1 == 2\n  in: \"t\" > \"a\"\n"));
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
    const Inner inner = run_inner(registry);
    INFO("output:\n{}", inner.output);
    CHECK(trace == "xyb");
    CHECK(contains(inner.output, "error: unexpected exception\n  exception: \"x failed\"\n  in: \"t\" > \"a\" > \"x\"\n"));
}

TEST_CASE("a failing REQUIRE before any subcase ends the test case after one pass") {
    int passes = 0;
    cpm::test::Registry registry;
    registry.add("t", [&] {
        ++passes;
        REQUIRE(false);
        SUBCASE("a") {}
        SUBCASE("b") {}
    });
    const Inner inner = run_inner(registry);
    CHECK(passes == 1);
    CHECK(inner.summary.failed == 1);
}

TEST_CASE("subcases can have names made at run time") {
    std::string trace;
    cpm::test::Registry registry;
    registry.add("t", [&] {
        for (int i = 0; i < 3; ++i) {
            SUBCASE("case " + std::to_string(i)) trace += std::to_string(i);
        }
    });
    const Inner inner = run_inner(registry);
    CHECK(trace == "012");
}

TEST_CASE("failures in subcases name the whole path") {
    cpm::test::Registry registry;
    registry.add("outer", [] {
        SUBCASE("middle") {
            SUBCASE("inner") CHECK(1 == 0);
        }
    });
    const Inner inner = run_inner(registry);
    CHECK(contains(inner.output, "  in: \"outer\" > \"middle\" > \"inner\"\n"));
}

// The same through TEST_CASE and the library's main(), as a user would write it.
TEST_CASE("a string grows and shrinks") {
    std::string v = "ab";
    REQUIRE(v.size() == 2);
    SUBCASE("grows") {
        v += 'c';
        CHECK(v == "abc");
    }
    SUBCASE("shrinks") {
        v.pop_back();
        CHECK(v == "a");
    }
    CHECK(v.size() != 2);
}
