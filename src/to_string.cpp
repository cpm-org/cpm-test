#include <cpm/test/to_string.hpp>

namespace cpm::test::detail::print {
namespace {

// Escapes what would make a string hard to read or ambiguous: backslashes, the quote, and control characters.
// Other bytes, UTF-8 included, pass through.
void append_escaped(std::string& out, char c, char quote) {
    switch (c) {
    case '\\': out += "\\\\"; return;
    case '\n': out += "\\n"; return;
    case '\r': out += "\\r"; return;
    case '\t': out += "\\t"; return;
    case '\0': out += "\\0"; return;
    default: break;
    }
    if (c == quote) {
        out += '\\';
        out += c;
    } else if (static_cast<unsigned char>(c) < 0x20 || c == '\x7f') {
        out += std::format("\\x{:02x}", static_cast<unsigned>(static_cast<unsigned char>(c)));
    } else {
        out += c;
    }
}

}  // namespace

std::string quote(std::string_view text) {
    std::string out;
    out.reserve(text.size() + 2);
    out += '"';
    for (const char c : text) append_escaped(out, c, '"');
    out += '"';
    return out;
}

std::string quote(char c) {
    std::string out = "'";
    append_escaped(out, c, '\'');
    out += '\'';
    return out;
}

}  // namespace cpm::test::detail::print
