// What CHECK, REQUIRE, FAIL and SKIP expand to. Not for direct use; may change in any release.
#pragma once

#include <string>
#include <string_view>
#include <utility>

namespace cpm::test::detail {

// Thrown to end a test case early: after a failed REQUIRE or FAIL (already reported), or by SKIP. Not derived from
// std::exception, so `catch (const std::exception&)` in tested code lets it through; `catch (...)` would not.
struct Stop {
    bool skipped = false;
};

enum class Kind { check, require };

// One check: built before its expression runs, told how that went, then finish() reports it.
class Assertion {
public:
    Assertion(Kind kind, bool negate, const char* macro, const char* expression, const char* file, int line) noexcept
        : kind_(kind), negate_(negate), macro_(macro), expression_(expression), file_(file), line_(line) {}

    // For a decomposed expression: an Lhs or a Binary.
    template <class E>
    void evaluate(const E& expression) {
        if (expression.passed() != negate_) {
            pass();
        } else {
            fail("values: " + expression.expand());
        }
    }
    void pass() noexcept { passed_ = true; }
    void fail(std::string detail) {
        passed_ = false;
        detail_ = std::move(detail);
    }
    // In a catch (...) block: fails, describing the exception being handled after `prefix`.
    void fail_with_exception(std::string_view prefix = "exception: ");
    // In a catch (...) block: passes if the message of the exception being handled is `expected`.
    void check_exception_message(std::string_view expected);
    // Reports the outcome; after a failed REQUIRE, ends the test case.
    void finish();

private:
    Kind kind_;
    bool negate_;
    bool passed_ = false;
    const char* macro_;
    const char* expression_;
    const char* file_;
    int line_;
    std::string detail_;
};

[[noreturn]] void fail_and_stop(const char* file, int line, std::string message);
void fail_and_continue(const char* file, int line, std::string message);
[[noreturn]] void skip(const char* file, int line, std::string reason);

}  // namespace cpm::test::detail
