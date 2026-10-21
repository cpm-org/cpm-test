// How CHECK(a == b) keeps a and b to print: it expands to `Decomposer{} <= a == b`, which groups as
// `(Decomposer{} <= a) == b`. The comparison runs once, as written, inside a std:: function object, so comparing
// values of mixed signedness (such as v.size() == 3) doesn't warn.
#pragma once

#include <cpm/test/to_string.hpp>

#include <format>
#include <functional>
#include <string>
#include <string_view>

namespace cpm::test::detail {

template <class T>
inline constexpr bool always_false = false;

// A comparison that has run, with its operands kept for printing.
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
        static_assert(always_false<T>,
                      "CHECK(a == b && c) can't show its values: write CHECK((a == b && c)), or two CHECKs");
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

// The left operand of a checked expression. Alone it is checked for truth; compared, it makes a Binary.
template <class L>
class Lhs {
public:
    explicit Lhs(const L& lhs) noexcept : lhs_(lhs) {}

    bool passed() const { return static_cast<bool>(lhs_); }
    std::string expand() const { return cpm::test::to_string(lhs_); }

    template <class R>
    Binary<L, R> operator==(const R& rhs) const { return compare(rhs, "==", std::equal_to<>{}); }
    template <class R>
    Binary<L, R> operator!=(const R& rhs) const { return compare(rhs, "!=", std::not_equal_to<>{}); }
    template <class R>
    Binary<L, R> operator<(const R& rhs) const { return compare(rhs, "<", std::less<>{}); }
    template <class R>
    Binary<L, R> operator<=(const R& rhs) const { return compare(rhs, "<=", std::less_equal<>{}); }
    template <class R>
    Binary<L, R> operator>(const R& rhs) const { return compare(rhs, ">", std::greater<>{}); }
    template <class R>
    Binary<L, R> operator>=(const R& rhs) const { return compare(rhs, ">=", std::greater_equal<>{}); }
    template <class R>
    Binary<L, R> operator&(const R& rhs) const { return compare(rhs, "&", std::bit_and<>{}); }
    template <class R>
    Binary<L, R> operator|(const R& rhs) const { return compare(rhs, "|", std::bit_or<>{}); }
    template <class R>
    Binary<L, R> operator^(const R& rhs) const { return compare(rhs, "^", std::bit_xor<>{}); }

    template <class T>
    friend void operator&&(const Lhs&, const T&) {
        static_assert(always_false<T>, "CHECK(a && b) can't show its values: write CHECK((a && b)), or two CHECKs");
    }
    template <class T>
    friend void operator||(const Lhs&, const T&) {
        static_assert(always_false<T>, "CHECK(a || b) can't show its values: write CHECK((a || b))");
    }

private:
    template <class R, class Op>
    Binary<L, R> compare(const R& rhs, std::string_view op, Op apply) const {
        return {lhs_, op, rhs, static_cast<bool>(apply(lhs_, rhs))};
    }

    const L& lhs_;
};

struct Decomposer {
    template <class L>
    friend Lhs<L> operator<=(Decomposer, const L& lhs) noexcept {
        return Lhs<L>(lhs);
    }
};

}  // namespace cpm::test::detail
