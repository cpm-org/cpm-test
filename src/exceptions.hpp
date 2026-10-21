// Reading the exception being handled. Call these only inside a catch block.
#pragma once

#include <optional>
#include <string>

namespace cpm::test::detail {

// what() of a std::exception, a thrown std::string or C string, or nullopt for anything else.
std::optional<std::string> current_exception_message();

// The message quoted, or "(unknown type)".
std::string describe_current_exception();

}  // namespace cpm::test::detail
