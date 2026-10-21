#include "exceptions.hpp"

#include <cpm/test/to_string.hpp>

#include <exception>

namespace cpm::test::detail {

std::optional<std::string> current_exception_message() {
    try {
        throw;
    } catch (const std::exception& e) {
        return std::string(e.what());
    } catch (const std::string& s) {
        return s;
    } catch (const char* s) {
        return std::string(s != nullptr ? s : "");
    } catch (...) {
        return std::nullopt;
    }
}

std::string describe_current_exception() {
    const std::optional<std::string> message = current_exception_message();
    return message ? print::quote(*message) : std::string("(unknown type)");
}

}  // namespace cpm::test::detail
