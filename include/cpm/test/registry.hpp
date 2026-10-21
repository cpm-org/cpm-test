// Test cases, and the registry TEST_CASE adds them to.
#pragma once

#include <functional>
#include <source_location>
#include <string>
#include <vector>

namespace cpm::test {

struct TestCase {
    std::string name;
    std::string file;
    int line = 0;
    std::function<void()> body;
};

// An ordered set of test cases. TEST_CASE adds to registry(); tests of test tools can build their own.
class Registry {
public:
    void add(TestCase test);
    void add(std::string name, std::function<void()> body,
             std::source_location where = std::source_location::current());

    const std::vector<TestCase>& cases() const noexcept { return cases_; }

private:
    std::vector<TestCase> cases_;
};

// The registry TEST_CASE adds to.
Registry& registry();

namespace detail {

// What TEST_CASE runs while the program starts.
bool add_test_case(const char* name, void (*body)(), const char* file, int line);

}  // namespace detail
}  // namespace cpm::test
