// cpm-test: a unit-test framework for C++20. See README.md for a guide.
//
//   #include <cpm/test.hpp>
//
//   TEST_CASE("addition") {
//       CHECK(1 + 1 == 2);          // on failure prints the values: 1 + 1 == 3 shows "values: 2 == 3"
//       SUBCASE("negative") {       // each subcase runs in its own pass through the test case
//           REQUIRE(-1 + -1 == -2); // REQUIRE stops the test case when it fails, CHECK carries on
//       }
//   }
//
// Test programs get main() from the library. Define CPM_TEST_NO_SHORT_MACROS to keep only the CPM_TEST_ names.
#pragma once

#include <cmath>
#include <cstddef>
#include <exception>
#include <format>
#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <ostream>
#include <source_location>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace cpm::test {

// ---- Test cases and running them

struct TestCase {
    std::string name;
    std::string file;
    int line = 0;
    std::function<void()> body;
};

// An ordered set of test cases. TEST_CASE adds to registry(); tests of test tools can build their own.
class Registry {
public:
    void add(TestCase test);
    void add(std::string name, std::function<void()> body,
             std::source_location where = std::source_location::current());
    const std::vector<TestCase>& cases() const noexcept { return cases_; }

private:
    std::vector<TestCase> cases_;
};

// The registry TEST_CASE adds to.
Registry& registry();

struct Options {
    // Run the test cases whose names match one of these patterns, or all of them when there are none. `*` matches
    // any text, and the rest of a pattern matches itself.
    std::vector<std::string> include;
    // Leave out the test cases whose names match one of these.
    std::vector<std::string> exclude;
    // Print a line for every test case, not only for failures.
    bool verbose = false;
    // Stop after the first test case that fails.
    bool fail_fast = false;
};

struct Summary {
    int passed = 0;   // test cases
    int failed = 0;
    int skipped = 0;
    long long checks_passed = 0;
    long long checks_failed = 0;

    // 1 when something failed, 77 (skipped, as `cpm test` reads it) when nothing passed, otherwise 0.
    int exit_code() const noexcept { return failed > 0 ? 1 : passed == 0 ? 77 : 0; }
};

// Runs the selected test cases of `registry`, writing failures and a summary line to `out`.
Summary run(const Registry& registry, const Options& options, std::ostream& out);

// Runs `registry` as a test program would with `args` (the arguments after the program name; --help lists them).
// Returns the exit code: 0, 1 when a test case failed, 2 for a bad command line, 77 when nothing passed.
int main(const Registry& registry, const std::vector<std::string>& args, std::ostream& out);

// What the library's main() calls: registry(), the command line and std::cout. A test program that defines its own
// main() can call it after its own setup.
int main(int argc, char* argv[]);

// ---- Floating-point comparison

// Compares equal to numbers within a relative `epsilon` (default: 100 float epsilons, about 1.2e-5) or an absolute
// `margin` (default 0) of its value: CHECK(0.1 + 0.2 == approx(0.3)), CHECK(x == approx(0.0).margin(1e-9)).
class approx {
public:
    explicit approx(double value) noexcept : value_(value) {}

    approx epsilon(double relative) const noexcept {
        approx copy = *this;
        copy.epsilon_ = relative;
        return copy;
    }
    approx margin(double absolute) const noexcept {
        approx copy = *this;
        copy.margin_ = absolute;
        return copy;
    }
    double value() const noexcept { return value_; }

    template <class T>
        requires std::is_arithmetic_v<T>
    friend bool operator==(T number, const approx& a) noexcept {
        return a.matches(static_cast<double>(number));
    }

private:
    bool matches(double x) const noexcept {
        if (x == value_) return true;                           // also equal infinities
        if (std::isinf(x) || std::isinf(value_)) return false;  // or any tolerance would be infinite
        const double distance = x > value_ ? x - value_ : value_ - x;
        const double ax = x < 0 ? -x : x;
        const double av = value_ < 0 ? -value_ : value_;
        return distance <= margin_ || distance <= epsilon_ * (ax > av ? ax : av);
    }

    double value_;
    double epsilon_ = std::numeric_limits<float>::epsilon() * 100;
    double margin_ = 0.0;
};

// ---- Printing values

