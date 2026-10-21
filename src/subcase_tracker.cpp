#include "subcase_tracker.hpp"

#include <format>
#include <utility>

namespace cpm::test::detail {

void SubcaseTracker::start_pass() {
    frames_.clear();
    levels_.assign(1, Level{});
    entered_any_ = false;
    unwound_path_.clear();
    unwinding_seen_ = false;
}

bool SubcaseTracker::enter(std::string_view name, const Location& where) {
    Level& level = levels_.back();
    std::vector<Frame> candidate = frames_;
    candidate.push_back(Frame{std::string(name), where});
    if (finished_.contains(key(candidate))) return false;
    if (level.entered) {
        // A sibling ran in this pass; this one waits for a later pass.
        level.unfinished = true;
        return false;
    }
    level.entered = true;
    frames_ = std::move(candidate);
    levels_.emplace_back();
    entered_any_ = true;
    return true;
}

void SubcaseTracker::leave(bool unwinding) {
    if (unwinding && !unwinding_seen_) {
        unwound_path_ = path();
        unwinding_seen_ = true;
    }
    const Level children = levels_.back();
    // After an exception, a subcase that entered a child may have others the exception kept it from reaching, so it
    // runs again; one that entered none can't get any further.
    const bool done = unwinding ? !children.entered : !children.unfinished;
    if (done) {
        finished_.insert(key(frames_));
    }
    levels_.pop_back();
    frames_.pop_back();
    if (!done) levels_.back().unfinished = true;
}

bool SubcaseTracker::needs_another_pass(bool pass_threw) const noexcept {
    // An exception may have ended the pass before it reached the rest of the subcases. Every pass that enters a
    // subcase finishes at least one, so this ends.
    return levels_.front().unfinished || (pass_threw && entered_any_);
}

std::vector<std::string> SubcaseTracker::path() const {
    std::vector<std::string> names;
    names.reserve(frames_.size());
    for (const Frame& frame : frames_) names.push_back(frame.name);
    return names;
}

std::string SubcaseTracker::key(const std::vector<Frame>& frames) {
    std::string joined;
    for (const Frame& frame : frames) {
        joined += std::format("{}:{}:{}\x1f", frame.where.file, frame.where.line, frame.name);
    }
    return joined;
}

}  // namespace cpm::test::detail
