#include "support/recording_reporter.hpp"

#include <cpm/test.hpp>

#include <limits>

using cpm::test::approx;

TEST_CASE("approx allows a small relative difference") {
    CHECK(0.1 + 0.2 == approx(0.3));
    CHECK(approx(0.3) == 0.1 + 0.2);
    CHECK(1.0 != approx(1.1));
    CHECK(1000000.0 == approx(1000001.0).epsilon(1e-5));
    CHECK(1000000.0 != approx(1000100.0).epsilon(1e-5));
    CHECK(2.0f == approx(2.0));
    CHECK(3 == approx(3.0));
}

TEST_CASE("approx compares to zero only with a margin") {
    CHECK(1e-12 != approx(0.0));
    CHECK(1e-12 == approx(0.0).margin(1e-9));
    CHECK(-1e-12 == approx(0.0).margin(1e-9));
}

TEST_CASE("approx and infinities") {
    const double inf = std::numeric_limits<double>::infinity();
    CHECK(inf == approx(inf));
    CHECK(-inf != approx(inf));
    CHECK(1e300 != approx(inf));
}

TEST_CASE("a failing approx shows both values") {
    cpm::test::Registry registry;
    registry.add("t", [] { CHECK(0.5 == approx(0.25)); });
    const RecordingReporter recorded = record(registry);
    REQUIRE(recorded.failures.size() == 1);
    CHECK(recorded.failures[0].detail == "values: 0.5 == approx(0.25)");
}
