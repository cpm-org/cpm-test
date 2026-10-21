#include "context.hpp"

#include <atomic>
#include <format>
#include <iostream>

namespace cpm::test::detail {
namespace {

// Checks made outside of a test run (say, in a helper called from main()) still say when they fail.
class OutsideRun final : public Context {
public:
    void check_passed() override {}
    void check_failed(Location where, std::string message, std::string detail) override {
        std::cerr << std::format("{}:{}: error: {} (outside of a test run)\n", where.file, where.line, message);
        if (!detail.empty()) std::cerr << "  " << detail << '\n';
    }
    void skipped(Location, std::string) override {}
    bool enter_subcase(std::string_view, Location) override { return false; }
    void leave_subcase(bool) override {}
};

std::atomic<Context*> current{nullptr};

}  // namespace

Context& context() noexcept {
    static OutsideRun outside;
    Context* const active = current.load();
    return active != nullptr ? *active : outside;
}

ContextScope::ContextScope(Context& active) noexcept : previous_(current.exchange(&active)) {}

ContextScope::~ContextScope() { current.store(previous_); }

}  // namespace cpm::test::detail