// How failures show a value: bool, chars and strings literally (quoted); then std::formatter, operator<<, ranges as
// [a, b], pairs and tuples as (a, b), optionals, enums as their number; "{?}" for anything else. To print your own
// type, specialise std::formatter for it or give it an operator<<.
template <class T>
std::string to_string(const T& value);

namespace detail {

std::string quote(std::string_view text);
std::string quote(char c);

template <class T>
concept formattable = std::is_default_constructible_v<std::formatter<std::remove_cvref_t<T>, char>>;

template <class T>
concept streamable = requires(std::ostream& out, const T& value) { out << value; };

template <class T>
concept iterable = requires(const T& value) {
    std::begin(value);
    std::end(value);
};

template <class T>
concept tuple_like = requires { std::tuple_size<T>::value; };

template <class T>
inline constexpr bool is_optional = false;
template <class T>
inline constexpr bool is_optional<std::optional<T>> = true;

inline constexpr std::size_t max_elements = 100;

template <class R>
std::string join(const R& range) {
    std::string out = "[";
    std::size_t shown = 0;
    std::size_t total = 0;
    for (const auto& element : range) {
        if (total++ < max_elements) {
            if (shown++ > 0) out += ", ";
            out += cpm::test::to_string(element);
        }
    }
    if (total > shown) out += std::format(", ... ({} more)", total - shown);
    out += ']';
    return out;
}

template <class T, std::size_t... I>
std::string join_tuple(const T& value, std::index_sequence<I...>) {
    std::string out = "(";
    ((out += (I == 0 ? "" : ", "), out += cpm::test::to_string(std::get<I>(value))), ...);
    out += ')';
    return out;
}

}  // namespace detail

template <class T>
std::string to_string(const T& value) {
    using U = std::remove_cvref_t<T>;
    if constexpr (std::is_same_v<U, bool>) {
        return value ? "true" : "false";
    } else if constexpr (std::is_same_v<U, std::nullptr_t>) {
        return "nullptr";
    } else if constexpr (std::is_same_v<U, std::nullopt_t>) {
        return "nullopt";
    } else if constexpr (std::is_same_v<U, char>) {
        return detail::quote(value);
    } else if constexpr (std::is_same_v<U, signed char> || std::is_same_v<U, unsigned char>) {
        return std::to_string(static_cast<int>(value));
    } else if constexpr (std::is_array_v<U> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<U>>, char>) {
        // A char array holds a string up to its first '\0', or fills the whole array.
        const std::string_view text(value, std::extent_v<U>);
        return detail::quote(text.substr(0, text.find('\0')));
    } else if constexpr (std::is_array_v<U>) {
        return detail::join(value);  // before operator<<, which would print the address it decays to
    } else if constexpr (std::is_same_v<U, const char*> || std::is_same_v<U, char*>) {
        return value ? detail::quote(std::string_view(value)) : "nullptr";
    } else if constexpr (std::is_convertible_v<const U&, std::string_view>) {
        return detail::quote(std::string_view(value));
    } else if constexpr (std::is_same_v<U, approx>) {
        return std::format("approx({})", value.value());
    } else if constexpr (std::is_pointer_v<U>) {
        if (value == nullptr) return "nullptr";
        if constexpr (std::is_function_v<std::remove_pointer_t<U>>) {
            return "(function pointer)";
        } else {
            return std::format("{}", const_cast<const void*>(static_cast<const volatile void*>(value)));
        }
    } else if constexpr (detail::is_optional<U>) {
        return value ? cpm::test::to_string(*value) : "nullopt";
    } else if constexpr (detail::formattable<U>) {
        return std::format("{}", value);
    } else if constexpr (detail::streamable<U>) {
        std::ostringstream out;
        out << value;
        return std::move(out).str();
    } else if constexpr (std::is_enum_v<U>) {
        return std::to_string(static_cast<std::underlying_type_t<U>>(value));
    } else if constexpr (detail::iterable<U>) {
        return detail::join(value);
    } else if constexpr (detail::tuple_like<U>) {
        return detail::join_tuple(value, std::make_index_sequence<std::tuple_size_v<U>>{});
    } else {
        return "{?}";
    }
}

// ---- What the macros expand to. Not for direct use; names and signatures may change in any release.

