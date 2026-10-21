// How failures show values.
#pragma once

#include <concepts>
#include <cstddef>
#include <format>
#include <iterator>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace cpm::test {

// Specialise Printer<T> with `static std::string print(const T&)` to choose how failures show a T. Without one,
// cpm-test uses the type's std::formatter or operator<<, or one of the built-in rules below.
template <class T>
struct Printer;

// A value as failures show it.
template <class T>
std::string to_string(const T& value);

namespace detail::print {

std::string quote(std::string_view text);
std::string quote(char c);

inline constexpr std::size_t max_elements = 100;

template <class T>
concept has_printer = requires(const T& value) {
    { Printer<T>::print(value) } -> std::convertible_to<std::string>;
};

template <class T>
concept formattable = std::is_default_constructible_v<std::formatter<T, char>>;

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
concept char_array = std::is_array_v<T> && std::is_same_v<std::remove_cv_t<std::remove_extent_t<T>>, char>;

template <class T>
inline constexpr bool is_optional = false;
template <class T>
inline constexpr bool is_optional<std::optional<T>> = true;

template <class T, class... U>
concept one_of = (std::is_same_v<T, U> || ...);

// "[a, b, c]", with at most max_elements shown.
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

// "(a, b)"
template <class T, std::size_t... I>
std::string join_tuple(const T& value, std::index_sequence<I...>) {
    std::string out = "(";
    ((out += (I == 0 ? "" : ", "), out += cpm::test::to_string(std::get<I>(value))), ...);
    out += ')';
    return out;
}

// Each rule prints one kind of value. to_string uses the first rule in its list that applies.

struct CustomRule {
    template <class T>
    static constexpr bool applies = has_printer<T>;
    template <class T>
    static std::string print(const T& value) { return Printer<T>::print(value); }
};

struct LiteralRule {
    template <class T>
    static constexpr bool applies = one_of<T, bool, std::nullptr_t, std::nullopt_t>;
    static std::string print(bool value) { return value ? "true" : "false"; }
    static std::string print(std::nullptr_t) { return "nullptr"; }
    static std::string print(std::nullopt_t) { return "nullopt"; }
};

// char quoted; signed and unsigned char, usually small integers, as numbers.
struct CharacterRule {
    template <class T>
    static constexpr bool applies = one_of<T, char, signed char, unsigned char>;
    static std::string print(char value) { return quote(value); }
    static std::string print(signed char value) { return std::to_string(value); }
    static std::string print(unsigned char value) { return std::to_string(value); }
};

// Strings, quoted and escaped.
struct TextRule {
    template <class T>
    static constexpr bool applies =
        char_array<T> || one_of<T, const char*, char*> || std::is_convertible_v<const T&, std::string_view>;

    template <class T>
    static std::string print(const T& value) {
        if constexpr (char_array<T>) {
            // A char array holds a string up to its first '\0', or fills the whole array.
            const std::string_view text(value, std::extent_v<T>);
            return quote(text.substr(0, text.find('\0')));
        } else if constexpr (one_of<T, const char*, char*>) {
            return value != nullptr ? quote(std::string_view(value)) : "nullptr";
        } else {
            return quote(std::string_view(value));
        }
    }
};

// Before the stream rule, which would print the address an array decays to.
struct ArrayRule {
    template <class T>
    static constexpr bool applies = std::is_array_v<T>;
    template <class T>
    static std::string print(const T& value) { return join(value); }
};

struct PointerRule {
    template <class T>
    static constexpr bool applies = std::is_pointer_v<T>;
    template <class T>
    static std::string print(T value) {
        if (value == nullptr) return "nullptr";
        if constexpr (std::is_function_v<std::remove_pointer_t<T>>) {
            return "(function pointer)";
        } else {
            return std::format("{}", const_cast<const void*>(static_cast<const volatile void*>(value)));
        }
    }
};

struct OptionalRule {
    template <class T>
    static constexpr bool applies = is_optional<T>;
    template <class T>
    static std::string print(const T& value) { return value ? cpm::test::to_string(*value) : "nullopt"; }
};

struct FormatterRule {
    template <class T>
    static constexpr bool applies = formattable<T>;
    template <class T>
    static std::string print(const T& value) { return std::format("{}", value); }
};

struct StreamRule {
    template <class T>
    static constexpr bool applies = streamable<T>;
    template <class T>
    static std::string print(const T& value) {
        std::ostringstream out;
        out << value;
        return std::move(out).str();
    }
};

struct EnumRule {
    template <class T>
    static constexpr bool applies = std::is_enum_v<T>;
    template <class T>
    static std::string print(T value) { return std::to_string(static_cast<std::underlying_type_t<T>>(value)); }
};

struct RangeRule {
    template <class T>
    static constexpr bool applies = iterable<T>;
    template <class T>
    static std::string print(const T& value) { return join(value); }
};

struct TupleRule {
    template <class T>
    static constexpr bool applies = tuple_like<T>;
    template <class T>
    static std::string print(const T& value) {
        return join_tuple(value, std::make_index_sequence<std::tuple_size_v<T>>{});
    }
};

struct UnknownRule {
    template <class T>
    static constexpr bool applies = true;
    template <class T>
    static std::string print(const T&) { return "{?}"; }
};

template <class T, class Rule, class... Rest>
std::string print_first(const T& value) {
    if constexpr (Rule::template applies<T>) {
        return Rule::print(value);
    } else {
        return print_first<T, Rest...>(value);
    }
}

}  // namespace detail::print

template <class T>
std::string to_string(const T& value) {
    using namespace detail::print;
    return print_first<std::remove_cvref_t<T>, CustomRule, LiteralRule, CharacterRule, TextRule, ArrayRule,
                       PointerRule, OptionalRule, FormatterRule, StreamRule, EnumRule, RangeRule, TupleRule,
                       UnknownRule>(value);
}

}  // namespace cpm::test
