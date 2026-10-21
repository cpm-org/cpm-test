// The command line of test programs, and the main() they run.
#pragma once

#include <cpm/test/registry.hpp>
#include <cpm/test/run.hpp>

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace cpm::test {

struct CommandLine {
    enum class Action { run, list, help };

    Action action = Action::run;
    Options options;
    bool verbose = false;
};

// A command line that can't be acted on: what() says what is wrong, hint() what to do about it.
class UsageError : public std::runtime_error {
public:
    UsageError(const std::string& message, std::string hint);
    const std::string& hint() const noexcept { return hint_; }

private:
    std::string hint_;
};

// Reads the arguments after the program name. Throws UsageError.
CommandLine parse_command_line(const std::vector<std::string>& args);

// The --help text.
std::string_view usage() noexcept;

// Acts on `args` for `registry`, writing to `out`. Returns the exit code: 0, 1 when a test case failed, 2 for a bad
// command line, 77 when nothing passed.
int main(const Registry& registry, const std::vector<std::string>& args, std::ostream& out);

// What the library's main() calls: registry(), the command line and std::cout. A test program that defines its own
// main() can call it after its own setup.
int main(int argc, char* argv[]);

}  // namespace cpm::test
