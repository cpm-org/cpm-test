// The command line of test programs: cpm::test::main(registry, args, out).
#include "support/inner.hpp"

#include <cpm/test.hpp>

#include <sstream>
#include <string>
#include <vector>

namespace {

struct Result {
    int code;
    std::string output;
};

Result run_main(const cpm::test::Registry& registry, const std::vector<std::string>& args) {
    std::ostringstream out;
    const int code = cpm::test::main(registry, args, out);
    return {code, std::move(out).str()};
}

cpm::test::Registry sample() {
    cpm::test::Registry registry;
    registry.add("parse number", [] { CHECK(true); });
    registry.add("parse string", [] { CHECK(true); });
    registry.add("print", [] { CHECK(1 == 2); });
    return registry;
}

}  // namespace

TEST_CASE("with no arguments every test case runs") {
    const Result r = run_main(sample(), {});
    INFO("output:\n{}", r.output);
    CHECK(r.code == 1);
    CHECK(contains(r.output, "test cases: 2 passed, 1 failed; checks: 2 passed, 1 failed\n"));
}

TEST_CASE("patterns choose test cases, and ~ leaves some out") {
    const Result parse = run_main(sample(), {"parse*"});
    INFO("output:\n{}", parse.output);
    CHECK(parse.code == 0);
    CHECK(contains(parse.output, "test cases: 2 passed;"));

    const Result exact = run_main(sample(), {"print"});
    CHECK(exact.code == 1);

    const Result excluded = run_main(sample(), {"~print"});
    CHECK(excluded.code == 0);
    CHECK(contains(excluded.output, "test cases: 2 passed;"));

    const Result both = run_main(sample(), {"*", "~*string"});
    CHECK(contains(both.output, "test cases: 1 passed, 1 failed;"));

    const Result middle = run_main(sample(), {"*e n*"});
    CHECK(contains(middle.output, "test cases: 1 passed;"));
}

TEST_CASE("--list prints the chosen test cases in order") {
    const Result all = run_main(sample(), {"--list"});
    CHECK(all.code == 0);
    CHECK(all.output == "parse number\nparse string\nprint\n");

    const Result some = run_main(sample(), {"-l", "p*", "~*number"});
    CHECK(some.output == "parse string\nprint\n");
}

TEST_CASE("--verbose prints every test case") {
    const Result r = run_main(sample(), {"-v"});
    INFO("output:\n{}", r.output);
    CHECK(contains(r.output, "passed: parse number (1 check)\n"));
    CHECK(contains(r.output, "failed: print (1 check)\n"));
}

TEST_CASE("--fail-fast stops at the first failure") {
    cpm::test::Registry registry;
    registry.add("fails", [] { CHECK(false); });
    registry.add("never runs", [] { CHECK(true); });
    const Result r = run_main(registry, {"--fail-fast"});
    CHECK(r.code == 1);
    CHECK(contains(r.output, "test cases: 0 passed, 1 failed;"));
}

TEST_CASE("--help") {
    const Result r = run_main(sample(), {"--help"});
    CHECK(r.code == 0);
    CHECK(r.output.starts_with("usage: TEST [options] [pattern...]\n"));
}

TEST_CASE("a bad command line exits with 2") {
    const Result option = run_main(sample(), {"--nope"});
    CHECK(option.code == 2);
    CHECK(option.output == "error: unknown option '--nope'\n  hint: --help lists the options\n");

    const Result pattern = run_main(sample(), {"missing", "gone*"});
    CHECK(pattern.code == 2);
    CHECK(pattern.output == "error: no test case matches 'missing', 'gone*'\n  hint: --list shows their names\n");
}

TEST_CASE("exit code 77 when nothing passed, so cpm test counts the program as skipped") {
    cpm::test::Registry registry;
    CHECK(run_main(registry, {}).code == 77);

    registry.add("skips", [] { SKIP("not here"); });
    CHECK(run_main(registry, {}).code == 77);

    registry.add("passes", [] {});
    CHECK(run_main(registry, {}).code == 0);
}
