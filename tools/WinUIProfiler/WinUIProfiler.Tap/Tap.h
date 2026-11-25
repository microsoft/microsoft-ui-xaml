#pragma once
#include <windows.h>
#include <atomic>

namespace WinUIProfiler::Tap
{
    inline constexpr GUID ClassId{0xd8e8c12c, 0x6c9f, 0x49f8, {0x81, 0x69, 0xc6, 0x9b, 0xb6, 0xcd, 0xb0, 0x58}};
    inline std::atomic<ULONG> ActiveObjects{0};
    inline std::atomic<ULONG> ActiveServers{0};

    class Handle final
    {
    public:
        Handle() noexcept = default;
        explicit Handle(HANDLE value) noexcept : value(value) {}
        ~Handle() { Reset(); }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        HANDLE Get() const noexcept { return value; }
        void Reset(HANDLE replacement = nullptr) noexcept
        {
            if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value);
            value = replacement;
        }
    private:
        HANDLE value{};
    };
}
