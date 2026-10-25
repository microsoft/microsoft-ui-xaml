#include "GpuFoundation.h"
#include "GpuPdhArray.h"
#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <initguid.h>
#include <dxcore.h>
#include <cstdio>
#include <cstring>

namespace wup
{
namespace
{
bool Matches(HANDLE process, WupProcessIdentity expected)
{
    FILETIME creation{}, exit{}, kernel{}, user{};
    return expected.pid && !expected.reserved && GetProcessId(process) == expected.pid &&
        GetProcessTimes(process, &creation, &exit, &kernel, &user) &&
        ((static_cast<uint64_t>(creation.dwHighDateTime) << 32) | creation.dwLowDateTime) ==
            expected.creation_filetime_100ns && WaitForSingleObject(process, 0) == WAIT_TIMEOUT;
}
void PdhArray(PDH_HCOUNTER counter, uint32_t pid, uint32_t family)
{
    WupGpuSnapshot snapshot{};
    uint32_t observed = 0, malformed = 0;
    const auto status = ReadGpuPdhArray(counter, pid, snapshot, observed, malformed);
    std::printf(" PDH raw family=%u arrayStatus=0x%08X instances=%u selectedEngineRows=%u malformed=%u (capability only)\n",
        family, static_cast<uint32_t>(status), observed, snapshot.row_count, malformed);
}
}

void ProbeGpu(void* process, WupProcessIdentity expected)
{
    if (!Matches(process, expected))
    { std::printf("GPU probe identity unavailable/mismatched; no measurement\n"); return; }
    const auto begin = GetTickCount64() * 10000;
    auto module = LoadLibraryExW(L"dxcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HRESULT factoryStatus = HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND);
    if (module)
    {
        using Create = HRESULT (WINAPI*)(REFIID, void**);
        const auto address = GetProcAddress(module, "DXCoreCreateAdapterFactory");
        Create create{};
        std::memcpy(&create, &address, sizeof(create));
        IDXCoreAdapterFactory* factory = nullptr;
        factoryStatus = create ? create(IID_PPV_ARGS(&factory)) : E_NOINTERFACE;
        if (SUCCEEDED(factoryStatus))
        {
            IDXCoreAdapterList* list = nullptr;
            const GUID attribute = DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS;
            const auto listStatus = factory->CreateAdapterList(1, &attribute, IID_PPV_ARGS(&list));
            std::printf("DXCore listHR=0x%08X adapters=%u (D3D12 graphics filter only)\n",
                static_cast<uint32_t>(listStatus), list ? list->GetAdapterCount() : 0);
            if (list)
            {
                const auto count = list->GetAdapterCount();
                for (uint32_t i = 0; i < count && i < 16; ++i)
                {
                    IDXCoreAdapter* adapter = nullptr;
                    if (FAILED(list->GetAdapter(i, IID_PPV_ARGS(&adapter)))) continue;
                    const bool engine = adapter->IsQueryStateSupported(DXCoreAdapterState::AdapterEngineRunningTimeByProcessMicroseconds);
                    const bool memory = adapter->IsQueryStateSupported(DXCoreAdapterState::AdapterMemoryUsageByProcessBytes);
                    DXCoreEngineQueryInput input{{0, 0}, expected.pid};
                    DXCoreEngineQueryOutput output{};
                    const HRESULT engineHR = engine ? adapter->QueryState(
                        DXCoreAdapterState::AdapterEngineRunningTimeByProcessMicroseconds, &input, &output) : E_NOTIMPL;
                    DXCoreProcessMemoryQueryInput memoryInput{0, DXCoreMemoryType::Dedicated, expected.pid};
                    DXCoreProcessMemoryQueryOutput memoryOutput{};
                    const HRESULT memoryHR = memory ? adapter->QueryState(
                        DXCoreAdapterState::AdapterMemoryUsageByProcessBytes, &memoryInput, &memoryOutput) : E_NOTIMPL;
                    std::printf(" DXCore adapter=%u perPIDEngine=%u HR=0x%08X processOK=%u perPIDMemory=%u HR=0x%08X processOK=%u (phys0/eng0 capability only)\n",
                        i, engine, static_cast<uint32_t>(engineHR), output.processQuerySucceeded,
                        memory, static_cast<uint32_t>(memoryHR), memoryOutput.processQuerySucceeded);
                    adapter->Release();
                }
                list->Release();
            }
            factory->Release();
        }
        FreeLibrary(module);
    }
    std::printf("DXCore factoryHR=0x%08X; budget is not selected-process memory\n", static_cast<uint32_t>(factoryStatus));
    PDH_HQUERY query = nullptr;
    const auto open = PdhOpenQueryW(nullptr, 0, &query);
    std::printf("PDH English GPU alternative open=0x%08X\n", static_cast<uint32_t>(open));
    if (open == ERROR_SUCCESS)
    {
        const wchar_t* paths[] = {L"\\GPU Engine(*)\\Utilization Percentage",
            L"\\GPU Process Memory(*)\\Dedicated Usage", L"\\GPU Process Memory(*)\\Shared Usage"};
        PDH_HCOUNTER counters[3]{};
        for (uint32_t i = 0; i < 3; ++i)
        {
            const auto status = PdhAddEnglishCounterW(query, paths[i], 0, &counters[i]);
            std::printf(" PDH add family=%u status=0x%08X\n", i, static_cast<uint32_t>(status));
            if (status != ERROR_SUCCESS) counters[i] = nullptr;
        }
        LONGLONG firstTime = 0, secondTime = 0;
        const auto first = PdhCollectQueryDataWithTime(query, &firstTime);
        Sleep(100);
        const auto second = PdhCollectQueryDataWithTime(query, &secondTime);
        std::printf(" PDH collect1=0x%08X collect2=0x%08X sampleBeginFILETIME100ns=%lld sampleEndFILETIME100ns=%lld\n",
            static_cast<uint32_t>(first), static_cast<uint32_t>(second), firstTime, secondTime);
        for (uint32_t i = 0; i < 3; ++i) if (counters[i]) PdhArray(counters[i], expected.pid, i);
        PdhCloseQuery(query);
    }
    const bool matched = Matches(process, expected);
    std::printf("GPU probe clock=2 begin=%llu end=%llu identityAfter=%u selectedGpuStatus=2 (unavailable; capability only); no parity claimed\n",
        begin, GetTickCount64() * 10000, matched);
}
}
