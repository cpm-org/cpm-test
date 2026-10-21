// What INFO and CAPTURE expand to. Not for direct use; may change in any release.
#pragma once

#include <exception>
#include <string>
#include <utility>

namespace cpm::test::detail {

// A message in scope on this thread, formatted only if a failure needs it.
struct InfoEntry {
    std::string (*message)(const void* context);
    const void* context;
};

void push_info(const InfoEntry* entry);
void pop_info(const InfoEntry* entry, bool unwinding) noexcept;

template <class F>
class ScopedInfo {
public:
    explicit ScopedInfo(F make) : make_(std::move(make)), entry_{&call, this}, uncaught_(std::uncaught_exceptions()) {
        push_info(&entry_);
    }
    ~ScopedInfo() { pop_info(&entry_, std::uncaught_exceptions() > uncaught_); }
    ScopedInfo(const ScopedInfo&) = delete;
    ScopedInfo& operator=(const ScopedInfo&) = delete;

private:
    static std::string call(const void* self) { return static_cast<const ScopedInfo*>(self)->make_(); }

    F make_;
    InfoEntry entry_;
    int uncaught_;
};

}  // namespace cpm::test::detail
