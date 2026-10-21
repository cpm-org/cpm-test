// Comparing floating-point numbers.
#pragma once

#include <cpm/test/to_string.hpp>

#include <cmath>
#include <format>
#include <limits>
#include <string>
#include <type_traits>

namespace cpm::test {

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
        const double distance = std::fabs(x - value_);
        return distance <= margin_ || distance <= epsilon_ * std::fmax(std::fabs(x), std::fabs(value_));
    }

    double value_;
    double epsilon_ = std::numeric_limits<float>::epsilon() * 100;
    double margin_ = 0.0;
};

template <>
struct Printer<approx> {
    static std::string print(const approx& a) { return std::format("approx({})", a.value()); }
};

}  // namespace cpm::test
