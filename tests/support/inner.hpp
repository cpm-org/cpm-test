// Runs a registry of its own inside a test case and keeps what it printed, for testing how cpm-test runs and reports.
#pragma once

#include <cpm/test.hpp>

#include <sstream>
#include <string>
#include <string_view>
#include <utility>

struct Inner {
    cpm::test::Summary summary;
    std::string output;
};

inline Inner run_inner(const cpm::test::Registry& registry, const cpm::test::Options& options = {}) {
    std::ostringstream out;
    Inner inner;
    inner.summary = cpm::test::run(registry, options, out);
    inner.output = std::move(out).str();
    return inner;
}

inline bool contains(std::string_view text, std::string_view part) { return text.find(part) != std::string_view::npos; }
