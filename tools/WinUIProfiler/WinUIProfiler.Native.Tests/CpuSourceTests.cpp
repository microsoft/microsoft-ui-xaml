#include "..\WinUIProfiler.Native\Metrics\CpuSource.h"
#include <cstdio>
#include <cmath>

int main(int argc, char**)
{
    if (argc > 1) return 0;
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL source line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    CHECK(wup::ValidProcessorBuffer(48 * 4, 4));
    CHECK(!wup::ValidProcessorBuffer(48 * 4 - 1, 4));
    CHECK(!wup::ValidProcessorBuffer(48 * 4 + 48, 4));
    CHECK(!wup::ValidProcessorBuffer(0, 0));
    CHECK(!wup::ValidProcessorBuffer(48 * 65, 65));
    FILETIME creation{}, exit{}, kernel{}, user{};
    CHECK(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user));
    const WupProcessIdentity identity{GetCurrentProcessId(), 0,
        (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
    const auto a = wup::CaptureCpu(GetCurrentProcess(), identity);
    std::printf("CPU probe source=%u status=%u groups=%u/%u inventory=%u error=0x%08X pid=%u creation=%llu\n",
        a.snapshot.source, a.snapshot.status, a.snapshot.group_count,
        a.snapshot.expected_group_count, a.inventory_count, a.os_error,
        a.snapshot.process.pid, a.snapshot.process.creation_filetime_100ns);
    CHECK(a.snapshot.source == WupNtProcessSnapshotCpu);
    CHECK(a.snapshot.status == WupOk);
    CHECK(a.snapshot.group_count == GetActiveProcessorGroupCount());
    CHECK(a.snapshot.group_count > 0);
    CHECK(a.inventory_count > 0);
    for (uint32_t i = 0; i < a.snapshot.group_count; ++i)
        std::printf(" group=%u processors=%u K=%llu U=%llu I=%llu DPC=%llu ISR=%llu\n",
            a.groups[i].group_id, a.groups[i].logical_processor_count,
            a.groups[i].kernel, a.groups[i].user, a.groups[i].idle, a.groups[i].dpc, a.groups[i].isr);
    WupCpuResult result{};
    CHECK(WupNormalizeCpu(nullptr, nullptr, &a.snapshot, a.groups, &result, sizeof(result)) == WUP_S_OK);
    CHECK(result.status == WupWarmup && std::isnan(result.process_utilization.percent));
    Sleep(100);
    const auto b = wup::CaptureCpu(GetCurrentProcess(), identity);
    CHECK(b.snapshot.status == WupOk);
    CHECK(WupNormalizeCpu(&a.snapshot, a.groups, &b.snapshot, b.groups, &result, sizeof(result)) == WUP_S_OK);
    CHECK(result.status == WupOk || result.status == WupPartial);
    auto wrong = identity;
    wrong.creation_filetime_100ns++;
    CHECK(wup::CaptureCpu(GetCurrentProcess(), wrong).snapshot.status == WupIdentityChanged);
    CHECK(wup::CaptureCpu(nullptr, identity).snapshot.status == WupUnavailable);
    wrong = identity;
    wrong.pid++;
    CHECK(wup::CaptureCpu(GetCurrentProcess(), wrong).snapshot.status != WupOk);
    wchar_t executable[MAX_PATH]{};
    CHECK(GetModuleFileNameW(nullptr, executable, MAX_PATH) > 0);
    wchar_t command[2 * MAX_PATH]{};
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
        const WupProcessIdentity childIdentity{child.dwProcessId, 0,
            (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
        CHECK(wup::CaptureCpu(child.hProcess, childIdentity).snapshot.status == WupTargetMissing);
        CloseHandle(child.hThread);
        CloseHandle(child.hProcess);
    }
    std::printf("%s: CPU source (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
