// The INFO messages of the calling thread, as failures and escaping exceptions need them.
#pragma once

#include <string>
#include <vector>

namespace cpm::test::detail {

// The messages of the INFOs in scope, outermost first.
std::vector<std::string> info_messages();

// The messages of the INFOs an exception left while unwinding, outermost first.
std::vector<std::string> unwound_info_messages();

// Forgets those: the exception they belonged to has been dealt with.
void forget_unwound_info() noexcept;

}  // namespace cpm::test::detail