namespace detail {

// Thrown to end a test case early: after a failed REQUIRE or FAIL (already reported), or by SKIP. Not derived from
// std::exception, so `catch (const std::exception&)` in tested code lets it through; `catch (...)` would not.
struct Stop {
    bool skipped = false;
};

template <class T>
inline constexpr bool always_false = false;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4018 4389)
#endif

// CHECK(a == b) expands to `Decomposer{} <= a == b`, which groups as `(Decomposer{} <= a) == b`: the comparison
// runs once, as written, and the operands stay available to print if it fails.
template <class L, class R>
class Binary {
public:
    Binary(const L& lhs, std::string_view op, const R& rhs, bool result) noexcept
        : lhs_(lhs), rhs_(rhs), op_(op), result_(result) {}

    bool passed() const noexcept { return result_; }
    std::string expand() const {
        return std::format("{} {} {}", cpm::test::to_string(lhs_), op_, cpm::test::to_string(rhs_));
    }

    template <class T>
    friend void operator&&(const Binary&, const T&) {
        static_assert(always_false<T>, "CHECK(a == b && c) can't show its values: write CHECK((a == b && c)), or two CHECKs");
    }
    template <class T>
    friend void operator||(const Binary&, const T&) {
        static_assert(always_false<T>, "CHECK(a == b || c) can't show its values: write CHECK((a == b || c))");
    }

private:
    const L& lhs_;
    const R& rhs_;
    std::string_view op_;
    bool result_;
};

template <class L>
class Lhs {
public:
    explicit Lhs(const L& lhs) noexcept : lhs_(lhs) {}

    bool passed() const { return static_cast<bool>(lhs_); }
    std::string expand() const { return cpm::test::to_string(lhs_); }

    template <class R>
    Binary<L, R> operator==(const R& rhs) const { return {lhs_, "==", rhs, static_cast<bool>(lhs_ == rhs)}; }
    template <class R>
    Binary<L, R> operator!=(const R& rhs) const { return {lhs_, "!=", rhs, static_cast<bool>(lhs_ != rhs)}; }
    template <class R>
    Binary<L, R> operator<(const R& rhs) const { return {lhs_, "<", rhs, static_cast<bool>(lhs_ < rhs)}; }
    template <class R>
    Binary<L, R> operator<=(const R& rhs) const { return {lhs_, "<=", rhs, static_cast<bool>(lhs_ <= rhs)}; }
    template <class R>
    Binary<L, R> operator>(const R& rhs) const { return {lhs_, ">", rhs, static_cast<bool>(lhs_ > rhs)}; }
    template <class R>
    Binary<L, R> operator>=(const R& rhs) const { return {lhs_, ">=", rhs, static_cast<bool>(lhs_ >= rhs)}; }
    template <class R>
    Binary<L, R> operator&(const R& rhs) const { return {lhs_, "&", rhs, static_cast<bool>(lhs_ & rhs)}; }
    template <class R>
    Binary<L, R> operator|(const R& rhs) const { return {lhs_, "|", rhs, static_cast<bool>(lhs_ | rhs)}; }
    template <class R>
    Binary<L, R> operator^(const R& rhs) const { return {lhs_, "^", rhs, static_cast<bool>(lhs_ ^ rhs)}; }

    template <class T>
    friend void operator&&(const Lhs&, const T&) {
        static_assert(always_false<T>, "CHECK(a && b) can't show its values: write CHECK((a && b)), or two CHECKs");
    }
    template <class T>
    friend void operator||(const Lhs&, const T&) {
        static_assert(always_false<T>, "CHECK(a || b) can't show its values: write CHECK((a || b))");
    }

private:
    const L& lhs_;
};

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

struct Decomposer {
    template <class L>
    friend Lhs<L> operator<=(Decomposer, const L& lhs) noexcept {
        return Lhs<L>(lhs);
    }
};

enum class Kind { check, require };

// One CHECK or REQUIRE: built before the expression runs, told how it went, then finish() reports it.
class Assertion {
public:
    Assertion(Kind kind, bool negate, const char* macro, const char* expression, const char* file, int line) noexcept
        : kind_(kind), negate_(negate), macro_(macro), expression_(expression), file_(file), line_(line) {}

