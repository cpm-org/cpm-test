// The command line of test programs: parse_command_line, and main(registry, args, out).
#include <cpm/test.hpp>

#include <sstream>
#include <string>
#include <utility>
#include <vector>

using cpm::test::CommandLine;
using cpm::test::parse_command_line;
using cpm::test::UsageError;
using Strings = std::vector<std::string>;

TEST_CASE("parsing: patterns, exclusions and options") {
    const CommandLine command = parse_command_line({"parse*", "~*slow*", "-v", "--fail-fast", "print"});
    CHECK(command.action == CommandLine::Action::run);
    CHECK(command.options.include == Strings{"parse*", "print"});
    CHECK(command.options.exclude == Strings{"*slow*"});
    CHECK(command.options.fail_fast);
    CHECK(command.verbose);
}

TEST_CASE("parsing: --help wins over --list, which wins over running") {
    CHECK(parse_command_line({}).action == CommandLine::Action::run);
    CHECK(parse_command_line({"-l"}).action == CommandLine::Action::list);
    CHECK(parse_command_line({"--list"}).action == CommandLine::Action::list);
    CHECK(parse_command_line({"--list", "-h"}).action == CommandLine::Action::help);
    CHECK(parse_command_line({"--help"}).action == CommandLine::Action::help);
}

TEST_CASE("parsing: an unknown option is a UsageError with a hint") {
    CHECK_THROWS_AS(parse_command_line({"--nope"}), UsageError);
    CHECK_THROWS_WITH(parse_command_line({"-x"}), "unknown option '-x'");
    try {
        parse_command_line({"--nope"});
    } catch (const UsageError& error) {
        CHECK(error.hint() == "--help lists the options");
    }
    CHECK(parse_command_line({"-"}).options.include == Strings{"-"});  // a lone dash is a pattern
}

namespace {

struct Result {
    int code;
    std::string output;
};

Result run_main(const cpm::test::Registry& registry, const Strings& args) {
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

TEST_CASE("main runs the chosen test cases and returns the exit code") {
    const Result all = run_main(sample(), {});
    INFO("output:\n{}", all.output);
    CHECK(all.code == 1);
    CHECK(all.output.ends_with("test cases: 2 passed, 1 failed; checks: 2 passed, 1 failed\n"));

    const Result parse = run_main(sample(), {"parse*"});
    CHECK(parse.code == 0);
    CHECK(parse.output == "test cases: 2 passed; checks: 2 passed\n");
}

TEST_CASE("main --list prints the chosen names in order") {
    CHECK(run_main(sample(), {"--list"}).output == "parse number\nparse string\nprint\n");
    CHECK(run_main(sample(), {"-l", "p*", "~*number"}).output == "parse string\nprint\n");
}

TEST_CASE("main -v prints every test case") {
    const Result r = run_main(sample(), {"-v", "parse*"});
    CHECK(r.output == "passed: parse number (1 check)\npassed: parse string (1 check)\n"
                      "test cases: 2 passed; checks: 2 passed\n");
}

TEST_CASE("main --help") {
    const Result r = run_main(sample(), {"--help"});
    CHECK(r.code == 0);
    CHECK(r.output == cpm::test::usage());
    CHECK(r.output.starts_with("usage: TEST [options] [pattern...]\n"));
}

TEST_CASE("main exits with 2 for a bad command line") {
    const Result option = run_main(sample(), {"--nope"});
    CHECK(option.code == 2);
    CHECK(option.output == "error: unknown option '--nope'\n  hint: --help lists the options\n");

    const Result pattern = run_main(sample(), {"missing", "gone*"});
    CHECK(pattern.code == 2);
    CHECK(pattern.output == "error: no test case matches 'missing', 'gone*'\n  hint: --list shows their names\n");
}

TEST_CASE("main exits with 77 when nothing passed, so cpm test counts the program as skipped") {
    cpm::test::Registry registry;
    CHECK(run_main(registry, {}).code == 77);

    registry.add("skips", [] { SKIP("not here"); });
    CHECK(run_main(registry, {}).code == 77);

    registry.add("passes", [] {});
    CHECK(run_main(registry, {}).code == 0);
}
