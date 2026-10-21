// Checks the basics without trusting cpm-test to check itself: plain ifs, and a main() of its own, which also shows
// that a test program can define main() instead of taking the library's.
#include <cpm/test.hpp>

#include <cstdio>
#include <sstream>
#include <string>

int main() {
    int failures = 0;
    const auto expect = [&](bool ok, const char* what) {
        if (!ok) {
            std::fprintf(stderr, "bootstrap: expected %s\n", what);
            ++failures;
        }
    };

    bool after_require = false;
    cpm::test::Registry registry;
    registry.add("passes", [] { CHECK(1 + 1 == 2); });
    registry.add("fails", [] { CHECK(1 + 1 == 3); });
    registry.add("stops", [&] {
        REQUIRE(false);
        after_require = true;
    });

    std::ostringstream out;
    const cpm::test::Summary summary = cpm::test::run(registry, {}, out);
    const std::string output = out.str();
    expect(summary.passed == 1, "1 test case to pass");
    expect(summary.failed == 2, "2 test cases to fail");
    expect(summary.checks_passed == 1, "1 check to pass");
    expect(summary.checks_failed == 2, "2 checks to fail");
    expect(summary.exit_code() == 1, "exit code 1");
    expect(!after_require, "REQUIRE to end its test case");
    expect(output.find("error: CHECK(1 + 1 == 3) failed\n  values: 2 == 3\n") != std::string::npos,
           "the failing CHECK and its values in the output");

    if (failures > 0) {
        std::fprintf(stderr, "bootstrap: the output was:\n%s", output.c_str());
        return 1;
    }
    std::puts("bootstrap: ok");
    return 0;
}