    template <class E>
    void evaluate(const E& expression) {
        if (expression.passed() != negate_) {
            passed_ = true;
        } else {
            passed_ = false;
            detail_ = "values: " + expression.expand();
        }
    }
    void pass() noexcept { passed_ = true; }
    void fail(std::string detail) {
        passed_ = false;
        detail_ = std::move(detail);
    }
    // For a catch (...) block: fails, describing the exception being handled.
    void fail_with_exception(std::string_view prefix = "exception: ");
    // For a catch (...) block of CHECK_THROWS_WITH: passes if the exception's message is `expected`.
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

// The message of the exception being handled: what() of a std::exception, a thrown std::string or C string, or
// nullopt for anything else.
std::optional<std::string> current_exception_message();

bool add_test_case(const char* name, void (*body)(), const char* file, int line);

class Subcase {
public:
    Subcase(std::string_view name, const char* file, int line);
    ~Subcase();
    Subcase(const Subcase&) = delete;
    Subcase& operator=(const Subcase&) = delete;

    explicit operator bool() const noexcept { return entered_; }

private:
    bool entered_;
    int uncaught_;
};

// INFO and CAPTURE messages in scope on this thread. Each is formatted only if a check fails while it is in scope.
struct InfoEntry {
    std::string (*message)(const void* context);
    const void* context;
};
void push_info(const InfoEntry* entry);
void pop_info(const InfoEntry* entry, bool unwinding) noexcept;

template <class F>
class ScopedInfo {
public:
    explicit ScopedInfo(F make) : make_(std::move(make)), entry_{&call, this}, uncaught_(std::uncaught_exceptions()) {
        push_info(&entry_);
    }
    ~ScopedInfo() { pop_info(&entry_, std::uncaught_exceptions() > uncaught_); }
    ScopedInfo(const ScopedInfo&) = delete;
    ScopedInfo& operator=(const ScopedInfo&) = delete;

private:
    static std::string call(const void* self) { return static_cast<const ScopedInfo*>(self)->make_(); }

    F make_;
    InfoEntry entry_;
    int uncaught_;
};

[[noreturn]] void fail_and_stop(const char* file, int line, std::string message);
void fail_and_continue(const char* file, int line, std::string message);
[[noreturn]] void skip(const char* file, int line, std::string reason);

}  // namespace detail
}  // namespace cpm::test

// ---- Macros

#define CPM_TEST_CAT2_(a, b) a##b
#define CPM_TEST_CAT_(a, b) CPM_TEST_CAT2_(a, b)
#define CPM_TEST_UNIQUE_(prefix) CPM_TEST_CAT_(prefix, __COUNTER__)

// `Decomposer{} <= a == b` is a chained comparison as far as -Wparentheses can tell.
#if defined(__GNUC__)
#define CPM_TEST_SUPPRESS_BEGIN_ _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wparentheses\"")
#define CPM_TEST_SUPPRESS_END_ _Pragma("GCC diagnostic pop")
#elif defined(_MSC_VER)
#define CPM_TEST_SUPPRESS_BEGIN_ __pragma(warning(push)) __pragma(warning(disable : 4127))
#define CPM_TEST_SUPPRESS_END_ __pragma(warning(pop))
#else
#define CPM_TEST_SUPPRESS_BEGIN_
#define CPM_TEST_SUPPRESS_END_
#endif

#define CPM_TEST_CASE(name) CPM_TEST_CASE_IMPL_(CPM_TEST_UNIQUE_(cpm_test_case_), name)
#define CPM_TEST_CASE_IMPL_(fn, name)                                                                            \
    static void fn();                                                                                            \
    [[maybe_unused]] static const bool CPM_TEST_CAT_(fn, _registered) =                                          \
        ::cpm::test::detail::add_test_case(name, &fn, __FILE__, __LINE__);                                       \
    static void fn()

#define CPM_TEST_SUBCASE(name) CPM_TEST_SUBCASE_IMPL_(CPM_TEST_UNIQUE_(cpm_test_subcase_), name)
#define CPM_TEST_SUBCASE_IMPL_(var, name) if (const ::cpm::test::detail::Subcase var{name, __FILE__, __LINE__}; var)

