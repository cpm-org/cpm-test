#include "info_stack.hpp"

#include <cpm/test/detail/info.hpp>

#include <algorithm>
#include <iterator>

namespace cpm::test::detail {
namespace {

thread_local std::vector<const InfoEntry*> in_scope;     // outermost first
thread_local std::vector<std::string> unwound;           // innermost first: the order an exception leaves them in

std::string message_of(const InfoEntry* entry) noexcept {
    try {
        return entry->message(entry->context);
    } catch (...) {
        return "(this INFO threw while being formatted)";
    }
}

}  // namespace

void push_info(const InfoEntry* entry) { in_scope.push_back(entry); }

void pop_info(const InfoEntry* entry, bool unwinding) noexcept {
    if (unwinding) {
        try {
            unwound.push_back(message_of(entry));
        } catch (...) {
            // Out of memory: the message is lost, the exception still reported.
        }
    }
    if (!in_scope.empty() && in_scope.back() == entry) {
        in_scope.pop_back();
    } else if (const auto it = std::find(in_scope.rbegin(), in_scope.rend(), entry); it != in_scope.rend()) {
        in_scope.erase(std::next(it).base());
    }
}

std::vector<std::string> info_messages() {
    std::vector<std::string> messages;
    messages.reserve(in_scope.size());
    for (const InfoEntry* entry : in_scope) messages.push_back(message_of(entry));
    return messages;
}

std::vector<std::string> unwound_info_messages() { return {unwound.rbegin(), unwound.rend()}; }

void forget_unwound_info() noexcept { unwound.clear(); }

}  // namespace cpm::test::detail
