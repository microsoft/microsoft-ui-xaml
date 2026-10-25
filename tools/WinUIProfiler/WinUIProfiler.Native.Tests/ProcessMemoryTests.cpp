#include "..\WinUIProfiler.Native\Metrics\ProcessMemorySampler.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>

int main(int argc, char**)
{
    if (argc > 1) return 0;
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL memory line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    static_assert(sizeof(WupMemoryRequest) == 24);
    static_assert(sizeof(WupByteMetric) == 16);
    static_assert(sizeof(WupMemoryResult) == 112);
    WupMemoryResult fixture{};
    fixture.source = WupSyntheticMemory;
    const uint64_t privateResident = 1024;
    wup::SetMemoryValues(fixture, 4096, 16384, &privateResident);
    CHECK(fixture.private_working_set.status == WupOk && fixture.private_working_set.value == 1024);
    CHECK(fixture.total_working_set.value == 4096 && fixture.private_commit.value == 16384);
    CHECK(fixture.private_commit.units == 1 && fixture.total_working_set.units == 1);
    wup::SetMemoryValues(fixture, 4096, 16384, nullptr);
    CHECK(fixture.status == WupPartial && fixture.private_working_set.status == WupUnavailable);
    CHECK(fixture.private_working_set.value == UINT64_MAX);
    CHECK(fixture.private_commit.value == 16384 && fixture.total_working_set.value == 4096);
    const uint64_t zero = 0;
    wup::SetMemoryValues(fixture, 0, 0, &zero);
    CHECK(fixture.private_working_set.status == WupOk && fixture.private_working_set.value == 0);

    FILETIME creation{}, exit{}, kernel{}, user{};
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, GetCurrentProcessId());
    CHECK(process != nullptr);
    CHECK(GetProcessTimes(process, &creation, &exit, &kernel, &user));
    WupMemoryRequest request{sizeof(request), WUP_MEMORY_VERSION, {GetCurrentProcessId(), 0,
        (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime}};
    WupMemoryResult before{}, during{}, after{};
    auto sample = [&](WupMemoryResult& result) {
        return WupCaptureProcessMemory(process, &request, &result, sizeof(result));
    };
    CHECK(sample(before) == WUP_S_OK);
    CHECK(before.status == WupOk || before.status == WupPartial);
    CHECK(before.private_commit.status == WupOk && before.total_working_set.status == WupOk);
    CHECK(before.clock_id != 0 && before.acquisition_end_100ns >= before.acquisition_begin_100ns);
    constexpr SIZE_T allocationSize = 16 * 1024 * 1024;
    auto memory = static_cast<volatile unsigned char*>(VirtualAlloc(nullptr, allocationSize,
        MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    CHECK(memory != nullptr);
    if (memory)
    {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        for (SIZE_T offset = 0; offset < allocationSize; offset += info.dwPageSize) memory[offset] = 0x5A;
        CHECK(sample(during) == WUP_S_OK);
        CHECK(during.private_commit.status == WupOk);
        CHECK(during.private_commit.value >= before.private_commit.value + allocationSize);
        if (during.private_working_set.status == WupOk)
            CHECK(during.private_working_set.value <= during.total_working_set.value);
        CHECK(VirtualFree(const_cast<unsigned char*>(memory), 0, MEM_RELEASE));
        CHECK(sample(after) == WUP_S_OK);
        CHECK(after.private_commit.status == WupOk);
        CHECK(during.private_commit.value >= after.private_commit.value + allocationSize);
    }
    std::printf("Memory probe source=%u status=%u privateWS=%llu/%u totalWS=%llu/%u commit=%llu/%u error=%u begin=%llu end=%llu\n",
        before.source, before.status, before.private_working_set.value, before.private_working_set.status,
        before.total_working_set.value, before.total_working_set.status, before.private_commit.value,
        before.private_commit.status, before.win32_error, before.acquisition_begin_100ns, before.acquisition_end_100ns);
    std::printf("Memory workload commit before=%llu during=%llu after=%llu allocation=%llu\n",
        before.private_commit.value, during.private_commit.value, after.private_commit.value,
        static_cast<uint64_t>(allocationSize));
    struct Guard { WupMemoryResult result; uint64_t canary; } guard{};
    std::memset(&guard, 0xA5, sizeof(guard));
    const auto original = guard;
    CHECK(WupCaptureProcessMemory(process, &request, &guard.result, sizeof(before) - 1) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    request.version++;
    CHECK(WupCaptureProcessMemory(process, &request, &guard.result, sizeof(before)) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    request.version--;
    request.size--;
    CHECK(WupCaptureProcessMemory(process, &request, &guard.result, sizeof(before)) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    request.size++;
    CHECK(WupCaptureProcessMemory(process, &request, &guard.result, sizeof(before)) == WUP_S_OK);
    CHECK(guard.canary == original.canary);
    CHECK(WupCaptureProcessMemory(nullptr, &request, &after, sizeof(after)) == WUP_S_OK);
    CHECK(after.status == WupUnavailable && after.private_commit.value == UINT64_MAX);
    request.process.creation_filetime_100ns++;
    CHECK(sample(after) == WUP_S_OK && after.status == WupIdentityChanged);
    request.process.creation_filetime_100ns--;
    request.process.pid++;
    CHECK(sample(after) == WUP_S_OK && after.status == WupIdentityChanged);
    request.process.pid--;
    CHECK(WupCaptureProcessMemory(process, nullptr, &after, sizeof(after)) == WUP_E_INVALIDARG);
    CHECK(WupCaptureProcessMemory(process, &request, nullptr, sizeof(after)) == WUP_E_INVALIDARG);
    CloseHandle(process);

    wchar_t executable[MAX_PATH]{}, command[2 * MAX_PATH]{};
    CHECK(GetModuleFileNameW(nullptr, executable, MAX_PATH) > 0);
    swprintf_s(command, L"\"%s\" --child", executable);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION child{};
    const bool started = CreateProcessW(executable, command, nullptr, nullptr, FALSE,
        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child) != FALSE;
    CHECK(started);
    if (started)
    {
        CHECK(WaitForSingleObject(child.hProcess, 5000) == WAIT_OBJECT_0);
        CHECK(GetProcessTimes(child.hProcess, &creation, &exit, &kernel, &user));
        request.process = {child.dwProcessId, 0,
            (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
        CHECK(WupCaptureProcessMemory(child.hProcess, &request, &after, sizeof(after)) == WUP_S_OK);
        CHECK(after.status == WupTargetMissing && after.private_working_set.value == UINT64_MAX);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
    }
    std::printf("%s: process memory (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
