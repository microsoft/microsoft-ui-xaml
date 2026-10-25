#include "..\WinUIProfiler.Native\Metrics\GpuFoundation.h"
#include <Windows.h>
#include <cstdio>
#include <cwchar>
#include <cmath>

int main()
{
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL GPU line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    wup::GpuInstance instance{};
    auto parse = [&](const wchar_t* text) { return wup::ParseGpuInstance(text, static_cast<uint32_t>(wcslen(text)), instance); };
    CHECK(parse(L"pid_42_luid_0x00000001_0x89ABCDEF_phys_0_eng_2_engtype_3D#1"));
    CHECK(instance.pid == 42 && instance.luid_high == 1 && instance.luid_low == 0x89ABCDEF);
    CHECK(instance.engine == 2 && instance.duplicate_index == 1 && instance.has_engine);
    CHECK(parse(L"pid_42_luid_0x00000001_0x89abcdef_phys_2"));
    CHECK(!instance.has_engine && instance.physical == 2);
    CHECK(!parse(L"pid_4294967296_luid_0x00000001_0x00000000_phys_0"));
    CHECK(!parse(L"pid_42_luid_0x1_0x00000000_phys_0"));
    CHECK(!parse(L"pid_42_luid_0x00000001_0x00000000_phys_-1"));
    CHECK(!parse(L"pid_42_luid_0x00000001_0x00000000_phys_0_eng_0_engtype_"));
    CHECK(!parse(L"pid_42_luid_0x00000001_0x00000000_phys_0#0"));
    CHECK(!parse(L"pid_42_luid_0x00000001_0x00000000_phys_0#1junk"));
    CHECK(parse(L"pid_42_luid_0x00000001_0x00000000_phys_0_eng_0_engtype_3D"));
    wup::GpuContext rows[] = {
        {instance, 1, WupOk, 20, 100, 200}, {instance, 2, WupOk, 30, 100, 200},
        {instance, 3, WupOk, 70, 100, 200}, {instance, 4, WupOk, 60, 100, 200}};
    rows[2].instance.engine = 1;
    rows[3].instance.luid_low = 2;
    auto result = wup::AggregateGpu(rows, 4, 42);
    CHECK(result.status == WupOk && result.percent == 70 && result.engine_count == 3);
    CHECK(result.dedicated_committed.status == WupUnavailable && result.shared_resident.value == UINT64_MAX);
    for (auto& row : rows) row.percent = 0;
    CHECK(wup::AggregateGpu(rows, 4, 42).percent == 0);
    CHECK(wup::AggregateGpu(nullptr, 0, 42).status == WupUnavailable);
    rows[0].status = WupWarmup;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupWarmup);
    rows[0].status = WupCounterReset;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupCounterReset);
    rows[0].status = WupOk;
    rows[1].context_id = 1;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    rows[1].context_id = 2;
    rows[1].end_100ns++;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    CHECK(wup::AggregateGpu(rows, wup::MaxGpuRows + 1, 42).status == WupPartial);
    rows[1].end_100ns--;
    rows[1].status = 999;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    rows[1].status = WupOk;
    rows[1].clock_id++;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    rows[1].clock_id--;
    rows[1].instance.duplicate_index = 1;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    rows[1].instance.duplicate_index = 0;
    rows[1].percent = -1;
    CHECK(wup::AggregateGpu(rows, 4, 42).status == WupPartial);
    rows[1].percent = 0;
    const auto preserved = instance;
    CHECK(!parse(L"pid_42_luid_0x00000001_0x00000000_phys_0#1bad"));
    CHECK(instance.pid == preserved.pid && instance.has_engine == preserved.has_engine);
    FILETIME creation{}, exit{}, kernel{}, user{};
    CHECK(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user));
    wup::ProbeGpu(GetCurrentProcess(), {GetCurrentProcessId(), 0,
        (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime});
    std::printf("%s: GPU foundation (%d failures); hardware capability is separate evidence\n",
        failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
