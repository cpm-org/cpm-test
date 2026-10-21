// cpm-test: a unit-test framework for C++20. See README.md for a guide.
//
//   #include <cpm/test.hpp>
//
//   TEST_CASE("addition") {
//       CHECK(1 + 1 == 2);          // on failure prints the values: 1 + 1 == 3 shows "values: 2 == 3"
//       SUBCASE("negative") {       // each subcase runs in its own pass through the test case
//           REQUIRE(-1 + -1 == -2); // REQUIRE ends the test case when it fails, CHECK carries on
//       }
//   }
//
// Test programs get main() from the library.
#pragma once

#include <cpm/test/approx.hpp>
#include <cpm/test/command_line.hpp>
#include <cpm/test/macros.hpp>
#include <cpm/test/registry.hpp>
#include <cpm/test/reporter.hpp>
#include <cpm/test/results.hpp>
#include <cpm/test/run.hpp>
#include <cpm/test/to_string.hpp>
