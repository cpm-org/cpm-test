# Changelog

## 0.1.0

The first release.

- `TEST_CASE`, with test programs getting `main()` from the library.
- `CHECK` and `REQUIRE`, with `_FALSE`, `_THROWS`, `_THROWS_AS`, `_THROWS_WITH` and `_NOTHROW` forms. Comparisons show both values when they fail.
- `SUBCASE`, nested and with names made at run time.
- `INFO`, `CAPTURE`, `FAIL`, `FAIL_CHECK` and `SKIP`.
- `approx` for floating-point numbers.
- Values printed through `std::formatter` or `operator<<`, with ranges, pairs, tuples, optionals and enums built in.
- Test programs that choose test cases by pattern, list them, and exit with 77 when nothing passed, as `cpm test` expects.
