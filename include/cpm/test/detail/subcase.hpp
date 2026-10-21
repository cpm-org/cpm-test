// What SUBCASE expands to. Not for direct use; may change in any release.
#pragma once

#include <string_view>

namespace cpm::test::detail {

// Asks the test case in progress whether this pass enters the subcase, and tells it when the subcase is left.
class Subcase {
public:
    Subcase(std::string_view name, const char* file, int line);
    ~Subcase();
    Subcase(const Subcase&) = delete;
    Subcase& operator=(const Subcase&) = delete;

    explicit operator bool() const noexcept { return entered_; }

private:
    bool entered_;
    int uncaught_;
};

}  // namespace cpm::test::detail
