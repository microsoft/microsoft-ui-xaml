#include "..\WinUIProfiler.Native\Exports.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>

int wmain(int argc, wchar_t** argv)
{
    if (argc == 2 && wcscmp(argv[1], L"--exit-child") == 0) return 0;
    static_assert(noexcept(WupCaptureProcessMemory(nullptr, nullptr, nullptr, 0)));
    static_assert(noexcept(WupNormalizeCpu(nullptr, nullptr, nullptr, nullptr, nullptr, 0)));
    static_assert(noexcept(WupCaptureCpu(nullptr, nullptr, nullptr, 0)));
    static_assert(sizeof(WupCpuCaptureRequest) == 24);
    static_assert(sizeof(WupCpuCaptureResult) == 3696);
    if (argc != 2 || wcslen(argv[1]) < 3 || argv[1][1] != L':' || argv[1][2] != L'\\')
    {
        std::printf("FAIL: trusted explicit drive-qualified DLL path required\n");
        return 1;
    }
    HANDLE file = CreateFileW(argv[1], GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        std::printf("FAIL: DLL missing/inaccessible Win32=%u\n", GetLastError());
        return 1;
    }
    IMAGE_DOS_HEADER dos{};
    DWORD read = 0;
    LARGE_INTEGER length{};
    bool valid = GetFileSizeEx(file, &length) && ReadFile(file, &dos, sizeof(dos), &read, nullptr) &&
        read == sizeof(dos) && dos.e_magic == IMAGE_DOS_SIGNATURE && dos.e_lfanew >= sizeof(dos) &&
        dos.e_lfanew <= length.QuadPart - static_cast<LONGLONG>(sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER));
    DWORD signature = 0;
    IMAGE_FILE_HEADER header{};
    if (valid)
    {
        LARGE_INTEGER offset{};
        offset.QuadPart = dos.e_lfanew;
        valid = SetFilePointerEx(file, offset, nullptr, FILE_BEGIN) &&
            ReadFile(file, &signature, sizeof(signature), &read, nullptr) && read == sizeof(signature) &&
            signature == IMAGE_NT_SIGNATURE && ReadFile(file, &header, sizeof(header), &read, nullptr) &&
            read == sizeof(header);
    }
    CloseHandle(file);
#if defined(_M_ARM64)
    constexpr WORD expectedMachine = IMAGE_FILE_MACHINE_ARM64;
#else
    constexpr WORD expectedMachine = IMAGE_FILE_MACHINE_AMD64;
#endif
    if (!valid || header.Machine != expectedMachine || !(header.Characteristics & IMAGE_FILE_DLL))
    {
        std::printf("FAIL: DLL architecture/header machine=0x%04X expected=0x%04X\n", header.Machine, expectedMachine);
        return 1;
    }
    HMODULE module = LoadLibraryExW(argv[1], nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module)
    {
        std::printf("FAIL: explicit DLL load Win32=%u\n", GetLastError());
        return 1;
    }
    decltype(&WupCaptureProcessMemory) memory{};
    decltype(&WupNormalizeCpu) cpu{};
    decltype(&WupCaptureCpu) capture{};
    decltype(&WupCaptureGpu) gpu{};
    decltype(&WupNormalizeGpu) normalizeGpu{};
    auto address = GetProcAddress(module, "WupCaptureProcessMemory");
    std::memcpy(&memory, &address, sizeof(memory));
    address = GetProcAddress(module, "WupNormalizeCpu");
    std::memcpy(&cpu, &address, sizeof(cpu));
    address = GetProcAddress(module, "WupCaptureCpu");
    std::memcpy(&capture, &address, sizeof(capture));
    address = GetProcAddress(module, "WupCaptureGpu");
    std::memcpy(&gpu, &address, sizeof(gpu));
    address = GetProcAddress(module, "WupNormalizeGpu");
    std::memcpy(&normalizeGpu, &address, sizeof(normalizeGpu));
    int failures = 0;
