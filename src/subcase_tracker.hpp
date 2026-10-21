// Which subcases each pass through a test case enters.
#pragma once

#include <cpm/test/results.hpp>

#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::test::detail {

// Each pass enters at most one unfinished subcase at each depth; passes go on until every subcase has run. Pure
// bookkeeping: it knows nothing of running code, threads or output.
class SubcaseTracker {
public:
    // Starts a pass from the top of the test case.
    void start_pass();

    // Whether this pass enters the subcase met here. Every true must be followed by leave().
    bool enter(std::string_view name, const Location& where);

    // Leaves the innermost entered subcase, normally or because an exception is unwinding through it.
    void leave(bool unwinding);

    // Whether another pass is needed for subcases this one didn't reach.
    bool needs_another_pass(bool pass_threw) const noexcept;

    // The names of the subcases entered and not yet left, outermost first.
    std::vector<std::string> path() const;

    // The path where an exception started unwinding in this pass; empty if none did.
    const std::vector<std::string>& unwound_path() const noexcept { return unwound_path_; }

private:
    struct Frame {
        std::string name;
        Location where;
    };
    // What a pass has done at one depth: whether it entered a subcase there, and whether one there still has to run.
    struct Level {
        bool entered = false;
        bool unfinished = false;
    };

    static std::string key(const std::vector<Frame>& frames);

    std::vector<Frame> frames_;            // the subcases entered and not yet left
    std::vector<Level> levels_{Level{}};   // one per depth: frames_.size() + 1
    std::set<std::string> finished_;       // keys of the subcases that have run completely
    bool entered_any_ = false;
    std::vector<std::string> unwound_path_;
    bool unwinding_seen_ = false;
};

}  // namespace cpm::test::detail
