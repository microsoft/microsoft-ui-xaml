#include "../WinUIProfiler.Native/Exports.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cstddef>
#include <initializer_list>
#ifdef WUP_GPU_MEMORY_VERSION
#include "../WinUIProfiler.Native/Metrics/GpuMemoryCore.h"
#endif
static int failures, checks;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; std::printf("FAIL line %d: %s\n", __LINE__, #x); } } while (0)
int main(int argc, char**)
{
    if (argc > 1) return 0;
#ifndef WUP_GPU_MEMORY_VERSION
    CHECK(false && "GPU memory ABI/core not implemented");
#else
    static_assert(sizeof(WupGpuMemoryRequest) == 40);
    static_assert(sizeof(WupGpuMemoryResult) == 152);
    static_assert(offsetof(WupGpuMemoryRequest, process) == 8);
    static_assert(offsetof(WupGpuMemoryRequest, luid_high) == 24);
    static_assert(offsetof(WupGpuMemoryRequest, physical_index) == 32);
    static_assert(offsetof(WupGpuMemoryResult, process) == 16);
    static_assert(offsetof(WupGpuMemoryResult, luid_high) == 32);
    static_assert(offsetof(WupGpuMemoryResult, physical_index) == 40);
    static_assert(offsetof(WupGpuMemoryResult, clock_id) == 48);
    static_assert(offsetof(WupGpuMemoryResult, dedicated_hresult) == 72);
    static_assert(offsetof(WupGpuMemoryResult, shared_hresult) == 76);
    static_assert(offsetof(WupGpuMemoryResult, win32_error) == 80);
    static_assert(offsetof(WupGpuMemoryResult, process_query_flags) == 84);
    static_assert(offsetof(WupGpuMemoryResult, dedicated_committed) == 88);
    static_assert(offsetof(WupGpuMemoryResult, shared_resident) == 136);
    static_assert(sizeof(WupMemoryResult) == 112);
    static_assert(sizeof(WupGpuResult) == 8336);
    FILETIME creation{}, exit{}, kernel{}, user{};
    CHECK(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user));
    const uint64_t token = (uint64_t(creation.dwHighDateTime) << 32) | creation.dwLowDateTime;
    WupGpuMemoryRequest request{sizeof(request), WUP_GPU_MEMORY_VERSION,
        {GetCurrentProcessId(), 0, token}, 0x123, 0x456, 2, 0};
    struct Guard { uint64_t before; WupGpuMemoryResult value; uint64_t after; };
    Guard guard{11, {}, 22};
    wup::GpuMemoryFamily dedicated{0, 1, 0, 0}, shared{0, 1, 999, 123};
    auto map = [&] { return wup::NormalizeGpuMemory(request, dedicated, shared,
        WupOk, 0, 100, 110, &guard.value, sizeof(guard.value)); };
    CHECK(map() == WUP_S_OK);
    CHECK(guard.value.status == WupOk);
    CHECK(guard.value.source == WupDxCoreProcessMemory);
    CHECK(guard.value.dedicated_committed.value == 0);
    CHECK(guard.value.dedicated_committed.status == WupOk);
    CHECK(guard.value.shared_committed.value == 999);
    CHECK(guard.value.shared_resident.value == 123);
    CHECK(guard.value.clock_id == 2 && guard.value.acquisition_begin_100ns == 100);
    CHECK(guard.value.process_query_flags == 3);
    shared.process_query_succeeded = 0;
    CHECK(map() == WUP_S_OK && guard.value.status == WupPartial);
    CHECK(guard.value.shared_resident.value == UINT64_MAX);
    CHECK(guard.value.shared_committed.status == WupUnavailable);
    dedicated.hresult = 0x80070005u;
    CHECK(map() == WUP_S_OK && guard.value.status == WupUnavailable);
    CHECK(guard.value.dedicated_hresult == 0x80070005u);
    CHECK(guard.value.process_query_flags == 1);
    dedicated = {0, 1, 999, 123};
    shared = {0, 1, 888, 77};
    CHECK(wup::NormalizeGpuMemory(request, dedicated, shared, WupIdentityChanged,
        ERROR_INVALID_DATA, 100, 110, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupIdentityChanged);
    CHECK(guard.value.dedicated_resident.value == UINT64_MAX);
    CHECK(guard.value.win32_error == ERROR_INVALID_DATA);
    CHECK(wup::NormalizeGpuMemory(request, dedicated, shared, WupTargetMissing,
        ERROR_PROCESS_ABORTED, 100, 110, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupTargetMissing);
    CHECK(wup::NormalizeGpuMemory(request, dedicated, shared, WupOk,
        0, 110, 100, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupUnavailable && guard.value.shared_committed.value == UINT64_MAX);
    auto saved = guard.value;
    request.version++;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.version--;
    request.size--;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.size++;
    request.reserved = 1;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.reserved = 0;
    request.process.reserved = 1;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.process.reserved = 0;
    auto pid = request.process.pid;
    request.process.pid = 0;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.process.pid = pid;
    CHECK(wup::NormalizeGpuMemory(request, dedicated, shared, WupOk, 0, 100, 110,
        &guard.value, sizeof(guard.value) - 1) == WUP_E_INVALIDARG);
    CHECK(!std::memcmp(&saved, &guard.value, sizeof(saved)));
    dedicated.process_query_succeeded = 2;
    CHECK(map() == WUP_E_INVALIDARG && !std::memcmp(&saved, &guard.value, sizeof(saved)));
    dedicated.process_query_succeeded = 1;
    for (uint32_t error : {0x80004005u, 0x887A0005u})
    {
        dedicated.hresult = error;
        CHECK(map() == WUP_S_OK && guard.value.status == WupPartial);
        CHECK(guard.value.dedicated_hresult == error && guard.value.process_query_flags == 3);
        CHECK(guard.value.dedicated_committed.value == UINT64_MAX);
    }
    dedicated.hresult = 1; // S_FALSE retains its successful, nonzero HRESULT bits.
    CHECK(map() == WUP_S_OK && guard.value.status == WupOk);
    CHECK(guard.value.dedicated_hresult == 1);
    CHECK(WupCaptureGpuMemory(GetCurrentProcess(), &request, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupUnavailable);
    CHECK(guard.value.dedicated_hresult != 0x80004001u);
    CHECK(guard.value.dedicated_committed.value == UINT64_MAX);
    CHECK(guard.value.acquisition_end_100ns >= guard.value.acquisition_begin_100ns);
    request.process.creation_filetime_100ns++;
    CHECK(WupCaptureGpuMemory(GetCurrentProcess(), &request, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupIdentityChanged);
    CHECK(WupCaptureGpuMemory(nullptr, &request, &guard.value, sizeof(guard.value)) == WUP_S_OK);
    CHECK(guard.value.status == WupUnavailable && guard.value.win32_error == ERROR_INVALID_HANDLE);
    request.process.creation_filetime_100ns--;
    saved = guard.value;
    request.version++;
    CHECK(WupCaptureGpuMemory(GetCurrentProcess(), &request, &guard.value, sizeof(guard.value)) == WUP_E_INVALIDARG);
    CHECK(!std::memcmp(&saved, &guard.value, sizeof(saved)));
    request.version--;
    CHECK(WupCaptureGpuMemory(GetCurrentProcess(), &request, &guard.value,
        sizeof(guard.value) - 1) == WUP_E_INVALIDARG);
    CHECK(!std::memcmp(&saved, &guard.value, sizeof(saved)));
    CHECK(WupCaptureGpuMemory(nullptr, nullptr, &guard.value, sizeof(guard.value)) == WUP_E_INVALIDARG);
    CHECK(!std::memcmp(&saved, &guard.value, sizeof(saved)));
    wchar_t executable[MAX_PATH]{}, command[MAX_PATH + 16]{};
    CHECK(GetModuleFileNameW(nullptr, executable, MAX_PATH) != 0);
    swprintf_s(command, L"\"%s\" --child", executable);
    STARTUPINFOW startup{}; startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    const bool launched = CreateProcessW(executable, command, nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child) != FALSE;
    CHECK(launched);
    if (launched)
    {
        const auto wait = WaitForSingleObject(child.hProcess, 5000);
        CHECK(wait == WAIT_OBJECT_0);
        if (wait != WAIT_OBJECT_0) { TerminateProcess(child.hProcess, 1); WaitForSingleObject(child.hProcess, 5000); }
        CHECK(GetProcessTimes(child.hProcess, &creation, &exit, &kernel, &user));
        request.process = {child.dwProcessId, 0,
            (uint64_t(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
        CHECK(WupCaptureGpuMemory(child.hProcess, &request, &guard.value, sizeof(guard.value)) == WUP_S_OK);
        CHECK(guard.value.status == WupTargetMissing);
        CHECK(guard.value.shared_resident.value == UINT64_MAX);
        CHECK(guard.value.win32_error == ERROR_PROCESS_ABORTED);
        CHECK(CloseHandle(child.hThread));
        CHECK(CloseHandle(child.hProcess));
    }
    CHECK(guard.before == 11 && guard.after == 22);
#endif
    std::printf("GPU memory: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
