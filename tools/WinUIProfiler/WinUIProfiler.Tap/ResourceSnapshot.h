#pragma once
#include <windows.h>
#include <unknwn.h>
#include <wrl/client.h>
#include <cstddef>
#include <cstdint>

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
        uint64_t AllocCount;
        uint64_t AllocSize;
        int64_t AllocatedMemory;
        uint64_t OutstandingAllocCount;
        uint64_t OutstandingAllocSize;
        Resource Resources[32];
    };
#pragma pack(pop)

    MIDL_INTERFACE("2b2681be-7a8a-43c5-889f-374ce8cab061")
    Diagnostics3 : public IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetResourceSnapShot(Snapshot* snapshot) = 0;
    };

    static_assert(sizeof(Resource) == 24 && offsetof(Resource, TotalCreated) == 0);
    static_assert(sizeof(Snapshot) == 808 && offsetof(Snapshot, Resources) == 40);

    inline HRESULT Read(IUnknown* diagnostics, Snapshot& snapshot) noexcept
    {
        snapshot = {};
        if (!diagnostics) return E_POINTER;
        Microsoft::WRL::ComPtr<Diagnostics3> resources;
        const HRESULT status = diagnostics->QueryInterface(IID_PPV_ARGS(&resources));
        if (FAILED(status)) return status;
        return resources->GetResourceSnapShot(&snapshot);
    }
}