#define CPM_TEST_ASSERT_(kind, negate, macro, ...)                                                               \
    do {                                                                                                         \
        CPM_TEST_SUPPRESS_BEGIN_                                                                                 \
        ::cpm::test::detail::Assertion cpm_test_assertion_(kind, negate, macro, #__VA_ARGS__, __FILE__, __LINE__); \
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
        ::cpm::test::detail::Assertion cpm_test_assertion_(kind, false, macro, #__VA_ARGS__, __FILE__, __LINE__); \
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
        ::cpm::test::detail::Assertion cpm_test_assertion_(kind, false, macro, #expr ", " #__VA_ARGS__, __FILE__, \
                                                           __LINE__);                                            \
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
        ::cpm::test::detail::Assertion cpm_test_assertion_(kind, false, macro, #expr ", " #__VA_ARGS__, __FILE__, \
                                                           __LINE__);                                            \
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
        ::cpm::test::detail::Assertion cpm_test_assertion_(kind, false, macro, #__VA_ARGS__, __FILE__, __LINE__); \
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

#define CPM_TEST_CHECK(...) CPM_TEST_ASSERT_(::cpm::test::detail::Kind::check, false, "CHECK", __VA_ARGS__)
#define CPM_TEST_CHECK_FALSE(...) CPM_TEST_ASSERT_(::cpm::test::detail::Kind::check, true, "CHECK_FALSE", __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS(...) CPM_TEST_THROWS_(::cpm::test::detail::Kind::check, "CHECK_THROWS", __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS_AS(expr, ...) \
    CPM_TEST_THROWS_AS_(::cpm::test::detail::Kind::check, "CHECK_THROWS_AS", expr, __VA_ARGS__)
#define CPM_TEST_CHECK_THROWS_WITH(expr, ...) \
    CPM_TEST_THROWS_WITH_(::cpm::test::detail::Kind::check, "CHECK_THROWS_WITH", expr, __VA_ARGS__)
#define CPM_TEST_CHECK_NOTHROW(...) CPM_TEST_NOTHROW_(::cpm::test::detail::Kind::check, "CHECK_NOTHROW", __VA_ARGS__)

#define CPM_TEST_REQUIRE(...) CPM_TEST_ASSERT_(::cpm::test::detail::Kind::require, false, "REQUIRE", __VA_ARGS__)
#define CPM_TEST_REQUIRE_FALSE(...) \
    CPM_TEST_ASSERT_(::cpm::test::detail::Kind::require, true, "REQUIRE_FALSE", __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS(...) CPM_TEST_THROWS_(::cpm::test::detail::Kind::require, "REQUIRE_THROWS", __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS_AS(expr, ...) \
    CPM_TEST_THROWS_AS_(::cpm::test::detail::Kind::require, "REQUIRE_THROWS_AS", expr, __VA_ARGS__)
#define CPM_TEST_REQUIRE_THROWS_WITH(expr, ...) \
    CPM_TEST_THROWS_WITH_(::cpm::test::detail::Kind::require, "REQUIRE_THROWS_WITH", expr, __VA_ARGS__)
#define CPM_TEST_REQUIRE_NOTHROW(...) \
    CPM_TEST_NOTHROW_(::cpm::test::detail::Kind::require, "REQUIRE_NOTHROW", __VA_ARGS__)

// INFO("i = {}", i): std::format arguments, formatted only when a check fails while the INFO is in scope (so it
// shows the values at that moment). CAPTURE(x) is INFO("x = {}", x) for any printable x.
#define CPM_TEST_INFO(...) \
    const ::cpm::test::detail::ScopedInfo CPM_TEST_UNIQUE_(cpm_test_info_)([&] { return ::std::format(__VA_ARGS__); })
#define CPM_TEST_CAPTURE(x)                                                                                      \
    const ::cpm::test::detail::ScopedInfo CPM_TEST_UNIQUE_(cpm_test_info_)(                                      \
        [&] { return ::std::string(#x " = ") + ::cpm::test::to_string(x); })

// FAIL(...) and FAIL_CHECK(...) fail with a std::format message; FAIL ends the test case, FAIL_CHECK carries on.
// SKIP(...) ends the test case and counts it as skipped, unless it already failed.
#define CPM_TEST_FAIL(...) ::cpm::test::detail::fail_and_stop(__FILE__, __LINE__, ::std::format(__VA_ARGS__))
#define CPM_TEST_FAIL_CHECK(...) ::cpm::test::detail::fail_and_continue(__FILE__, __LINE__, ::std::format(__VA_ARGS__))
#define CPM_TEST_SKIP(...) ::cpm::test::detail::skip(__FILE__, __LINE__, ::std::format(__VA_ARGS__))

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
