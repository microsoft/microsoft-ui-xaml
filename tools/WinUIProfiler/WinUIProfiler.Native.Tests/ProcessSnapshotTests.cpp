#include "..\WinUIProfiler.Native\Metrics\CpuSource.h"
#include "..\WinUIProfiler.Native\Metrics\ProcessSnapshot.h"
#include <cstdio>

namespace
{
int calls = 0, mode = 0;
std::vector<unsigned char> Fixture(WupProcessIdentity target)
{
    SYSTEM_PROCESS_INFORMATION row{};
    row.UniqueProcessId = reinterpret_cast<HANDLE>(static_cast<uintptr_t>(target.pid));
    std::memcpy(row.Reserved1 + 24, &target.creation_filetime_100ns, 8);
    const uint64_t user = 100, kernel = 200, privateWS = 1024;
    std::memcpy(row.Reserved1, &privateWS, 8);
    std::memcpy(row.Reserved1 + 32, &user, 8);
    std::memcpy(row.Reserved1 + 40, &kernel, 8);
    row.WorkingSetSize = 4096;
    row.PrivatePageCount = 8192;
    std::vector<unsigned char> bytes(sizeof(row));
    std::memcpy(bytes.data(), &row, sizeof(row));
    return bytes;
}
LONG NTAPI Query(ULONG kind, void* output, ULONG size, ULONG* returned)
{
    ++calls;
    if (kind != 5) return static_cast<LONG>(0xC0000003);
    if (mode == 1) return static_cast<LONG>(0xC0000022);
    if (mode == 2) { *returned = 32 * 1024 * 1024; return static_cast<LONG>(0xC0000004); }
    if (mode == 3 || calls == 1) { *returned = size + 1024; return static_cast<LONG>(0xC0000004); }
    const auto bytes = Fixture({123, 0, 456});
    *returned = static_cast<ULONG>(bytes.size());
    std::memcpy(output, bytes.data(), bytes.size());
    return 0;
}
}

int main()
{
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL snapshot line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    FILETIME creation{}, exit{}, kernel{}, user{};
    CHECK(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user));
    const WupProcessIdentity target{GetCurrentProcessId(), 0,
        (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
    const auto cpu = wup::CaptureCpu(GetCurrentProcess(), target);
    CHECK(cpu.snapshot.source == 9);
    CHECK(cpu.snapshot.status == WupOk);
    WupMemoryRequest request{sizeof(request), WUP_MEMORY_VERSION, target};
    WupMemoryResult memory{};
    CHECK(WupCaptureProcessMemory(GetCurrentProcess(), &request, &memory, sizeof(memory)) == WUP_S_OK);
    CHECK(memory.source == 10);
    CHECK(memory.status == WupOk);
    CHECK(memory.private_working_set.value <= memory.total_working_set.value);
    CHECK(memory.private_commit.value > 0);
    const WupProcessIdentity identity{123, 0, 456};
    const auto bytes = Fixture(identity);
    const auto parsed = wup::detail::ParseProcessSnapshot(bytes.data(), bytes.size(), identity);
    CHECK(parsed.status == WupOk && parsed.error == 0 && parsed.domain == 0 && parsed.count == 1);
    CHECK(parsed.kernel == 200 && parsed.user == 100);
    CHECK(parsed.private_working_set == 1024 && parsed.working_set == 4096 && parsed.commit == 8192);
    CHECK(wup::detail::ParseProcessSnapshot(bytes.data(), bytes.size() - 1, identity).status == WupUnavailable);
    CHECK(wup::detail::ParseProcessSnapshot(nullptr, bytes.size(), identity).status == WupUnavailable);
    CHECK(wup::detail::ParseProcessSnapshot(bytes.data(), bytes.size(), {123, 0, 457}).status == WupIdentityChanged);
    CHECK(wup::detail::ParseProcessSnapshot(bytes.data(), bytes.size(), {124, 0, 456}).status == WupTargetMissing);
    for (const ULONG next : {1ul, 128ul, 257ul, 512ul})
    {
        auto malformed = bytes;
        std::memcpy(malformed.data(), &next, sizeof(next));
        CHECK(wup::detail::ParseProcessSnapshot(malformed.data(), malformed.size(), identity).status == WupUnavailable);
    }
    auto duplicate = bytes;
    duplicate.insert(duplicate.end(), bytes.begin(), bytes.end());
    const ULONG next = 256;
    std::memcpy(duplicate.data(), &next, sizeof(next));
    CHECK(wup::detail::ParseProcessSnapshot(duplicate.data(), duplicate.size(), identity).status == WupUnavailable);
    auto invalid = bytes;
    const uint64_t negative = UINT64_MAX;
    std::memcpy(invalid.data() + 48, &negative, sizeof(negative));
    CHECK(wup::detail::ParseProcessSnapshot(invalid.data(), invalid.size(), identity).status == WupUnavailable);
    calls = mode = 0;
    CHECK(wup::detail::CaptureProcessSnapshot(Query, identity).status == WupOk && calls == 2);
    mode = 1; calls = 0;
    const auto denied = wup::detail::CaptureProcessSnapshot(Query, identity);
    CHECK(denied.status == WupUnavailable && denied.error == 0xC0000022 && denied.domain == 2 && calls == 1);
    mode = 2; calls = 0;
    CHECK(wup::detail::CaptureProcessSnapshot(Query, identity).status == WupUnavailable && calls == 1);
    mode = 3; calls = 0;
    CHECK(wup::detail::CaptureProcessSnapshot(Query, identity).error == ERROR_RETRY && calls == 4);
    CHECK(wup::detail::CaptureProcessSnapshot(nullptr, identity).error == ERROR_PROC_NOT_FOUND);
    std::printf("%s: process snapshot sources (%d failures)\n", failures ? "FAILED" : "PASSED", failures);
    return failures ? 1 : 0;
}
