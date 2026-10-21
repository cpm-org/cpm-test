#include "context.hpp"
#include "exceptions.hpp"

#include <cpm/test/detail/assertion.hpp>

#include <format>
#include <optional>
#include <utility>

namespace cpm::test::detail {

void Assertion::fail_with_exception(std::string_view prefix) {
    fail(std::string(prefix) + describe_current_exception());
}

void Assertion::check_exception_message(std::string_view expected) {
    const std::optional<std::string> message = current_exception_message();
    if (message && *message == expected) {
        pass();
    } else {
        fail("exception: " + describe_current_exception());
    }
}

void Assertion::finish() {
    if (passed_) {
        context().check_passed();
        return;
    }
    context().check_failed(Location{file_, line_}, std::format("{}({}) failed", macro_, expression_),
                           std::move(detail_));
    if (kind_ == Kind::require) throw Stop{};
}

void fail_and_continue(const char* file, int line, std::string message) {
    context().check_failed(Location{file, line}, std::move(message), {});
}

void fail_and_stop(const char* file, int line, std::string message) {
    fail_and_continue(file, line, std::move(message));
    throw Stop{};
}

void skip(const char* file, int line, std::string reason) {
    context().skipped(Location{file, line}, std::move(reason));
    throw Stop{true};
}

}  // namespace cpm::test::detail
