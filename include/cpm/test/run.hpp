// Running the test cases of a registry.
#pragma once

#include <cpm/test/registry.hpp>
#include <cpm/test/reporter.hpp>
#include <cpm/test/results.hpp>

#include <iosfwd>
#include <string>
#include <vector>

namespace cpm::test {

struct Options {
    // Run the test cases whose names match one of these patterns, or all of them when there are none. `*` matches
    // any text, and every other character matches itself.
    std::vector<std::string> include;
    // Leave out the test cases whose names match one of these.
    std::vector<std::string> exclude;
    // Stop after the first test case that fails.
    bool fail_fast = false;
};

// Runs the test cases `options` select, telling `reporter` what happens.
Summary run(const Registry& registry, const Options& options, Reporter& reporter);

// The same, reporting to `out` through a ConsoleReporter.
Summary run(const Registry& registry, const Options& options, std::ostream& out);

}  // namespace cpm::test
