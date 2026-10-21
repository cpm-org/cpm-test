# cpm-test

A unit-test framework for C++20, made for [CPM](https://github.com/meirborowski/CPM). Write test cases with checks that print the values they compared, split them into subcases, and run them with `cpm test`.

```sh
cpm install -D cpm-test
```

```cpp
// tests/vector.test.cpp
#include <cpm/test.hpp>

#include <vector>

TEST_CASE("vector") {
    std::vector<int> v{1, 2, 3};
    REQUIRE(v.size() == 3);

    SUBCASE("push_back adds at the end") {
        v.push_back(4);
        CHECK(v.back() == 5);
    }
    SUBCASE("every element is small") {
        for (std::size_t i = 0; i < v.size(); ++i) {
            INFO("i = {}", i);
            CHECK(v[i] < 3);
        }
    }
}
```

`cpm test` builds each `tests/*.test.cpp` into its own program and runs it. When something fails, the output says where, what, with which values, and in which subcase:

```
tests/vector.test.cpp:11: error: CHECK(v.back() == 5) failed
  values: 4 == 5
  in: "vector" > "push_back adds at the end"
tests/vector.test.cpp:16: error: CHECK(v[i] < 3) failed
  values: 3 < 3
  in: "vector" > "every element is small"
  info: i = 2
test cases: 0 passed, 1 failed; checks: 4 passed, 2 failed
```

Test programs need no `main()`: the library has one, which the linker uses when the program defines none.

## Writing tests

### Test cases

`TEST_CASE("name") { ... }` defines a test case. Test cases run in the order they are written.

### Checks

| Macro | Passes when |
|---|---|
| `CHECK(expr)` | `expr` is true |
| `CHECK_FALSE(expr)` | `expr` is false |
| `CHECK_THROWS(expr)` | `expr` throws anything |
| `CHECK_THROWS_AS(expr, type)` | `expr` throws a `type`, or something derived from it |
| `CHECK_THROWS_WITH(expr, message)` | `expr` throws an exception whose message is `message`: `what()` of a `std::exception`, or a thrown `std::string` or C string |
| `CHECK_NOTHROW(expr)` | `expr` throws nothing |

A failing `CHECK` lets the test case carry on. Each `CHECK` has a `REQUIRE` form (`REQUIRE`, `REQUIRE_FALSE`, `REQUIRE_THROWS` and so on) that ends the test case when it fails. Use it when the rest of the test can't run, for example before using a pointer it checked.

A comparison (`==`, `!=`, `<`, `<=`, `>`, `>=`, or `&`, `|` and `^`) shows both of its values when it fails. It runs exactly as written, once. `&&` and `||` can't be split up like this, so `CHECK(a == 1 && b == 2)` doesn't compile. Write `CHECK((a == 1 && b == 2))` to check it without the values, or use two `CHECK`s.

An exception thrown inside a check fails that check, and the test case carries on.

### Floating-point numbers

`approx` compares within a relative tolerance (by default 100 float epsilons, about 1.2e-5) or an absolute margin:

```cpp
using cpm::test::approx;

CHECK(0.1 + 0.2 == approx(0.3));
CHECK(x == approx(1000.0).epsilon(1e-3));
CHECK(y == approx(0.0).margin(1e-9));  // near zero only a margin helps
```

### Subcases

Each `SUBCASE("name") { ... }` runs in a separate pass through its test case, along with the code around it. In the example above, `v` starts as `{1, 2, 3}` in both subcases. Subcases can nest, and their names can be built at run time. Failures name the path, as in `in: "vector" > "push_back adds at the end"`.

### Messages

- **`INFO("format", args...)`** takes `std::format` arguments. It is shown with every failure while it is in scope, and with an exception that escapes its scope. The message is formatted only when needed, so it shows the values at the time of the failure.
- **`CAPTURE(x)`** is short for `INFO("x = {}", x)`, and works for any value cpm-test can print.
- **`FAIL("format", args...)`** fails and ends the test case. **`FAIL_CHECK(...)`** fails and carries on.
- **`SKIP("format", args...)`** ends the test case and counts it as skipped, unless it had already failed.

### Printing your own types

Failures print values like this:
- `bool`, characters and strings literally, quoted and escaped.
- Anything with a `std::formatter`, or else an `operator<<`.
- Ranges as `[a, b]`, showing at most 100 elements.
- Pairs and tuples as `(a, b)`, and `std::optional` as its value or `nullopt`.
- Enums as their number, unless they have a formatter or an `operator<<`.
- Anything else as `{?}`.

To show your own type, give it a `std::formatter` specialisation or an `operator<<`. To print a value the same way yourself, call `cpm::test::to_string(value)`.

## Running tests

`cpm test` runs every test program. A test program can also run by itself, and takes these arguments:

```
usage: TEST [options] [pattern...]

Runs the test cases whose names match a pattern, or all of them. In a pattern, * matches any
text. A pattern that starts with ~ leaves out the test cases it matches.

Options:
  -l, --list       List the test cases instead of running them
  -v, --verbose    Print a line for every test case, not only for failures
      --fail-fast  Stop after the first test case that fails
  -h, --help       Show this help
```

For example, `build/debug/tests/parser.test "parse *" "~*slow*"` runs the test cases whose names start with `parse `, except the slow ones.

The exit status:
- 0 when nothing failed.
- 1 when a test case failed.
- 2 for a bad command line, including patterns that match nothing.
- 77 when no test case passed, for example because every one was skipped. `cpm test` reports such a program as skipped.

### Your own `main()`

A test program that needs setup can define `main()` itself. It then calls `cpm::test::main(argc, argv)`:

```cpp
int main(int argc, char* argv[]) {
    setup_logging();
    return cpm::test::main(argc, argv);
}
```

To run test cases from code, for example in tests of test tools, build a `cpm::test::Registry` and pass it to `cpm::test::run(registry, options, out)`. It returns the counts as a `cpm::test::Summary`.

## Good to know

- **How REQUIRE stops:** a failed `REQUIRE`, `FAIL` or `SKIP` throws an object that doesn't derive from `std::exception`, so `catch (const std::exception&)` in the code under test lets it through. `catch (...)` would swallow it, and a `noexcept` function would end the program.
- **Threads:** `CHECK` and its forms work from any thread. `REQUIRE`, `SUBCASE` and `INFO` belong on the test case's own thread.
- **Macro names:** each macro also has a `CPM_TEST_` name, such as `CPM_TEST_CHECK`. Define `CPM_TEST_NO_SHORT_MACROS` before including the header to keep only those, if another library already defines `CHECK`.

## Developing

```sh
cpm test                        # the tests, in the debug profile
cpm test --profile strict       # every warning CPM knows, as errors (what CI runs)
cpm test -C examples/consumer   # a project that depends on cpm-test, as a user's would
```

CI runs these on Linux, macOS and Windows, with AddressSanitizer and UBSan on Linux (`--profile sanitize`).

## License

[MIT](LICENSE)
