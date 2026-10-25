#include "../WinUIProfiler.Native/Exports.h"
#include <Windows.h>
#include <initguid.h>
#include <dxcore.h>
#include <cstdio>
#include <cstring>
int main()
{
    HMODULE module = LoadLibraryExW(L"dxcore.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!module) { std::printf("DXCore live unavailable: load Win32=%lu\n", GetLastError()); return 0; }
    using Create = HRESULT(WINAPI*)(REFIID, void**);
    auto address = GetProcAddress(module, "DXCoreCreateAdapterFactory");
    Create create{};
    std::memcpy(&create, &address, sizeof(create));
    IDXCoreAdapterFactory* factory = nullptr;
    IDXCoreAdapterList* list = nullptr;
    IDXCoreAdapter* adapter = nullptr;
    HRESULT hr = create ? create(IID_PPV_ARGS(&factory)) : HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    if (SUCCEEDED(hr)) hr = factory->CreateAdapterList(1,
        &DXCORE_ADAPTER_ATTRIBUTE_D3D12_GRAPHICS, IID_PPV_ARGS(&list));
    if (SUCCEEDED(hr)) hr = list->GetAdapterCount() ? list->GetAdapter(0, IID_PPV_ARGS(&adapter)) : E_FAIL;
    if (SUCCEEDED(hr))
    {
        LUID luid{};
        hr = adapter->GetProperty(DXCoreAdapterProperty::InstanceLuid, &luid);
        if (SUCCEEDED(hr))
        {
            FILETIME creation{}, exit{}, kernel{}, user{};
            if (!GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) hr = E_FAIL;
            else
            {
                uint32_t high{};
                std::memcpy(&high, &luid.HighPart, sizeof(high));
                WupGpuMemoryRequest request{sizeof(request), 1,
                    {GetCurrentProcessId(), 0, (uint64_t(creation.dwHighDateTime) << 32) | creation.dwLowDateTime},
                    high, luid.LowPart, 0, 0};
                WupGpuMemoryResult result{};
                const auto dispatch = WupCaptureGpuMemory(GetCurrentProcess(), &request, &result, sizeof(result));
                std::printf("DXCore live one-adapter/phys0 (D3D12 discovery filter only): dispatch=%08X source=%u status=%u LUID=%08X:%08X PID=%u creation=%llu familyHR=%08X/%08X flags=%u Win32=%u committed=%llu/%llu resident=%llu/%llu bracket=%llu..%llu clock=%llu; NOT hardware/parity proof\n",
                    dispatch, result.source, result.status, high, luid.LowPart, request.process.pid,
                    request.process.creation_filetime_100ns, result.dedicated_hresult, result.shared_hresult,
                    result.process_query_flags, result.win32_error, result.dedicated_committed.value,
                    result.shared_committed.value, result.dedicated_resident.value, result.shared_resident.value,
                    result.acquisition_begin_100ns, result.acquisition_end_100ns, result.clock_id);
                if (dispatch != WUP_S_OK) hr = E_FAIL;
            }
        }
    }
    if (adapter) adapter->Release();
    if (list) list->Release();
    if (factory) factory->Release();
    FreeLibrary(module);
    std::printf("DXCore live discovery HRESULT=%08X; availability is separate from test assertions\n", uint32_t(hr));
    return FAILED(hr) ? 2 : 0;
}
