// The macros tests are written with. Each has a CPM_TEST_ name; the short names (TEST_CASE, CHECK, ...) are defined
// too unless CPM_TEST_NO_SHORT_MACROS is.
#pragma once

#include <cpm/test/detail/assertion.hpp>
#include <cpm/test/detail/decompose.hpp>
#include <cpm/test/detail/info.hpp>
#include <cpm/test/detail/subcase.hpp>
#include <cpm/test/registry.hpp>
#include <cpm/test/to_string.hpp>

#include <format>
#include <string>

#define CPM_TEST_CAT2_(a, b) a##b
#define CPM_TEST_CAT_(a, b) CPM_TEST_CAT2_(a, b)
#define CPM_TEST_UNIQUE_(prefix) CPM_TEST_CAT_(prefix, __COUNTER__)

// `Decomposer{} <= a == b` looks like a chained comparison to -Wparentheses.
#if defined(__GNUC__)
#define CPM_TEST_SUPPRESS_BEGIN_ _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#define CPM_TEST_SUPPRESS_END_ _Pragma("GCC diagnostic pop")
#else
#define CPM_TEST_SUPPRESS_BEGIN_
#define CPM_TEST_SUPPRESS_END_
#endif

// ---- Test cases and subcases

#define CPM_TEST_CASE(name) CPM_TEST_CASE_IMPL_(CPM_TEST_UNIQUE_(cpm_test_case_), name)
#define CPM_TEST_CASE_IMPL_(fn, name)                                                                            \
    static void fn();                                                                                            \
    [[maybe_unused]] static const bool CPM_TEST_CAT_(fn, _registered) =                                          \
        ::cpm::test::detail::add_test_case(name, &fn, __FILE__, __LINE__);                                       \
    static void fn()

#define CPM_TEST_SUBCASE(name) CPM_TEST_SUBCASE_IMPL_(CPM_TEST_UNIQUE_(cpm_test_subcase_), name)
#define CPM_TEST_SUBCASE_IMPL_(var, name) if (const ::cpm::test::detail::Subcase var{name, __FILE__, __LINE__}; var)

// ---- Checks. Each builds an Assertion, runs the expression inside a try block, tells the Assertion how it went, and
// lets a Stop (a REQUIRE failing further down) through.

#define CPM_TEST_ASSERTION_(kind, negate, macro, text) \
    ::cpm::test::detail::Assertion cpm_test_assertion_(kind, negate, macro, text, __FILE__, __LINE__)

