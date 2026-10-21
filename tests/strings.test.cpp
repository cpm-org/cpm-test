// How failures print values: cpm::test::to_string.
#include <cpm/test.hpp>

#include <format>
#include <map>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using cpm::test::to_string;

namespace {

enum class Color { red, green = 5 };

enum class Named { one = 1 };
std::ostream& operator<<(std::ostream& out, Named) { return out << "Named::one"; }

struct Streamed {
    int x;
};
std::ostream& operator<<(std::ostream& out, const Streamed& s) { return out << "Streamed{" << s.x << '}'; }

struct Formatted {
    int x;
};

struct Opaque {};

int function(int x) { return x; }

}  // namespace

template <>
struct std::formatter<Formatted, char> : std::formatter<std::string_view, char> {
    auto format(const Formatted& f, std::format_context& ctx) const {
        return std::formatter<std::string_view, char>::format(std::format("Formatted<{}>", f.x), ctx);
    }
};

TEST_CASE("basic values") {
    CHECK(to_string(true) == "true");
    CHECK(to_string(false) == "false");
    CHECK(to_string(nullptr) == "nullptr");
    CHECK(to_string(42) == "42");
    CHECK(to_string(-7L) == "-7");
    CHECK(to_string(0.1) == "0.1");
    CHECK(to_string(2.5f) == "2.5");
    CHECK(to_string(static_cast<signed char>(-3)) == "-3");
    CHECK(to_string(static_cast<unsigned char>(200)) == "200");
}

TEST_CASE("characters and strings are quoted and escaped") {
    CHECK(to_string('a') == "'a'");
    CHECK(to_string('\n') == "'\\n'");
    CHECK(to_string('\'') == "'\\''");
    CHECK(to_string("hi") == "\"hi\"");
    CHECK(to_string(std::string("say \"hi\"\n")) == "\"say \\\"hi\\\"\\n\"");
    CHECK(to_string(std::string_view("tab\there")) == "\"tab\\there\"");
    CHECK(to_string(std::string("\x01")) == "\"\\x01\"");
    CHECK(to_string(std::string("caf\xc3\xa9")) == "\"caf\xc3\xa9\"");  // UTF-8 passes through
    const char* null = nullptr;
    CHECK(to_string(null) == "nullptr");
    const char buffer[8] = "abc";
    CHECK(to_string(buffer) == "\"abc\"");
    const char full[3] = {'x', 'y', 'z'};  // no '\0': the whole array
    CHECK(to_string(full) == "\"xyz\"");
}

TEST_CASE("pointers") {
    int x = 0;
    CHECK(to_string(&x).starts_with("0x"));
    int* null = nullptr;
    CHECK(to_string(null) == "nullptr");
    CHECK(to_string(&function) == "(function pointer)");
}

TEST_CASE("enums print their number, unless they have a formatter or operator<<") {
    CHECK(to_string(Color::red) == "0");
    CHECK(to_string(Color::green) == "5");
    CHECK(to_string(Named::one) == "Named::one");
}

TEST_CASE("std::formatter comes before operator<<") {
    CHECK(to_string(Formatted{3}) == "Formatted<3>");
    CHECK(to_string(Streamed{4}) == "Streamed{4}");
    CHECK(to_string(Opaque{}) == "{?}");
}

TEST_CASE("ranges, pairs, tuples and optionals") {
    CHECK(to_string(std::vector<int>{1, 2, 3}) == "[1, 2, 3]");
    CHECK(to_string(std::vector<int>{}) == "[]");
    CHECK(to_string(std::vector<std::string>{"a", "b"}) == "[\"a\", \"b\"]");
    CHECK(to_string(std::vector<std::vector<int>>{{1}, {2, 3}}) == "[[1], [2, 3]]");
    CHECK(to_string(std::map<int, std::string>{{1, "one"}, {2, "two"}}) == "[(1, \"one\"), (2, \"two\")]");
    CHECK(to_string(std::pair<int, char>{1, 'x'}) == "(1, 'x')");
    CHECK(to_string(std::tuple<int, bool, std::string>{1, true, "s"}) == "(1, true, \"s\")");
    CHECK(to_string(std::tuple<>{}) == "()");
    CHECK(to_string(std::optional<int>{5}) == "5");
    CHECK(to_string(std::optional<int>{}) == "nullopt");
    CHECK(to_string(std::nullopt) == "nullopt");
    const int array[] = {7, 8};
    CHECK(to_string(array) == "[7, 8]");
}

TEST_CASE("long ranges are cut short") {
    const std::vector<int> v(250, 1);
    const std::string s = to_string(v);
    CHECK(s.ends_with(", 1, ... (150 more)]"));
    CHECK(s.starts_with("[1, 1, "));
}

TEST_CASE("approx") {
    CHECK(to_string(cpm::test::approx(0.25)) == "approx(0.25)");
}
