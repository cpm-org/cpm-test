// Choosing test cases by name.
#include <filter.hpp>

#include <cpm/test.hpp>

#include <string>
#include <vector>

using cpm::test::detail::Filter;
using cpm::test::detail::matches;

TEST_CASE("a pattern without * matches only itself") {
    CHECK(matches("parse", "parse"));
    CHECK_FALSE(matches("parse", "parser"));
    CHECK_FALSE(matches("parse", "pars"));
    CHECK(matches("", ""));
    CHECK_FALSE(matches("", "x"));
}

TEST_CASE("* matches any text, including none") {
    CHECK(matches("*", ""));
    CHECK(matches("*", "anything"));
    CHECK(matches("parse*", "parse"));
    CHECK(matches("parse*", "parse number"));
    CHECK(matches("*number", "parse number"));
    CHECK(matches("*e n*", "parse number"));
    CHECK(matches("a*b*c", "aXbYc"));
    CHECK(matches("a*b*c", "abbbc"));
    CHECK_FALSE(matches("a*b*c", "aXbY"));
    CHECK(matches("**", "x"));
}

TEST_CASE("matching is case-sensitive") {
    CHECK_FALSE(matches("Parse", "parse"));
}

namespace {

cpm::test::Registry sample() {
    cpm::test::Registry registry;
    for (const char* name : {"parse number", "parse string", "print"}) registry.add(name, [] {});
    return registry;
}

std::vector<std::string> names(const std::vector<const cpm::test::TestCase*>& tests) {
    std::vector<std::string> out;
    for (const cpm::test::TestCase* test : tests) out.push_back(test->name);
    return out;
}

}  // namespace

TEST_CASE("without patterns every test case is chosen, in order") {
    const cpm::test::Registry registry = sample();
    CHECK(names(Filter({}).select(registry)) == std::vector<std::string>{"parse number", "parse string", "print"});
}

TEST_CASE("include patterns choose, exclude patterns leave out") {
    const cpm::test::Registry registry = sample();
    cpm::test::Options options;
    options.include = {"parse*", "print"};
    options.exclude = {"*string"};
    CHECK(names(Filter(options).select(registry)) == std::vector<std::string>{"parse number", "print"});

    cpm::test::Options only_exclude;
    only_exclude.exclude = {"p*"};
    CHECK(Filter(only_exclude).select(registry).empty());
}
