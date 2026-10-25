#include "../WinUIProfiler.Native/Exports.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>

static int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)

static WupCpuSnapshot Snapshot(uint64_t timestamp)
{
    return {sizeof(WupCpuSnapshot), WUP_METRICS_VERSION, WupSyntheticCpuSnapshot,
        WupOk, 7, timestamp, {42, 0, 12345}, 2, 2, 1, 0, 1400, 0};
}

static WupCpuGroup Group(uint32_t id, uint64_t kernel, uint64_t user,
    uint64_t idle, uint64_t dpc, uint64_t isr)
{
    return {sizeof(WupCpuGroup), WUP_METRICS_VERSION, id, 2,
        kernel, user, idle, dpc, isr};
}

int main()
{
    static_assert(sizeof(WupProcessIdentity) == 16);
    static_assert(sizeof(WupCpuSnapshot) == 80);
    static_assert(sizeof(WupCpuGroup) == 56);
    static_assert(sizeof(WupCpuResult) == 136);
    auto a = Snapshot(1000), b = Snapshot(1100);
    b.process_kernel = 1440;
    WupCpuGroup ga[] = {Group(0, 1000, 1000, 500, 50, 10),
        Group(1, 2000, 2000, 1000, 50, 10)};
    WupCpuGroup gb[] = {Group(1, 2120, 2080, 1100, 60, 20),
        Group(0, 1120, 1080, 550, 60, 20)};
    WupCpuResult out{};
    auto run = [&]() { return WupNormalizeCpu(&a, ga, &b, gb, &out, sizeof(out)); };
    CHECK(WupNormalizeCpu(nullptr, nullptr, &b, gb, &out, sizeof(out)) == WUP_S_OK);
    CHECK(out.status == WupWarmup);
    CHECK(std::isnan(out.process_utilization.percent));
    CHECK(run() == WUP_S_OK);
    CHECK(out.status == WupOk);
    CHECK(out.system_utilization.percent == 62.5);
    CHECK(out.process_utilization.percent == 10.0);
    CHECK(out.interrupt_utilization.percent == 10.0);
    CHECK(out.other_or_unattributed_utilization.percent == 52.5);
    CHECK(out.process_utility.status == WupUnavailable);
    CHECK(std::isnan(out.process_utility.percent));
    CHECK(out.begin_100ns == 1000 && out.end_100ns == 1100);
    b.process_kernel = 1480;
    CHECK(run() == WUP_S_OK && out.process_utilization.percent == 20.0);
    b.process_kernel = a.process_kernel;
    CHECK(run() == WUP_S_OK && out.process_utilization.status == WupOk);
    CHECK(out.process_utilization.percent == 0.0);
    CHECK(out.other_or_unattributed_utilization.percent == 62.5);
    b.process_kernel = 1440;
    b.process.creation_filetime_100ns++;
    CHECK(run() == WUP_S_OK && out.status == WupIdentityChanged);
    b.process = a.process;
    b.process.pid++;
    CHECK(run() == WUP_S_OK && out.status == WupIdentityChanged);
    b.process = a.process;
    b.target_present = 0;
    CHECK(run() == WUP_S_OK && out.status == WupTargetMissing);
    b.target_present = 1;
    a.process_kernel = 10000;
    CHECK(run() == WUP_S_OK && out.status == WupCounterReset);
    a.process_kernel = 1400;
    b.process_kernel = 1800;
    CHECK(run() == WUP_S_OK && out.status == WupPartial);
    CHECK(out.system_utilization.status == WupOk);
    CHECK(std::isnan(out.process_utilization.percent));
    b.process_kernel = 1440;
    a.target_present = 0;
    CHECK(run() == WUP_S_OK && out.status == WupWarmup);
    a.target_present = 1;
    gb[1].logical_processor_count = 1;
    CHECK(run() == WUP_S_OK && out.status == WupPartial);
    gb[1].logical_processor_count = 2;
    gb[1].kernel = ga[0].kernel - 1;
    CHECK(run() == WUP_S_OK && out.status == WupCounterReset);
    gb[1].kernel = 1120;
    b.group_count = 1;
    CHECK(run() == WUP_S_OK && out.status == WupPartial);
    CHECK(std::isnan(out.system_utilization.percent));
    b.group_count = 2;
    b.status = WupUnavailable;
    CHECK(run() == WUP_S_OK && out.status == WupUnavailable);
    b.status = WupOk;
    gb[1].group_id = 1;
    CHECK(run() == WUP_E_INVALIDARG);
    gb[1].group_id = 0;
    b.timestamp_100ns = a.timestamp_100ns;
    CHECK(run() == WUP_E_INVALIDARG);
    b.timestamp_100ns = 1100;
    gb[1].kernel = std::numeric_limits<uint64_t>::max();
    gb[1].user = std::numeric_limits<uint64_t>::max();
    CHECK(run() == WUP_E_INVALIDARG);
    gb[1] = Group(0, 1120, 1080, 550, 60, 20);
    const auto saved0 = gb[0], saved1 = gb[1];
    gb[0] = ga[1];
    gb[1] = ga[0];
    CHECK(run() == WUP_S_OK && out.status == WupUnavailable);
    CHECK(std::isnan(out.process_utilization.percent));
    gb[0] = saved0;
    gb[1] = saved1;
    gb[1].isr = 1000;
    CHECK(run() == WUP_E_INVALIDARG);
    gb[1] = saved1;
    b.clock_id++;
    CHECK(run() == WUP_E_INVALIDARG);
    b.clock_id--;
    struct Guard { WupCpuResult result; uint64_t canary; } guard{};
    std::memset(&guard, 0xA5, sizeof(guard));
    const auto original = guard;
    CHECK(WupNormalizeCpu(&a, ga, &b, gb, &guard.result, sizeof(out) - 1) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    b.version++;
    CHECK(WupNormalizeCpu(&a, ga, &b, gb, &guard.result, sizeof(out)) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
    b.version--;
    CHECK(WupNormalizeCpu(&a, ga, &b, gb, &guard.result, sizeof(out)) == WUP_S_OK);
    CHECK(guard.canary == original.canary);
    const auto savedGuard = guard;
    b.expected_group_count = WUP_MAX_CPU_GROUPS + 1;
    CHECK(WupNormalizeCpu(&a, ga, &b, gb, &guard.result, sizeof(out)) == WUP_E_INVALIDARG);
    CHECK(std::memcmp(&guard, &savedGuard, sizeof(guard)) == 0);
    b.expected_group_count = 2;
    CHECK(WupNormalizeCpu(&a, ga, &b, gb, nullptr, sizeof(out)) == WUP_E_INVALIDARG);
    b.size--;
    CHECK(run() == WUP_E_INVALIDARG);
    b.size++;
    gb[1].version++;
    CHECK(run() == WUP_E_INVALIDARG);
    gb[1].version--;
    b.source = 999;
    CHECK(run() == WUP_E_INVALIDARG);
    b.source = WupSyntheticCpuSnapshot;
    CHECK(WupNormalizeCpu(&a, nullptr, &b, gb, &out, sizeof(out)) == WUP_E_INVALIDARG);
    const auto originalA = a;
    const auto originalB = b;
    a.process_kernel = a.process_user = 0;
    b.process_kernel = UINT64_MAX;
    b.process_user = 1;
    CHECK(run() == WUP_E_INVALIDARG);
    a = originalA;
    b = originalB;
    ga[0] = Group(0, 0, 0, 0, 0, 0);
    ga[1] = Group(1, 0, 0, 0, 0, 0);
    gb[0] = Group(1, UINT64_MAX / 2 + 1, 0, 0, 0, 0);
    gb[1] = Group(0, UINT64_MAX / 2 + 1, 0, 0, 0, 0);
    CHECK(run() == WUP_E_INVALIDARG);
    std::printf("%s: CPU normalization (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
