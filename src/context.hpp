// The seam between code inside a test case (checks, subcases, SKIP) and whatever is running that test case.
#pragma once

#include <cpm/test/results.hpp>

#include <string>
#include <string_view>

namespace cpm::test::detail {

// What code inside a test case reports to. Checks may call it from any thread.
class Context {
public:
    virtual ~Context() = default;

    virtual void check_passed() = 0;
    virtual void check_failed(Location where, std::string message, std::string detail) = 0;
    virtual void skipped(Location where, std::string reason) = 0;
    // Whether this pass enters the subcase. Every true is followed by leave_subcase().
    virtual bool enter_subcase(std::string_view name, Location where) = 0;
    virtual void leave_subcase(bool unwinding) = 0;

protected:
    Context() = default;
    Context(const Context&) = default;
    Context& operator=(const Context&) = default;
};

// The context of the test case running now; outside of one, a context that writes failures to std::cerr.
Context& context() noexcept;

// Makes a context the current one for its lifetime, then brings back the one before: runs can nest.
class ContextScope {
public:
    explicit ContextScope(Context& active) noexcept;
    ~ContextScope();
    ContextScope(const ContextScope&) = delete;
    ContextScope& operator=(const ContextScope&) = delete;

private:
    Context* previous_;
};

}  // namespace cpm::test::detail
