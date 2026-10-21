#include "context.hpp"

#include <cpm/test/detail/subcase.hpp>

#include <exception>

namespace cpm::test::detail {

Subcase::Subcase(std::string_view name, const char* file, int line)
    : entered_(context().enter_subcase(name, Location{file, line})), uncaught_(std::uncaught_exceptions()) {}

Subcase::~Subcase() {
    if (entered_) context().leave_subcase(std::uncaught_exceptions() > uncaught_);
}

}  // namespace cpm::test::detail
