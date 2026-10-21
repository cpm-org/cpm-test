#include "filter.hpp"

#include <algorithm>

namespace cpm::test::detail {
namespace {

bool matches_any(const std::vector<std::string>& patterns, std::string_view name) {
    return std::ranges::any_of(patterns, [&](const std::string& pattern) { return matches(pattern, name); });
}

}  // namespace

bool matches(std::string_view pattern, std::string_view text) noexcept {
    std::size_t p = 0;
    std::size_t t = 0;
    std::size_t star = std::string_view::npos;  // the last '*' passed, to backtrack to
    std::size_t resume = 0;                      // where in text to try that '*' next
    while (t < text.size()) {
        if (p < pattern.size() && pattern[p] == '*') {
            star = p++;
            resume = t;
        } else if (p < pattern.size() && pattern[p] == text[t]) {
            ++p;
            ++t;
        } else if (star != std::string_view::npos) {
            p = star + 1;
            t = ++resume;
        } else {
            return false;
        }
    }
    while (p < pattern.size() && pattern[p] == '*') ++p;
    return p == pattern.size();
}

Filter::Filter(const Options& options) : include_(options.include), exclude_(options.exclude) {}

bool Filter::selects(std::string_view name) const {
    return (include_.empty() || matches_any(include_, name)) && !matches_any(exclude_, name);
}

std::vector<const TestCase*> Filter::select(const Registry& registry) const {
    std::vector<const TestCase*> chosen;
    for (const TestCase& test : registry.cases()) {
        if (selects(test.name)) chosen.push_back(&test);
    }
    return chosen;
}

}  // namespace cpm::test::detail
