#include "test_case_execution.hpp"

#include "exceptions.hpp"
#include "info_stack.hpp"

#include <cpm/test/detail/assertion.hpp>

#include <exception>
#include <format>
#include <utility>

namespace cpm::test::detail {

TestCaseExecution::TestCaseExecution(const TestCase& test, Reporter& reporter, CheckCounter& checks) noexcept
    : test_(test), reporter_(reporter), checks_(checks) {}

Outcome TestCaseExecution::run() {
    const ContextScope scope(*this);
    PassEnd end = PassEnd::normally;
    for (int pass = 1;; ++pass) {
        end = run_pass();
        if (end == PassEnd::by_skip) break;
        bool more = false;
        {
            const std::lock_guard lock(mutex_);
            more = subcases_.needs_another_pass(end == PassEnd::by_exception);
        }
        if (!more) break;
        if (pass == max_passes) {
            report_too_many_passes();
            break;
        }
    }
    const std::lock_guard lock(mutex_);
    if (failed_) return Outcome::failed;
    return end == PassEnd::by_skip ? Outcome::skipped : Outcome::passed;
}

TestCaseExecution::PassEnd TestCaseExecution::run_pass() {
    {
        const std::lock_guard lock(mutex_);
        subcases_.start_pass();
    }
    forget_unwound_info();
    PassEnd end = PassEnd::normally;
    try {
        test_.body();
    } catch (const Stop& stop) {
        end = stop.skipped ? PassEnd::by_skip : PassEnd::by_exception;
    } catch (...) {
        end = PassEnd::by_exception;
        report_escaped_exception();
    }
    forget_unwound_info();
    return end;
}

// A check running normally means any exception thrown before it was caught, so the INFOs it left no longer apply.
void TestCaseExecution::forget_caught_exceptions() noexcept {
    if (std::uncaught_exceptions() == 0) forget_unwound_info();
}

void TestCaseExecution::check_passed() {
    forget_caught_exceptions();
    checks_.passed.fetch_add(1, std::memory_order_relaxed);
}

void TestCaseExecution::check_failed(Location where, std::string message, std::string detail) {
    forget_caught_exceptions();
    checks_.failed.fetch_add(1, std::memory_order_relaxed);
    // INFO messages are user code, so they are formatted before taking the lock.
    std::vector<std::string> info = info_messages();
    const std::lock_guard lock(mutex_);
    record(Failure{std::move(where), std::move(message), std::move(detail), path_to(subcases_.path()),
                   std::move(info)});
}

void TestCaseExecution::skipped(Location where, std::string reason) {
    const std::lock_guard lock(mutex_);
    reporter_.skipped(Skip{std::move(where), std::move(reason), path_to(subcases_.path())});
}

bool TestCaseExecution::enter_subcase(std::string_view name, Location where) {
    const std::lock_guard lock(mutex_);
    return subcases_.enter(name, where);
}

void TestCaseExecution::leave_subcase(bool unwinding) {
    const std::lock_guard lock(mutex_);
    subcases_.leave(unwinding);
}

void TestCaseExecution::report_escaped_exception() {
    std::string detail = "exception: " + describe_current_exception();
    std::vector<std::string> info = unwound_info_messages();
    const std::lock_guard lock(mutex_);
    record(Failure{Location{test_.file, test_.line}, "unexpected exception", std::move(detail),
                   path_to(subcases_.unwound_path()), std::move(info)});
}

void TestCaseExecution::report_too_many_passes() {
    const std::lock_guard lock(mutex_);
    record(Failure{Location{test_.file, test_.line},
                   std::format("subcases still unfinished after {} passes", max_passes),
                   "hint: subcase names and their order must be the same in every pass", path_to({}), {}});
}

void TestCaseExecution::record(const Failure& failure) {
    failed_ = true;
    reporter_.failure(failure);
}

std::vector<std::string> TestCaseExecution::path_to(const std::vector<std::string>& subcases) const {
    std::vector<std::string> path{test_.name};
    path.insert(path.end(), subcases.begin(), subcases.end());
    return path;
}

}  // namespace cpm::test::detail
