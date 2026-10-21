// How ConsoleReporter prints each event.
#include <cpm/test.hpp>

#include <sstream>
#include <string>

using cpm::test::ConsoleReporter;

TEST_CASE("a failure: where and what, the detail, the path and the INFOs") {
    std::ostringstream out;
    ConsoleReporter reporter(out);
    reporter.failure({{"tests/a.test.cpp", 12}, "CHECK(x == 1) failed", "values: 2 == 1", {"t", "sub"}, {"i = 3"}});
    CHECK(out.str() ==
          "tests/a.test.cpp:12: error: CHECK(x == 1) failed\n"
          "  values: 2 == 1\n"
          "  in: \"t\" > \"sub\"\n"
          "  info: i = 3\n");
}

TEST_CASE("a failure without a detail, path or INFOs is one line") {
    std::ostringstream out;
    ConsoleReporter reporter(out);
    reporter.failure({{"a.cpp", 1}, "unreachable", "", {}, {}});
    CHECK(out.str() == "a.cpp:1: error: unreachable\n");
}

TEST_CASE("a skip is a note") {
    std::ostringstream out;
    ConsoleReporter reporter(out);
    reporter.skipped({{"a.cpp", 7}, "no network", {"download"}});
    CHECK(out.str() == "a.cpp:7: note: skipped: no network\n  in: \"download\"\n");
}

TEST_CASE("test cases get a line only when verbose") {
    std::ostringstream quiet;
    ConsoleReporter(quiet).test_case_finished({"t", cpm::test::Outcome::passed, 3});
    CHECK(quiet.str().empty());

    std::ostringstream out;
    ConsoleReporter verbose(out, true);
    verbose.test_case_finished({"adds", cpm::test::Outcome::passed, 3});
    verbose.test_case_finished({"parses", cpm::test::Outcome::failed, 1});
    verbose.test_case_finished({"downloads", cpm::test::Outcome::skipped, 0});
    CHECK(out.str() == "passed: adds (3 checks)\nfailed: parses (1 check)\nskipped: downloads (0 checks)\n");
}

TEST_CASE("the summary leaves out counts of zero after passed") {
    std::ostringstream all_passed;
    ConsoleReporter(all_passed).run_finished({2, 0, 0, 5, 0});
    CHECK(all_passed.str() == "test cases: 2 passed; checks: 5 passed\n");

    std::ostringstream mixed;
    ConsoleReporter(mixed).run_finished({1, 2, 3, 4, 5});
    CHECK(mixed.str() == "test cases: 1 passed, 2 failed, 3 skipped; checks: 4 passed, 5 failed\n");
}

TEST_CASE("run() with a stream reports through a ConsoleReporter") {
    cpm::test::Registry registry;
    registry.add("t", [] { CHECK(1 == 2); });
    std::ostringstream out;
    cpm::test::run(registry, {}, out);
    const std::string text = out.str();
    CHECK(text.find(": error: CHECK(1 == 2) failed\n  values: 1 == 2\n  in: \"t\"\n") != std::string::npos);
    CHECK(text.ends_with("test cases: 0 passed, 1 failed; checks: 0 passed, 1 failed\n"));
}