#define CPM_TEST_ASSERT_(kind, negate, macro, ...)                                                               \
    do {                                                                                                         \
        CPM_TEST_SUPPRESS_BEGIN_                                                                                 \
        CPM_TEST_ASSERTION_(kind, negate, macro, #__VA_ARGS__);                                                  \
        try {                                                                                                    \
            cpm_test_assertion_.evaluate(::cpm::test::detail::Decomposer{} <= __VA_ARGS__);                      \
        } catch (const ::cpm::test::detail::Stop&) {                                                             \
            throw;                                                                                               \
        } catch (...) {                                                                                          \
            cpm_test_assertion_.fail_with_exception();                                                           \
        }                                                                                                        \
        cpm_test_assertion_.finish();                                                                            \
        CPM_TEST_SUPPRESS_END_                                                                                   \
    } while (false)

#define CPM_TEST_THROWS_(kind, macro, ...)                                                                       \
    do {                                                                                                         \
        CPM_TEST_ASSERTION_(kind, false, macro, #__VA_ARGS__);                                                   \
        try {                                                                                                    \
            static_cast<void>(__VA_ARGS__);                                                                      \
            cpm_test_assertion_.fail("exception: none");                                                         \
        } catch (const ::cpm::test::detail::Stop&) {                                                             \
            throw;                                                                                               \
        } catch (...) {                                                                                          \
            cpm_test_assertion_.pass();                                                                          \
        }                                                                                                        \
        cpm_test_assertion_.finish();                                                                            \
    } while (false)

#define CPM_TEST_THROWS_AS_(kind, macro, expr, ...)                                                              \
    do {                                                                                                         \
        CPM_TEST_ASSERTION_(kind, false, macro, #expr ", " #__VA_ARGS__);                                        \
        try {                                                                                                    \
            static_cast<void>(expr);                                                                             \
            cpm_test_assertion_.fail("exception: none");                                                         \
        } catch (const ::cpm::test::detail::Stop&) {                                                             \
            throw;                                                                                               \
        } catch (const __VA_ARGS__&) {                                                                           \
            cpm_test_assertion_.pass();                                                                          \
        } catch (...) {                                                                                          \
            cpm_test_assertion_.fail_with_exception("exception of another type: ");                             \
        }                                                                                                        \
        cpm_test_assertion_.finish();                                                                            \
    } while (false)

#define CPM_TEST_THROWS_WITH_(kind, macro, expr, ...)                                                            \
    do {                                                                                                         \
        CPM_TEST_ASSERTION_(kind, false, macro, #expr ", " #__VA_ARGS__);                                        \
        try {                                                                                                    \
            static_cast<void>(expr);                                                                             \
            cpm_test_assertion_.fail("exception: none");                                                         \
        } catch (const ::cpm::test::detail::Stop&) {                                                             \
            throw;                                                                                               \
        } catch (...) {                                                                                          \
            cpm_test_assertion_.check_exception_message(__VA_ARGS__);                                            \
        }                                                                                                        \
        cpm_test_assertion_.finish();                                                                            \
    } while (false)

#define CPM_TEST_NOTHROW_(kind, macro, ...)                                                                      \
    do {                                                                                                         \
        CPM_TEST_ASSERTION_(kind, false, macro, #__VA_ARGS__);                                                   \
        try {                                                                                                    \
            static_cast<void>(__VA_ARGS__);                                                                      \
            cpm_test_assertion_.pass();                                                                          \
        } catch (const ::cpm::test::detail::Stop&) {                                                             \
            throw;                                                                                               \
        } catch (...) {                                                                                          \
            cpm_test_assertion_.fail_with_exception();                                                           \
        }                                                                                                        \
        cpm_test_assertion_.finish();                                                                            \
    } while (false)

#define CPM_TEST_KIND_CHECK_ ::cpm::test::detail::Kind::check
#define CPM_TEST_KIND_REQUIRE_ ::cpm::test::detail::Kind::require

#define CPM_TEST_CHECK(...) CPM_TEST_ASSERT_(CPM_TEST_KIND_CHECK_, false, "CHECK", __VA_ARGS__)
#define CPM_TEST_CHECK_FALSE(...) CPM_TEST_ASSERT_(CPM_TEST_KIND_CHECK_, true, "CHECK_FALSE", __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS(...) CPM_TEST_THROWS_(CPM_TEST_KIND_CHECK_, "CHECK_THROWS", __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS_AS(expr, ...) \
    CPM_TEST_THROWS_AS_(CPM_TEST_KIND_CHECK_, "CHECK_THROWS_AS", expr, __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS_WITH(expr, ...) \
    CPM_TEST_THROWS_WITH_(CPM_TEST_KIND_CHECK_, "CHECK_THROWS_WITH", expr, __VA_ARGS__)
#define CPM_TEST_CHECK_NOTHROW(...) CPM_TEST_NOTHROW_(CPM_TEST_KIND_CHECK_, "CHECK_NOTHROW", __VA_ARGS__)

#define CPM_TEST_REQUIRE(...) CPM_TEST_ASSERT_(CPM_TEST_KIND_REQUIRE_, false, "REQUIRE", __VA_ARGS__)
#define CPM_TEST_REQUIRE_FALSE(...) CPM_TEST_ASSERT_(CPM_TEST_KIND_REQUIRE_, true, "REQUIRE_FALSE", __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS(...) CPM_TEST_THROWS_(CPM_TEST_KIND_REQUIRE_, "REQUIRE_THROWS", __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS_AS(expr, ...) \
    CPM_TEST_THROWS_AS_(CPM_TEST_KIND_REQUIRE_, "REQUIRE_THROWS_AS", expr, __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS_WITH(expr, ...) \
    CPM_TEST_THROWS_WITH_(CPM_TEST_KIND_REQUIRE_, "REQUIRE_THROWS_WITH", expr, __VA_ARGS__)
#define CPM_TEST_REQUIRE_NOTHROW(...) CPM_TEST_NOTHROW_(CPM_TEST_KIND_REQUIRE_, "REQUIRE_NOTHROW", __VA_ARGS__)

// ---- Messages

// INFO("i = {}", i): std::format arguments, formatted only when a failure needs them (so they show the values at the
// time of the failure). CAPTURE(x) is INFO("x = {}", x) for anything to_string prints.
#define CPM_TEST_INFO(...) \
    const ::cpm::test::detail::ScopedInfo CPM_TEST_UNIQUE_(cpm_test_info_)([&] { return ::std::format(__VA_ARGS__); })
#define CPM_TEST_CAPTURE(x)                                                \
    const ::cpm::test::detail::ScopedInfo CPM_TEST_UNIQUE_(cpm_test_info_)( \
        [&] { return ::std::string(#x " = ") + ::cpm::test::to_string(x); })

// FAIL ends the test case, FAIL_CHECK carries on; both take std::format arguments. SKIP ends the test case and counts
// it as skipped, unless it already failed.
#define CPM_TEST_FAIL(...) ::cpm::test::detail::fail_and_stop(__FILE__, __LINE__, ::std::format(__VA_ARGS__))
#define CPM_TEST_FAIL_CHECK(...) ::cpm::test::detail::fail_and_continue(__FILE__, __LINE__, ::std::format(__VA_ARGS__))
#define CPM_TEST_SKIP(...) ::cpm::test::detail::skip(__FILE__, __LINE__, ::std::format(__VA_ARGS__))

// ---- Short names

#ifndef CPM_TEST_NO_SHORT_MACROS
#define TEST_CASE(name) CPM_TEST_CASE(name)
#define SUBCASE(name) CPM_TEST_SUBCASE(name)
#define CHECK(...) CPM_TEST_CHECK(__VA_ARGS__)
#define CHECK_FALSE(...) CPM_TEST_CHECK_FALSE(__VA_ARGS__)
#define CHECK_THROWS(...) CPM_TEST_CHECK_THROWS(__VA_ARGS__)
#define CHECK_THROWS_AS(expr, ...) CPM_TEST_CHECK_THROWS_AS(expr, __VA_ARGS__)
#define CHECK_THROWS_WITH(expr, ...) CPM_TEST_CHECK_THROWS_WITH(expr, __VA_ARGS__)
#define CHECK_NOTHROW(...) CPM_TEST_CHECK_NOTHROW(__VA_ARGS__)
#define REQUIRE(...) CPM_TEST_REQUIRE(__VA_ARGS__)
#define REQUIRE_FALSE(...) CPM_TEST_REQUIRE_FALSE(__VA_ARGS__)
#define REQUIRE_THROWS(...) CPM_TEST_REQUIRE_THROWS(__VA_ARGS__)
#define REQUIRE_THROWS_AS(expr, ...) CPM_TEST_REQUIRE_THROWS_AS(expr, __VA_ARGS__)
#define REQUIRE_THROWS_WITH(expr, ...) CPM_TEST_REQUIRE_THROWS_WITH(expr, __VA_ARGS__)
#define REQUIRE_NOTHROW(...) CPM_TEST_REQUIRE_NOTHROW(__VA_ARGS__)
#define INFO(...) CPM_TEST_INFO(__VA_ARGS__)
#define CAPTURE(x) CPM_TEST_CAPTURE(x)
#define FAIL(...) CPM_TEST_FAIL(__VA_ARGS__)
#define FAIL_CHECK(...) CPM_TEST_FAIL_CHECK(__VA_ARGS__)
#define SKIP(...) CPM_TEST_SKIP(__VA_ARGS__)
#endif
