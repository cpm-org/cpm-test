// Choosing test cases by name.
#pragma once

#include <cpm/test/registry.hpp>
#include <cpm/test/run.hpp>

#include <string>
#include <string_view>
#include <vector>

namespace cpm::test::detail {

// Whether `text` matches `pattern`, in which `*` matches any text and every other character itself.
bool matches(std::string_view pattern, std::string_view text) noexcept;

// The test cases matching an include pattern (all of them when there are none), minus those matching an exclude
// pattern.
class Filter {
public:
    explicit Filter(const Options& options);

    bool selects(std::string_view name) const;
    std::vector<const TestCase*> select(const Registry& registry) const;

private:
    std::vector<std::string> include_;
    std::vector<std::string> exclude_;
};

}  // namespace cpm::test::detail