#define CHECK(x) do { if (!(x)) { std::printf("FAIL DLL line %d: %s\n", __LINE__, #x); ++failures; } } while (0)
    CHECK(memory && cpu);
    CHECK(capture);
    CHECK(gpu && normalizeGpu);
    CHECK(GetProcAddress(module, "WupCaptureGpuMemory"));
    CHECK(GetProcAddress(module, "WupEnumerateGpuAdapters"));
    if (gpu && normalizeGpu)
    {
        FILETIME created{}, exited{}, kernel{}, user{};
        CHECK(GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user));
        WupGpuRequest request{sizeof(request), WUP_GPU_VERSION, {GetCurrentProcessId(), 0,
            (static_cast<uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime}};
        struct GpuGuard { WupGpuSnapshot result; uint64_t canary; } guard{};
        std::memset(&guard, 0xA5, sizeof(guard));
        const auto original = guard;
        CHECK(gpu(GetCurrentProcess(), &request, &guard.result, sizeof(WupGpuSnapshot) - 1) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
        request.version++;
        CHECK(gpu(GetCurrentProcess(), &request, &guard.result, sizeof(WupGpuSnapshot)) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
        request.version--;
        CHECK(gpu(GetCurrentProcess(), &request, &guard.result, sizeof(WupGpuSnapshot)) == WUP_S_OK);
        CHECK(guard.canary == original.canary);
        CHECK(guard.result.source == WupEnglishPdhGpuEngine);
        CHECK(guard.result.status == WupOk || guard.result.status == WupUnavailable || guard.result.status == WupPartial);
        std::printf("DLL GPU status=%u source=%u rows=%u error=0x%08X sampleFILETIME=%llu acquisition=%llu..%llu\n",
            guard.result.status, guard.result.source, guard.result.row_count, guard.result.pdh_error,
            guard.result.sample_filetime_100ns, guard.result.acquisition_begin_100ns, guard.result.acquisition_end_100ns);
        std::printf("DLL GPU observedInstances=%u acquisitionClock=%llu sampleClock=%llu win32=%u\n",
            guard.result.observed_instance_count, guard.result.acquisition_clock_id, guard.result.clock_id, guard.result.win32_error);
        WupGpuResult result{};
        auto a = guard.result;
        a.size = sizeof(a); a.version = WUP_GPU_VERSION; a.status = WupOk;
        a.clock_id = 3; a.sample_filetime_100ns = 100; a.row_count = 2;
        a.rows[0] = {1, 2, 0, 0, 0, WupOk, 0, PERF_100NSEC_TIMER, 10, 100, 100};
        a.rows[1] = {1, 2, 0, 1, 0, WupOk, 0, PERF_100NSEC_TIMER, 20, 100, 100};
        auto b = a;
        b.sample_filetime_100ns = 200;
        b.rows[0].running_time += 30; b.rows[0].timer += 100; b.rows[0].timestamp_filetime_100ns += 100;
        b.rows[1].running_time += 70; b.rows[1].timer += 100; b.rows[1].timestamp_filetime_100ns += 100;
        CHECK(normalizeGpu(nullptr, &a, &result, sizeof(result)) == WUP_S_OK && result.status == WupWarmup);
        CHECK(std::isnan(result.utilization.percent));
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupOk);
        CHECK(result.utilization.percent == 70 && result.engine_count == 2);
        b.rows[1].running_time = 0;
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupCounterReset);
        b = a; b.status = WupUnavailable; b.row_count = 0;
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupUnavailable);
        b = a; b.process.creation_filetime_100ns++;
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupIdentityChanged);
        b = a; b.sample_filetime_100ns += 100;
        for (auto& row : b.rows) { row.timer += 100; row.timestamp_filetime_100ns += 100; }
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupOk);
        CHECK(result.utilization.percent == 0);
        b.rows[0].duplicate_index = 1;
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_S_OK && result.status == WupPartial);
        const auto preserved = result;
        b.row_count = WUP_MAX_GPU_ROWS + 1;
        CHECK(normalizeGpu(&a, &b, &result, sizeof(result)) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&preserved, &result, sizeof(result)) == 0);
        request.process.creation_filetime_100ns++;
        CHECK(gpu(GetCurrentProcess(), &request, &guard.result, sizeof(WupGpuSnapshot)) == WUP_S_OK);
        CHECK(guard.result.status == WupIdentityChanged);
        request.process.creation_filetime_100ns--;
        CHECK(gpu(nullptr, &request, &guard.result, sizeof(WupGpuSnapshot)) == WUP_S_OK);
        CHECK(guard.result.status == WupUnavailable);
    }
    if (memory && cpu)
    {
        FILETIME creation{}, exit{}, kernel{}, user{};
        CHECK(GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user));
        WupMemoryRequest request{sizeof(request), WUP_MEMORY_VERSION,
            {GetCurrentProcessId(), 0,
                (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime}};
        struct Guard { WupMemoryResult result; uint64_t canary; } guard{};
        std::memset(&guard, 0xA5, sizeof(guard));
        const auto original = guard;
        CHECK(memory(GetCurrentProcess(), &request, &guard.result, sizeof(WupMemoryResult) - 1) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
        request.version++;
        CHECK(memory(GetCurrentProcess(), &request, &guard.result, sizeof(WupMemoryResult)) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
        request.version--;
        request.size--;
        CHECK(memory(GetCurrentProcess(), &request, &guard.result, sizeof(WupMemoryResult)) == WUP_E_INVALIDARG);
        CHECK(std::memcmp(&guard, &original, sizeof(guard)) == 0);
        request.size++;
        CHECK(memory(GetCurrentProcess(), &request, &guard.result, sizeof(WupMemoryResult)) == WUP_S_OK);
        CHECK(guard.canary == original.canary);
        CHECK(guard.result.size == 112 && guard.result.version == 1);
        CHECK(guard.result.status == WupOk || guard.result.status == WupPartial);
        CHECK(guard.result.private_commit.status == WupOk && guard.result.private_commit.units == WupBytes);
        CHECK(guard.result.process.pid == request.process.pid &&
            guard.result.process.creation_filetime_100ns == request.process.creation_filetime_100ns);
        CHECK(cpu(nullptr, nullptr, nullptr, nullptr, nullptr, 0) == WUP_E_INVALIDARG);
        WupCpuSnapshot baseline{sizeof(WupCpuSnapshot), WUP_METRICS_VERSION,
            WupSyntheticCpuSnapshot, WupOk, 7, 100, request.process, 1, 1, 1, 0, 100, 50};
        WupCpuGroup group{sizeof(WupCpuGroup), WUP_METRICS_VERSION, 0, 2, 1000, 1000, 500, 50, 10};
        struct CpuGuard { WupCpuResult result; uint64_t canary; } cpuGuard{};
        std::memset(&cpuGuard, 0xA5, sizeof(cpuGuard));
        CHECK(cpu(nullptr, nullptr, &baseline, &group, &cpuGuard.result, sizeof(WupCpuResult)) == WUP_S_OK);
        CHECK(cpuGuard.result.status == WupWarmup && std::isnan(cpuGuard.result.process_utilization.percent));
        CHECK(cpuGuard.canary == original.canary);
        auto next = baseline;
        auto nextGroup = group;
        next.timestamp_100ns += 100;
        next.process_kernel += 20;
        nextGroup.kernel += 120;
        nextGroup.user += 80;
        nextGroup.idle += 100;
        nextGroup.dpc += 10;
        nextGroup.isr += 10;
        CHECK(cpu(&baseline, &group, &next, &nextGroup, &cpuGuard.result, sizeof(WupCpuResult)) == WUP_S_OK);
        CHECK(cpuGuard.result.status == WupOk && cpuGuard.result.process_utilization.percent == 10);
        CHECK(cpuGuard.canary == original.canary);
        std::printf("DLL memory source=%u status=%u privateWS=%llu totalWS=%llu commit=%llu size=%u version=%u\n",
            guard.result.source, guard.result.status, guard.result.private_working_set.value,
            guard.result.total_working_set.value, guard.result.private_commit.value,
            guard.result.size, guard.result.version);
        if (capture)
        {
            WupCpuCaptureRequest cpuRequest{sizeof(cpuRequest), WUP_CPU_CAPTURE_VERSION, request.process};
            struct CaptureGuard { WupCpuCaptureResult result; uint64_t canary; } captured{};
            std::memset(&captured, 0xA5, sizeof(captured));
            const auto originalCapture = captured;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult) - 1) == WUP_E_INVALIDARG);
            CHECK(std::memcmp(&captured, &originalCapture, sizeof(captured)) == 0);
            cpuRequest.version++;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_E_INVALIDARG);
            CHECK(std::memcmp(&captured, &originalCapture, sizeof(captured)) == 0);
            cpuRequest.version--;
            cpuRequest.size--;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_E_INVALIDARG);
            CHECK(std::memcmp(&captured, &originalCapture, sizeof(captured)) == 0);
            cpuRequest.size++;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
            CHECK(captured.canary == originalCapture.canary);
            const auto firstCapture = captured.result;
            CHECK(firstCapture.size == 3696 && firstCapture.version == 1);
            CHECK(firstCapture.snapshot.status == WupOk && firstCapture.snapshot.source == WupNtProcessSnapshotCpu);
            CHECK(firstCapture.snapshot.clock_id == 2);
            CHECK(firstCapture.snapshot.group_count > 0 && firstCapture.snapshot.group_count <= WUP_MAX_CPU_GROUPS);
            CHECK(firstCapture.snapshot.group_count == firstCapture.snapshot.expected_group_count);
            CHECK(firstCapture.acquisition_begin_100ns <= firstCapture.snapshot.timestamp_100ns &&
                firstCapture.snapshot.timestamp_100ns <= firstCapture.acquisition_end_100ns);
            WupCpuResult actual{};
            CHECK(cpu(nullptr, nullptr, &firstCapture.snapshot, firstCapture.groups, &actual, sizeof(actual)) == WUP_S_OK);
            CHECK(actual.status == WupWarmup && std::isnan(actual.process_utilization.percent));
            Sleep(100);
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
            CHECK(captured.result.snapshot.status == WupOk);
            CHECK(cpu(&firstCapture.snapshot, firstCapture.groups, &captured.result.snapshot,
                captured.result.groups, &actual, sizeof(actual)) == WUP_S_OK);
            CHECK(actual.status == WupOk || actual.status == WupPartial);
            CHECK(actual.clock_id == 2 && actual.process_utility.status == WupUnavailable);
            std::printf("DLL CPU capture source=%u status=%u groups=%u/%u clock=%llu normalizedStatus=%u system=%g process=%g begin=%llu end=%llu\n",
                captured.result.snapshot.source, captured.result.snapshot.status,
                captured.result.snapshot.group_count, captured.result.snapshot.expected_group_count,
                actual.clock_id, actual.status, actual.system_utilization.percent, actual.process_utilization.percent,
                actual.begin_100ns, actual.end_100ns);
            cpuRequest.process.creation_filetime_100ns++;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
            CHECK(captured.result.snapshot.status == WupIdentityChanged);
            cpuRequest.process.creation_filetime_100ns--;
            cpuRequest.process.pid++;
            CHECK(capture(GetCurrentProcess(), &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
            CHECK(captured.result.snapshot.status == WupIdentityChanged);
            cpuRequest.process.pid--;
            CHECK(capture(nullptr, &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
            CHECK(captured.result.snapshot.status == WupUnavailable && captured.result.error_domain == 1);
            CHECK(captured.result.snapshot.group_count == 0 && !captured.result.snapshot.target_present);
            CHECK(cpu(nullptr, nullptr, &captured.result.snapshot, captured.result.groups, &actual, sizeof(actual)) == WUP_S_OK);
            CHECK(actual.status == WupUnavailable && std::isnan(actual.process_utilization.percent));
            const auto failedCapture = captured;
            CHECK(capture(GetCurrentProcess(), nullptr, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_E_INVALIDARG);
            CHECK(std::memcmp(&captured, &failedCapture, sizeof(captured)) == 0);
            CHECK(capture(GetCurrentProcess(), &cpuRequest, nullptr, sizeof(WupCpuCaptureResult)) == WUP_E_INVALIDARG);
            CHECK(captured.canary == originalCapture.canary);
            wchar_t executable[MAX_PATH]{}, command[2 * MAX_PATH]{};
            CHECK(GetModuleFileNameW(nullptr, executable, MAX_PATH) > 0);
            swprintf_s(command, L"\"%s\" --exit-child", executable);
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
                cpuRequest.process = {child.dwProcessId, 0,
                    (static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime};
                CHECK(capture(child.hProcess, &cpuRequest, &captured.result, sizeof(WupCpuCaptureResult)) == WUP_S_OK);
                CHECK(captured.result.snapshot.status == WupTargetMissing && captured.result.snapshot.group_count == 0);
                CHECK(captured.canary == originalCapture.canary);
                CloseHandle(child.hThread);
                CloseHandle(child.hProcess);
            }
        }
    }
    FreeLibrary(module);
    std::printf("%s: native DLL interop (%d failures) machine=0x%04X\n",
        failures ? "FAILED" : "PASSED", failures, header.Machine);
    return failures ? 1 : 0;
}
