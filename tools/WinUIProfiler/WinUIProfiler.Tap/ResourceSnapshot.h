#pragma once
#include <windows.h>
#include <unknwn.h>
#include <wrl/client.h>
#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace WinUIProfiler::Tap::ResourceSnapshotAbi
{
    // Private XamlOM.WinUI.Private.idl ABI; keep the standalone TAP independent of product builds.
#pragma pack(push, 8)
    struct Resource
    {
        uint64_t TotalCreated;
        uint64_t TotalDestroyed;
        uint64_t LiveCount;
    };

    struct Snapshot
    {
        int64_t AllocCount;
        int64_t AllocSize;
        int64_t AllocatedMemory;
        int64_t OutstandingAllocCount;
        int64_t OutstandingAllocSize;
        Resource Resources[32];
    };
#pragma pack(pop)

    MIDL_INTERFACE("e79e0da1-cf7f-4fa6-b557-fa0de39add1d")
    DiagnosticsPrivate : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetResourceSnapShot(Snapshot* snapshot) = 0;
    };

    static_assert(sizeof(Resource) == 24 && offsetof(Resource, TotalCreated) == 0);
    static_assert(sizeof(Snapshot) == 808 && offsetof(Snapshot, Resources) == 40);
    static_assert(std::is_same_v<decltype(Snapshot::AllocCount), int64_t> &&
        std::is_same_v<decltype(Snapshot::AllocSize), int64_t> &&
        std::is_same_v<decltype(Snapshot::OutstandingAllocCount), int64_t> &&
        std::is_same_v<decltype(Snapshot::OutstandingAllocSize), int64_t>);

    inline HRESULT Read(IUnknown* diagnostics, Snapshot& snapshot) noexcept
    {
        snapshot = {};
        if (!diagnostics) return E_POINTER;
        Microsoft::WRL::ComPtr<DiagnosticsPrivate> resources;
        const HRESULT status = diagnostics->QueryInterface(IID_PPV_ARGS(&resources));
        if (FAILED(status)) return status;
        return resources->GetResourceSnapShot(&snapshot);
    }
}
