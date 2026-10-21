#include <cpm/test/registry.hpp>

#include <utility>

namespace cpm::test {

void Registry::add(TestCase test) { cases_.push_back(std::move(test)); }

void Registry::add(std::string name, std::function<void()> body, std::source_location where) {
    add(TestCase{std::move(name), where.file_name(), static_cast<int>(where.line()), std::move(body)});
}

Registry& registry() {
    static Registry instance;
    return instance;
}

namespace detail {

bool add_test_case(const char* name, void (*body)(), const char* file, int line) {
    registry().add(TestCase{name, file, line, body});
    return true;
}

}  // namespace detail
}  // namespace cpm::test
