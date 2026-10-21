// The subcase bookkeeping on its own, driven pass by pass the way a test case would.
#include <subcase_tracker.hpp>

#include <cpm/test.hpp>

#include <string>
#include <vector>

using cpm::test::Location;
using cpm::test::detail::SubcaseTracker;
using Strings = std::vector<std::string>;

namespace {

const Location here{"t.cpp", 1};
const Location there{"t.cpp", 2};

}  // namespace

TEST_CASE("a test case without subcases needs one pass") {
    SubcaseTracker tracker;
    tracker.start_pass();
    CHECK_FALSE(tracker.needs_another_pass(false));
    CHECK_FALSE(tracker.needs_another_pass(true));
}

TEST_CASE("siblings run in separate passes, in order") {
    SubcaseTracker tracker;

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    CHECK(tracker.path() == Strings{"a"});
    tracker.leave(false);
    CHECK_FALSE(tracker.enter("b", there));
    CHECK(tracker.needs_another_pass(false));

    tracker.start_pass();
    CHECK_FALSE(tracker.enter("a", here));
    REQUIRE(tracker.enter("b", there));
    tracker.leave(false);
    CHECK_FALSE(tracker.needs_another_pass(false));
}

TEST_CASE("a subcase is finished once its children are") {
    SubcaseTracker tracker;

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    REQUIRE(tracker.enter("x", here));
    CHECK(tracker.path() == Strings{"a", "x"});
    tracker.leave(false);
    CHECK_FALSE(tracker.enter("y", there));
    tracker.leave(false);
    CHECK(tracker.needs_another_pass(false));

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    CHECK_FALSE(tracker.enter("x", here));
    REQUIRE(tracker.enter("y", there));
    tracker.leave(false);
    tracker.leave(false);
    CHECK_FALSE(tracker.needs_another_pass(false));
}

TEST_CASE("subcases with the same name in different places are different subcases") {
    SubcaseTracker tracker;
    tracker.start_pass();
    REQUIRE(tracker.enter("same", here));
    tracker.leave(false);
    CHECK_FALSE(tracker.enter("same", there));
    CHECK(tracker.needs_another_pass(false));
}

TEST_CASE("an exception ending a pass early asks for another pass, for subcases it didn't reach") {
    SubcaseTracker tracker;

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    tracker.leave(true);  // threw inside "a", before "b" was met
    CHECK(tracker.unwound_path() == Strings{"a"});
    CHECK(tracker.needs_another_pass(true));

    tracker.start_pass();
    CHECK(tracker.unwound_path().empty());
    CHECK_FALSE(tracker.enter("a", here));
    REQUIRE(tracker.enter("b", there));
    tracker.leave(false);
    CHECK_FALSE(tracker.needs_another_pass(false));
}

TEST_CASE("after an exception in a child, its parent runs again for the other children") {
    SubcaseTracker tracker;

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    REQUIRE(tracker.enter("x", here));
    tracker.leave(true);
    tracker.leave(true);
    CHECK(tracker.unwound_path() == Strings{"a", "x"});
    CHECK(tracker.needs_another_pass(true));

    tracker.start_pass();
    REQUIRE(tracker.enter("a", here));
    CHECK_FALSE(tracker.enter("x", here));
    REQUIRE(tracker.enter("y", there));
    tracker.leave(false);
    tracker.leave(false);
    CHECK_FALSE(tracker.needs_another_pass(false));
}

TEST_CASE("an exception before any subcase needs no other pass") {
    SubcaseTracker tracker;
    tracker.start_pass();
    CHECK_FALSE(tracker.needs_another_pass(true));
}
