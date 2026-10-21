#include <consumer/add.hpp>

#include <cpm/test.hpp>

TEST_CASE("add") {
    CHECK(add(2, 2) == 4);
    SUBCASE("negative numbers") { CHECK(add(-2, -3) == -5); }
    SUBCASE("zero") { CHECK(add(0, 7) == 7); }
}
