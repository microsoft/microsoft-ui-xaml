// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      An instance of a D3D11 Device.  If we are sharing devices, then multiple
//      CD3D11Device objects will use the same instance.  If not, then they will
//      get individual D3D11 Device instances (and all the related objects that
//      go along with them).

#include "precomp.h"

#include "D3D11DeviceInstance.h"
#include "D3D11SharedDeviceGuard.h"
#include "XamlTraceLogging.h"
#include <XcpAllocation.h>

#include <d3d11_1.h>
#include <dxgi1_2.h>

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"

#include <DependencyLocator.h>
#include <RuntimeEnabledFeatures.h>

using namespace RuntimeFeatureBehavior;
using namespace Microsoft::WRL;

typedef HRESULT (WINAPI *D3D11CREATEDEVICEFUNCTION)(
    _In_ IDXGIAdapter *pAdapter,
    _In_ D3D_DRIVER_TYPE DriverType,
    _In_ HMODULE Software,
    _In_ UINT Flags,
    _In_ const D3D_FEATURE_LEVEL *pFeatureLevels,
    _In_ UINT FeatureLevels,
    _In_ UINT SDKVersion,
    _Out_ ID3D11Device **ppDevice,
    _Out_ D3D_FEATURE_LEVEL *pFeatureLevel,
    _Out_ ID3D11DeviceContext **ppImmediateContext
    );

typedef HRESULT (WINAPI *CREATEDXGIFACTORY1FUNCTION)(
    _In_ REFIID riid,
    _Out_ void **ppFactory
    );

std::weak_ptr<CD3D11DeviceInstance> CD3D11DeviceInstance::s_sharedDeviceWeak;

wil::critical_section CD3D11DeviceInstance::s_sharedInstanceCriticalSection;

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the shared device instance if one exists and if not (or we are
//      quirked) we will create one.
//-------------------------------------------------------------------------
HRESULT CD3D11DeviceInstance::GetInstance(_Out_ std::shared_ptr<CD3D11DeviceInstance> & sharedDevice, bool useUniqueDevice)
{
    if (useUniqueDevice)
    {
        TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_CreateUnShared", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        sharedDevice = std::make_shared<CD3D11DeviceInstance>(false /* isSharedDevice */);
        return S_OK;
    }

    // We are going to be a shared device so ensure that we take the shared
    // device lock
    wil::cs_leave_scope_exit guard = s_sharedInstanceCriticalSection.lock();

    // If our weak reference still points to a valid device, then we will use it.
    sharedDevice = s_sharedDeviceWeak.lock();
    if (sharedDevice != nullptr)
    {
        TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_UseShared", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        return S_OK;
    }

#if XCP_MONITOR
    // s_sharedDeviceWeak is a global weak_ptr. This is fine, as it will clean up the storage as the process is destroyed
    // However, this teardown occurs after our leak checker runs, resulting in false positives in tests.
    // To bypass this, we use the LeakIgnoringAllocator for the smart pointer's internal ref count
    // but still use conventional allocation for the actual object to keep it in the leak check
    auto temp = new CD3D11DeviceInstance(TRUE /* isSharedDevice */);
    sharedDevice.reset(temp, [](CD3D11DeviceInstance* obj) { delete obj; }, XcpAllocation::LeakIgnoringAllocator<CD3D11DeviceInstance>());
#else
    sharedDevice = std::make_shared<CD3D11DeviceInstance>(true /* isSharedDevice */);
#endif

    s_sharedDeviceWeak = sharedDevice;
    TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_CreateShared", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//-------------------------------------------------------------------------
CD3D11DeviceInstance::CD3D11DeviceInstance(bool isSharedDevice)
    : m_pDevice(NULL)
    , m_pDeviceContext(NULL)
    , m_pD2DFactory(NULL)
    , m_pD2DMultithread(NULL)
    , m_pD3DMultithread(NULL)
    , m_featureLevel(D3D_FEATURE_LEVEL_9_1)
    , m_fIsWarpDevice(FALSE)
    , m_fIsHardwareOutput(FALSE)
    , m_fUseIntermediateUploadSurface(FALSE)
    , m_isSharedDevice(isSharedDevice)
    , m_pDXGIDevice2(NULL)
    , m_pD2DDevice(NULL)
{
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//-------------------------------------------------------------------------
CD3D11DeviceInstance::~CD3D11DeviceInstance()
{
    // Remove all system memory surfaces from the pool
    while (!m_sysMemBitsPool.IsEmpty())
    {
        SystemMemoryBits *pSurface = m_sysMemBitsPool.GetHead(SystemMemoryBits::PoolLinkOffset());

        m_sysMemBitsPool.Remove(pSurface->GetPoolLink());

        // This should be the final release
        UINT32 refCount = pSurface->Release();
        ASSERT(refCount == 0);
        UNREFERENCED_PARAMETER(refCount);
    }

    ReleaseInterface(m_pDeviceContext);
    ReleaseInterface(m_pDevice);
    ReleaseInterface(m_pD2DFactory);
    ReleaseInterface(m_pD2DDevice);
    ReleaseInterface(m_pD2DMultithread);
    ReleaseInterface(m_pD3DMultithread);

    ReleaseInterface(m_pDXGIDevice2);
    ReleaseInterface(m_pD2DDevice);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an object that deals with the refresh rate.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
    CD3D11DeviceInstance::GetRefreshRateInfo(
    _Outptr_ IPALRefreshRateInfo **ppRefreshRateInfo
    )
{
    HRESULT hr = S_OK;

    wil::cs_leave_scope_exit guard = m_lock.lock();

    ComPtr<IDXGIOutput> primaryOutput;
    xref_ptr<RefreshRateInfo> refreshRateInfo;

    // Ensure the primary adapter is up-to-date.
    IFC(EnsureDXGIAdapters());

    // If we are no longer the current shared device, then return a device lost status.  This
    // duplicates a check that is in CheckForStaleD3DDevice (which is called by EnsureDXGIAdapters),
    // but if another thread refreshes the DXGI factory before us, then we won't go through
    // that code.  However, if another thread has changed out the device (due to it being stale)
    // we want to start using that new device.
    if (m_isSharedDevice && GetSharedDeviceInstance().get() != this)
    {
        TraceCheckForStaleDxgiDeviceInfo(DXGI_ERROR_DEVICE_RESET, true);
        IFC(DXGI_ERROR_DEVICE_RESET);
    }

    if (m_hardwareAdapter)
    {
        // The DWM composes using the primary display's vBlank, so our composition should keep
        // the same beat.
        hr = m_hardwareAdapter->EnumOutputs(
            0 /*primary display*/,
            &primaryOutput
            );
    }
    else if (m_warpAdapter)
    {
        // If we don't have a hardware adapter, try to use the warp adapter
        hr = m_warpAdapter->EnumOutputs(
            0 /*primary display*/,
            &primaryOutput
            );
    }

    // If all displays are turned off or disconnected, it's possible for no outputs to be
    // found. The RefreshRateInfo wrapper will simulate a vBlank in this case. When a display
    // is reconnected or powered back on, the DXGIFactory->IsCurrent will return false and
    // this method will be need to be called again to re-enumerate outputs.
    if (hr == DXGI_ERROR_NOT_FOUND)
    {
        hr = S_OK;
    }
    else
    {
        IFC(hr);
    }

    IFC(RefreshRateInfo::Create(
        primaryOutput.Get(),
        m_dxgiFactory.Get(),
        refreshRateInfo.ReleaseAndGetAddressOf()
        ));

    *ppRefreshRateInfo = refreshRateInfo.detach();

Cleanup:
    return hr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure the D3D11 device and other state is initialized.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::EnsureResources()
{
    HRESULT hr = S_OK;

    CD3D11SharedDeviceGuard guard;
    if (m_isSharedDevice)
    {
        guard.TakeLock(&m_lock);
    }

    //
    // Ensure we have a D3D11 device. Also ensure we aren't marked as a CD3D11DeviceInstance that has released its
    // lost D3D resources. This can happen if a new CD3D11Device picks up the shared CD3D11DeviceInstance right before
    // the instance is marked as lost:
    //
    //   1. Thread A creates a CD3D11Device, which picks up the existing shared CD3D11DeviceInstance X.
    //
    //   2. Thread A spins up a worker thread to call CD3D11DeviceInstance::EnsureResources on X.
    //
    //   3. Thread B detects CD3D11DeviceInstance X as lost. It goes through recovery, which clears out the shared
    //      CD3D11DeviceInstance X and marks it with m_wasLostDeviceReleased. It creates a new shared CD3D11DeviceInstance
    //      Y, but thread A has already picked up the old shared instance X in step 1.
    //
    //   4. Thread A's worker thread gets here and sees no m_pDevice inside the old instance X. It repopulates the
    //      CD3D11DeviceInstance that was just cleared out. This creates a duplicate set of D3D and D2D objects.
    //
    //   5. Thread A goes on using the rehydrated CD3D11DeviceInstance X. It should switch over to instance Y instead.
    //
    // The problem here is that thread A got to the shared instance a little too early - right before we were about to
    // clear it out. Instead, we'll have step 4 detect the lost instance and no-op, leaving thread A with an empty
    // CD3D11DeviceInstance wrapper. This is tolerated - if threads A and B shared a CD3D11DeviceInstance to begin
    // with, thread B can recover first which leaves thread A with an emptied out instance anyway. When thread A tries
    // to do anything with its CD3D11DeviceInstance, it will find that the device has been lost and recover to use the
    // new CD3D11DeviceInstance Y. It won't toss Y and create another instance Z, because it detects that the instance
    // it has is no longer the shared instance.
    //
    if (!m_pDevice && !m_wasLostDeviceReleased)
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        ComPtr<IDXGIAdapter1> warpAdapter;

        TraceCreateGraphicsDeviceBegin();

        // TODO: HWPC: We don't have fallback for blocked drivers yet - waiting on a DCR from D3D.
        // TODO: HWPC: Do we want to support a block list at all?  Probably not the same as SL?

        while (true)
        {
            {
                // The lock scope is reduced as far as possible here to minimize contention between the composition thread,
                // used for scheduling, and the background thread used to initialize expensive resources.
                 wil::cs_leave_scope_exit guard2 = m_lock.lock();

                // Devices are always created with the hardware adapter first with a fallback to warp.
                // NOTE: We detect hardware adapter changes for refresh-rate purposes, but do _not_ recreate our devices
                //       in direct response to those changes.
                IFC(EnsureDXGIAdapters());

                hardwareAdapter = m_hardwareAdapter;
                warpAdapter = m_warpAdapter;
            }

            // We sometimes failfast here in ShellExperienceHost when running with FailFastOnStowedExceptionEnabled() true (TFS 6847319).
            // Error HR is handled manually here, and is used to implement retrying with a WARP device if HW D3D device creation fails.
            // We need to always preserve this behavior, so suspend FailFastOnStowedException around the calls.
            {
                SuspendFailFastOnStowedException suspender;
                static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

                bool isDebugDevice = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableDebugD3DDevice);

                // Attempt to open the hardware adapter if we have one.
                // For CloverTrailPC, there is a bug in graphics driver causing HW rendering issues, fallback to warp on that device.
                SYSTEM_INFO systemInfo;
                ::GetSystemInfo(&systemInfo);
                bool isCloverTrailPC = systemInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL
                    && systemInfo.wProcessorLevel == 6
                    && ((systemInfo.wProcessorRevision >> 8) & 0xff) == 0x35;

                if (hardwareAdapter && !isCloverTrailPC && !runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ForceWarp))
                {
                    hr = CreateGraphicsDeviceFromAdapter(hardwareAdapter.Get(), false /* isWarpAdapter */, false /* isVideo */, isDebugDevice);
                    if (SUCCEEDED(hr))
                    {
                        break;
                    }
                }

                // Try to fall back to warp
                if (warpAdapter)
                {
                    hr = CreateGraphicsDeviceFromAdapter(warpAdapter.Get(), true /* isWarpAdapter */, false /* isVideo */, isDebugDevice);
                    if (SUCCEEDED(hr))
                    {
                        break;
                    }
                }
            }

            // If we still don't have a device and our factory is still current, go ahead and report
            // our last error since it doesn't seem like we can open a device.
            if (m_dxgiFactory->IsCurrent())
            {
                IFC(hr);
            }
        }

        IFC(InitializeState());

        IFC(m_pDevice->QueryInterface(__uuidof(IDXGIDevice2), reinterpret_cast<void**>(&m_pDXGIDevice2)));

        // Get the multithread interface for lock in order to synchronize with DComp
        IFC(m_pDevice->QueryInterface(__uuidof(ID3D10Multithread), reinterpret_cast<void**>(&m_pD3DMultithread)));

        TraceCreateGraphicsDeviceEnd();
    }

Cleanup:

    return hr;
}

void CD3D11DeviceInstance::ReleaseResourcesOnDeviceLost()
{
    // We should have already entered the critical section in RecordDeviceAsLost. Guard against this function
    // being used elsewhere by entering the CS again.
    wil::cs_leave_scope_exit guard = m_lock.lock();
    ReleaseInterface(m_pDeviceContext);
    ReleaseInterface(m_pDevice);
    ReleaseInterface(m_pD2DDevice);
    ReleaseInterface(m_pD2DMultithread);
    ReleaseInterface(m_pD3DMultithread);
    m_d2dDeviceContexts.clear();
    m_d2dSolidColorBrushes.clear();
    ReleaseInterface(m_pDXGIDevice2);
    // m_pD2DFactory doesn't hold any device-dependent resources. It can be left alone.
    m_wasLostDeviceReleased = true;
}

_Check_return_ HRESULT CD3D11DeviceInstance::TakeLockAndCheckDeviceLost(_In_ CD3D11SharedDeviceGuard* guard)
{
    FAIL_FAST_ASSERT(!guard->HasLock() && guard->LockMatches(nullptr));
    guard->TakeLock(&m_lock);

    if (IsDeviceLost())
    {
        return DXGI_ERROR_DEVICE_REMOVED;
    }

    return S_OK;
}

void CD3D11DeviceInstance::EnsureLockTaken(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    FAIL_FAST_ASSERT(guard->HasLock() && guard->LockMatches(&m_lock));
}

_Ret_notnull_ ID3D11Device* CD3D11DeviceInstance::GetDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    EnsureLockTaken(guard);
    ASSERT(m_pDevice != NULL);
    return m_pDevice;
}

CLockedGraphicsPointer<ID3D11DeviceContext> CD3D11DeviceInstance::GetLockedDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    EnsureLockTaken(guard);
    ASSERT(m_pDeviceContext != NULL);
    CLockedGraphicsPointer<ID3D11DeviceContext> pointerWithLock(m_pDeviceContext, m_pD3DMultithread, m_pD2DMultithread);
    return pointerWithLock;
}

_Ret_maybenull_ IDXGIDevice2* CD3D11DeviceInstance::GetDXGIDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    EnsureLockTaken(guard);
    return m_pDXGIDevice2;
}

_Ret_maybenull_ ID2D1Device* CD3D11DeviceInstance::GetD2DDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    EnsureLockTaken(guard);
    return m_pD2DDevice;
}

_Ret_maybenull_ ID3D11Device* CD3D11DeviceInstance::TestHook_GetDevice() const
{
    return m_pDevice;
}

_Ret_maybenull_ CD2DFactory* CD3D11DeviceInstance::GetD2DFactory() const
{
    return m_pD2DFactory;
}

_Ret_maybenull_ ID2D1DeviceContext* CD3D11DeviceInstance::GetD2DDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard, _In_ const CD3D11Device* deviceWrapper) const
{
    EnsureLockTaken(guard);
    const auto& searchResult = m_d2dDeviceContexts.find(deviceWrapper);
    if (searchResult != m_d2dDeviceContexts.end())
    {
        return searchResult->second.Get();
    }
    return nullptr;
}

_Ret_maybenull_ ID2D1SolidColorBrush* CD3D11DeviceInstance::GetD2DSolidColorBrush(_In_ const CD3D11SharedDeviceGuard* guard, _In_ const CD3D11Device* deviceWrapper) const
{
    EnsureLockTaken(guard);
    const auto& searchResult = m_d2dSolidColorBrushes.find(deviceWrapper);
    if (searchResult != m_d2dSolidColorBrushes.end())
    {
        return searchResult->second.Get();
    }
    return nullptr;
}

_Check_return_ HRESULT
CD3D11DeviceInstance::CreateGraphicsDeviceFromAdapter(_In_ IDXGIAdapter1* adapter, bool isWarpAdapter, bool isVideo, bool isDebugDevice)
{
    HRESULT hr = S_OK;

    m_fIsWarpDevice = isWarpAdapter;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT |
                (isWarpAdapter ? 0 : D3D11_CREATE_DEVICE_PREVENT_INTERNAL_THREADING_OPTIMIZATIONS) |
                (isVideo ? D3D11_CREATE_DEVICE_VIDEO_SUPPORT : 0) |
                (isDebugDevice ? D3D11_CREATE_DEVICE_DEBUG : 0);

    // Load D3D11 so we can create the device and device context.
    static HMODULE hD3D11Module = nullptr;
    static D3D11CREATEDEVICEFUNCTION pfnD3D11CreateDevice;
    if (hD3D11Module == nullptr)
    {
        IFCW32(hD3D11Module = LoadLibraryEx(L"d3d11.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        pfnD3D11CreateDevice = reinterpret_cast<D3D11CREATEDEVICEFUNCTION>(GetProcAddress(hD3D11Module, "D3D11CreateDevice"));
        IFCPTR(pfnD3D11CreateDevice);
    }

    static const D3D_FEATURE_LEVEL levelAttempts[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    ASSERT(m_pDeviceContext == NULL);

    // NOTES:
    // - The ID3D11Device is thread-safe by default.
    // - BGRA support is needed for D2D interop.  However, successfully creating the device
    //   doesn't mean BGRA is fully supported - on 9.1 devices you cannot set BGRA formats
    //   for vertex data in the input layout.
    IFC(pfnD3D11CreateDevice(
        adapter,
        adapter == nullptr ? D3D_DRIVER_TYPE_HARDWARE : D3D_DRIVER_TYPE_UNKNOWN, // If we provide an adapter, this should be unknown
        NULL,   // For software rasterization only
        flags,
        levelAttempts,
        ARRAYSIZE(levelAttempts),
        D3D11_SDK_VERSION,
        &m_pDevice,
        &m_featureLevel,
        &m_pDeviceContext
        ));

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize underlying D3D device state.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::InitializeState()
{
    ASSERT(m_pDevice != NULL && m_pDeviceContext != NULL);

    // Name Device
    IFC_RETURN(m_pDevice->SetPrivateData(
        WKPDID_D3DDebugObjectName, /* REFGUID guid */
        sizeof(D3D11DeviceName) - 1, /* UINT Data Size */
        D3D11DeviceName /* const void* pData */
        ));

    // Determine the LUID of the adapter
    LUID adapterLuid;
    IFC_RETURN(GetDeviceAdapterLuid(
        m_pDevice,
        &adapterLuid
        ));

    // Save the LUID in m_adapterLuid
    // After this point, the UI Thread may read this value
    m_adapterLuid.set(&adapterLuid);

    // Check if we want to do multi-stage texture uploads.  This should never be used for WARP.
    if (!m_fIsWarpDevice && (m_featureLevel < D3D_FEATURE_LEVEL_10_0))
    {
        // There was a time in the past we queried the device caps as well to see if we should use an intermediate
        // surface for better perf.   But at this point (2019), devices that are FL9 and wouldn’t benefit from this
        // optimization are exceedingly rare, so we just check for FL9.
        m_fUseIntermediateUploadSurface = true;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
// Synopsis:
//     Does the work to create the D2D Factory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::EnsureD2DFactory()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (m_pD2DFactory != NULL) return S_OK;

    IFC_RETURN(CD2DFactory::Create(&m_pD2DFactory));
    return S_OK;
}

//------------------------------------------------------------------------
//
// Synopsis:
//     Does the work to create the D2D device and related resources.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::EnsureD2DResources()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    IFC_RETURN(EnsureD2DFactory());

    if (IsDeviceLost())
    {
        return DXGI_ERROR_DEVICE_REMOVED;
    }

    if (m_pD2DDevice != nullptr) return S_OK;

    ComPtr<IDXGIDevice> dxgiDevice;
    IFC_RETURN(m_pDevice->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));

    IFC_RETURN(m_pD2DFactory->GetFactory()->CreateDevice(dxgiDevice.Get(), &m_pD2DDevice));

    // Because D2D doesn't take the D3D lock, we need to take the D2D lock around our D3D calls
    IFC_RETURN(m_pD2DFactory->GetFactory()->QueryInterface(__uuidof(ID2D1Multithread), reinterpret_cast<void**>(&m_pD2DMultithread)));

    return S_OK;
}

_Check_return_ HRESULT CD3D11DeviceInstance::EnsureDeviceSpecificD2DResources(_In_ const CD3D11Device* deviceWrapper)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (IsDeviceLost())
    {
        return DXGI_ERROR_DEVICE_REMOVED;
    }

    if (m_d2dDeviceContexts.find(deviceWrapper) == m_d2dDeviceContexts.end())
    {
        wrl::ComPtr<ID2D1DeviceContext> d2dDeviceContext;
        IFC_RETURN(m_pD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDeviceContext));
        m_d2dDeviceContexts[deviceWrapper] = d2dDeviceContext;

        wrl::ComPtr<ID2D1SolidColorBrush> d2dSolidColorBrush;
        IFC_RETURN(d2dDeviceContext->CreateSolidColorBrush(
            &D2D1::ColorF(D2D1::ColorF::Black),
            &D2D1::BrushProperties(),
            &d2dSolidColorBrush));
        m_d2dSolidColorBrushes[deviceWrapper] = d2dSolidColorBrush;
    }
    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures that m_hardwareAdapter (primary) and m_warpAdapter
//      (fallback) will be initialized and ensures that m_dxgiFactory
//      will be initalized
//
//      NOTE: Callers must hold m_lock.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::EnsureDXGIAdapters()
{
    HRESULT hr = S_OK;

    // If we already have a factory and it is current, then we already have
    // our adapters.
    if (m_dxgiFactory != nullptr && m_dxgiFactory->IsCurrent())
    {
        return m_hardwareAdapter != nullptr || m_warpAdapter != nullptr ? S_OK : DXGI_ERROR_NOT_FOUND;
    }

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    ComPtr<IDXGIAdapter1> warpAdapterWithOutputs;
    ComPtr<IDXGIAdapter1> warpAdapterWithNoOutputs;
    ComPtr<IDXGIAdapter1> enumeratedAdapter;

    m_hardwareAdapter.Reset();
    m_warpAdapter.Reset();
    m_dxgiFactory.Reset();

    // Get a current factory
    static HMODULE hDXGIModule = nullptr;
    static CREATEDXGIFACTORY1FUNCTION pfnCreateDXGIFactory1;
    if (hDXGIModule == nullptr)
    {
        IFCW32(hDXGIModule = LoadLibraryEx(L"dxgi.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));
        pfnCreateDXGIFactory1 = reinterpret_cast<CREATEDXGIFACTORY1FUNCTION>(GetProcAddress(hDXGIModule, "CreateDXGIFactory1"));
        IFCPTR(pfnCreateDXGIFactory1);
    }
    IFC(pfnCreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void **>(m_dxgiFactory.ReleaseAndGetAddressOf())));

    // Find the best hardware and warp adapters available
    m_fIsWarpDevice = false;
    m_fIsHardwareOutput = false;

    for (UINT i = 0; SUCCEEDED(m_dxgiFactory->EnumAdapters1(i, enumeratedAdapter.ReleaseAndGetAddressOf())); i++)
    {
        DXGI_ADAPTER_DESC1 enumeratedAdapterDesc;

        enumeratedAdapter->GetDesc1(&enumeratedAdapterDesc);

        const bool isWarpDevice =
            enumeratedAdapterDesc.VendorId == WARP_VENDOR_ID
            && enumeratedAdapterDesc.DeviceId == WARP_DEVICE_ID;

        TRACE(TraceAlways, L"enumerated adapter 0x%08x 0x%08x", enumeratedAdapterDesc.VendorId, enumeratedAdapterDesc.DeviceId);

        if (isWarpDevice)
        {
            ComPtr<IDXGIOutput> enumeratedOutput;
            if (SUCCEEDED(enumeratedAdapter->EnumOutputs(0, &enumeratedOutput)))
            {
                //
                // Found a WARP adapter with outputs -- this is what DComp will
                // be using, so let's stick to it!
                //
                if (!warpAdapterWithOutputs)
                {
                    TRACE(TraceAlways, L"found primary WARP adapter 0x%08x 0x%08x", enumeratedAdapterDesc.VendorId, enumeratedAdapterDesc.DeviceId);
                    warpAdapterWithOutputs = enumeratedAdapter;
                }
            }
            else if (!warpAdapterWithNoOutputs)
            {
                //
                // Found a WARP adapter, but with no outputs -- keep onto this
                // one, but keep searching.
                //
                ASSERT(nullptr == enumeratedOutput);
                TRACE(TraceAlways, L"found secondary WARP adapter 0x%08x 0x%08x", enumeratedAdapterDesc.VendorId, enumeratedAdapterDesc.DeviceId);
                warpAdapterWithNoOutputs = enumeratedAdapter;
            }
        }
        else
        {
            //
            // Actual hardware device -- this is what we were asked to use!
            //
            if (!hardwareAdapter)
            {
                TRACE(TraceAlways, L"found primary HW adapter 0x%08x 0x%08x", enumeratedAdapterDesc.VendorId, enumeratedAdapterDesc.DeviceId);
                hardwareAdapter = enumeratedAdapter;
            }
        }
    }

    if (!hardwareAdapter && !warpAdapterWithOutputs && !warpAdapterWithNoOutputs)
    {
        IFC(DXGI_ERROR_NOT_FOUND);
    }

    m_hardwareAdapter = hardwareAdapter;
    m_fIsHardwareOutput = m_hardwareAdapter != nullptr;

    m_warpAdapter = warpAdapterWithOutputs ? warpAdapterWithOutputs : warpAdapterWithNoOutputs;

    // See if our current device (if we have one) is still valid
    if (m_pDevice)
    {
        // Using FailFastOnStowedException here, since we don't want to FailFast due to
        // a device lost error in either of these calls.
        SuspendFailFastOnStowedException suspender;

        IFC(m_pDevice->GetDeviceRemovedReason());

        IFC(CheckForStaleD3DDevice());
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Determine whether the current D3D Device is stale based upon the
//      the latest DXGI adapters.
//
//      NOTE: Callers must hold m_lock.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::CheckForStaleD3DDevice()
{
    HRESULT hr = S_OK;

    // If our device is no longer the current shared device, then some other thread has
    // already created a new device so we are definitely stale.
    if (m_isSharedDevice && GetSharedDeviceInstance().get() != this)
    {
        TraceCheckForStaleDxgiDeviceInfo(DXGI_ERROR_DEVICE_RESET, false);
        return DXGI_ERROR_DEVICE_RESET;
    }

    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    // Check to see if the current device uses the same adapter that is being requested.
    bool useWarp = m_hardwareAdapter == nullptr || runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ForceWarp);
    ComPtr<IDXGIAdapter1> adapter = useWarp ? m_warpAdapter : m_hardwareAdapter;

    DXGI_ADAPTER_DESC adapterDescription;
    IFC(adapter->GetDesc(&adapterDescription));
    if (IsOnSameAdapter(&adapterDescription.AdapterLuid))
    {
        // the current devices matches the target adapter so our current
        // device is still valid.
        TraceCheckForStaleDxgiDeviceInfo(S_OK, false);
        return S_OK;
    }

    // Make sure that we can create a D3D Device from the adapter.  Note: This is a bit wasteful because we create the
    // device here, but never use it.  The issue is that we don't have any place to actually put it.  We can't put it
    // into a static because we may not be sharing devices.  We can't put it in graphics manager or core, because we
    // may be sharing devices and those are unique per thread.  Ideally, we would have one CD3D11DeviceInstance per
    // thread that wants one (or one global one if shared) and we would update the D3D/D2D resources within it.
    // However, that would require a significant reworking of a bunch of code and its not currently budgeted. Since we don't
    // expect this scenario to occur very often (only once when a video driver is installed or upgraded), we will just
    // take the hit for the extra device creation.  The other slightly odd thing here is that we create a new instance
    // of CD3D11DeviceInstance since CreateGraphicsDeviceFromAdapter uses member variables, but all of that is discarded
    // after the check.
    {
        std::unique_ptr<CD3D11DeviceInstance> newDevice(new CD3D11DeviceInstance(false /* isSharedDevice */));
        IFC(newDevice->CreateGraphicsDeviceFromAdapter(adapter.Get(), useWarp, false /* isVideo */, false /* isDebugDevice */));
    }

    // We succeeded at creating the new D3D Device, so our current one must be stale. Mark the current one as lost and
    // send back a device lost status.
    RecordDeviceAsLost();
    TraceCheckForStaleDxgiDeviceInfo(DXGI_ERROR_DEVICE_REMOVED, false);
    return DXGI_ERROR_DEVICE_REMOVED;

Cleanup:
    // Any fatal errors that we get out of this will be ignored since we already have a device.  We will just continue
    // to use it until (if ever) it reports itself as lost.
    TraceCheckForStaleDxgiDeviceInfo(hr, false);
    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize ID3D11VideoDevice for video playback.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::InitializeVideoDevice()
{
    ComPtr<ID3D10Multithread> multithread;
    bool isWarp = false;
    bool isDebugDevice = false;

    {
        wil::cs_leave_scope_exit guard = m_lock.lock();

        IFC_RETURN(EnsureDXGIAdapters());

        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        // Note, the use of the ForceWarp feature here is probably not right, since we end up creating the device with no
        // specific adapter means it will probably be hardware, but could be warp.  The use of the ForceWarp feature here
        // is to maintain compatibility with previous code.
        isWarp = m_hardwareAdapter == nullptr || runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ForceWarp);
        isDebugDevice = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::EnableDebugD3DDevice);
    }

    IFC_RETURN(CreateGraphicsDeviceFromAdapter(nullptr, isWarp, true /* isVideo */, isDebugDevice));

    // seems odd that you have to opt into thread safety!
    IFC_RETURN(m_pDevice->QueryInterface(__uuidof(ID3D10Multithread), (void**)&multithread));
    multithread->SetMultithreadProtected(TRUE);

    return S_OK;
}

static bool IsHdrOutput(_In_ HMONITOR monitor, _In_ IDXGIFactory1* dxgiFactory)
{
    for (UINT adapterIndex = 0; ; adapterIndex++)
    {
        ComPtr<IDXGIAdapter> adapter;
        HRESULT hr = dxgiFactory->EnumAdapters(adapterIndex, &adapter);
        if (hr == DXGI_ERROR_NOT_FOUND)
        {
            break;
        }
        IFCFAILFAST(hr);

        for (UINT outputIndex = 0; ; outputIndex++)
        {
            ComPtr<IDXGIOutput> output;
            hr = adapter->EnumOutputs(outputIndex, &output);
            if (hr == DXGI_ERROR_NOT_FOUND)
            {
                break;
            }
            IFCFAILFAST(hr);

            ComPtr<IDXGIOutput6> output6;
            if (SUCCEEDED(output.As(&output6)))
            {
                DXGI_OUTPUT_DESC1 outputDesc;
                IFCFAILFAST(output6->GetDesc1(&outputDesc));

                if (monitor == outputDesc.Monitor)
                {
                    return (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P2020) || // wcg
                        (outputDesc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020); // hdr
                }
            }
        }
    }

    return false;
}

_Check_return_ HRESULT CD3D11DeviceInstance::IsHdrOutput(_In_ HMONITOR monitor, _Out_ bool *isHDR)
{
    *isHDR = false;

    wil::cs_leave_scope_exit guard = m_lock.lock();

    // Ensure the primary adapter is up-to-date.
    IFC_RETURN(EnsureDXGIAdapters());

    *isHDR = ::IsHdrOutput(monitor, m_dxgiFactory.Get());

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread.  Returns TRUE if the adapter LUID for this D3D device
//      matches the passed in adapter LUID.  Note that this can be called before the
//      render thread has initialized the D3D device, in which case this returns FALSE
//
//-------------------------------------------------------------------------
bool
CD3D11DeviceInstance::IsOnSameAdapter(
_In_ const LUID *pOtherLuid
)
{
    bool result = false;
    LUID thisLuid;

    if (m_adapterLuid.get(&thisLuid))
    {
        // Render thread has initialized the D3D device
        result = (0 == memcmp(&thisLuid, pOtherLuid, sizeof(LUID)));
    }

    return result;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread after each frame.
//      Releases up to 1 system memory surface that has not been used in a long time.
//
//------------------------------------------------------------------------------
void
CD3D11DeviceInstance::TrimMemory(const UINT64 releaseTime)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    // Loop through the pool looking for surfaces that have been in the pool for a long time.
    IntrusiveList<SystemMemoryBits>::Iterator it = m_sysMemBitsPool.Begin(SystemMemoryBits::PoolLinkOffset());
    while (it != m_sysMemBitsPool.End())
    {
        SystemMemoryBits *pPooledSurface = *it;

        // Move the iterator to the next ptr in the list, since the current entry might be removed.
        ++it;

        if ((pPooledSurface->GetTimeStamp()) < releaseTime)
        {
            // pPooledSurface has been in the pool too long
            m_sysMemBitsPool.Remove(pPooledSurface->GetPoolLink());

            // This should be the final release
            UINT32 refCount = pPooledSurface->Release();
            ASSERT(refCount == 0);
            UNREFERENCED_PARAMETER(refCount);
        }
    }

}

std::shared_ptr<CD3D11DeviceInstance> CD3D11DeviceInstance::GetSharedDeviceInstance()
{
    wil::cs_leave_scope_exit guard = s_sharedInstanceCriticalSection.lock();
    std::shared_ptr<CD3D11DeviceInstance> device = s_sharedDeviceWeak.lock();
    return device;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Records the current device as lost
//
//------------------------------------------------------------------------------
void
CD3D11DeviceInstance::RecordDeviceAsLost()
{
    TraceRecordDeviceAsLostInfo();

    if (m_isSharedDevice)
    {
        //
        // This shared CD3D11DeviceInstance is lost. We want to null out the shared CD3D11DeviceInstance (which
        // requires taking the static s_sharedInstanceCriticalSection lock) and also clear out the D3D objects
        // inside this old shared instance (which requires taking the m_lock). Do these separately so we don't
        // hold one lock while trying to take another.
        //
        bool releaseResources = false;

        //
        // We were the shared CD3D11DeviceInstance, so check to see if we still are.
        //
        // If so, reset the static weak pointer so the next device request will generate a new CD3D11DeviceInstance
        // with valid a D3D device. Also release the D3D device and DX resources inside this wrapper to allow device
        // lost recovery to succeed.
        //
        // If not, it means someone else already reset the static shared CD3D11DeviceInstance weak pointer and cleared
        // out this CD3D11DeviceInstance while this thread was swapped out. There's no reason to reset the static weak
        // pointer again. The CD3D11DeviceInstance that it now points to was freshly created and should be valid. Just
        // switch over to it when we recover.
        //
        {
            wil::cs_leave_scope_exit guard = s_sharedInstanceCriticalSection.lock();
            std::shared_ptr<CD3D11DeviceInstance> device = s_sharedDeviceWeak.lock();

            if (device.get() == this)
            {
                s_sharedDeviceWeak.reset();
                releaseResources = true;
            }
        }

        if (releaseResources)
        {
            //
            // In processes with multiple Xaml UI threads (e.g. StartMenuExperienceHost.exe), each thread has its own
            // CD3D11Device, but they share the same CD3D11DeviceInstance. When we encounter a device lost as part of
            // a driver upgrade, the first thread to do anything will encounter device lost and will discard its
            // CD3D11Device, but the CD3D11DeviceInstance will stay alive and keep the D3D device alive as long as
            // there are other threads that haven't cleaned up their CD3D11Devices. This creates a problem for the
            // thread trying to recover from device lost, because the old user mode driver will be kept open as long
            // as there are still outstanding D3D devices. As long as the old user mode driver is kept open, we can't
            // make a new D3D device.
            //
            // To get around this problem, we must fully release the underlying D3D device before any thread attempts
            // to create a new device. We do that by having the first thread to attempt cleanup reach into the
            // CD3D11DeviceInstance and release all underlying D3D resources. Subsequent threads that try to recover
            // will find that their device instance is now empty, so they can just release the CD3D11Device and
            // create a new one. Creating the CD3D11Device will also create a new CD3D11DeviceInstance, which will
            // find the cached one created by the first thread to recover.
            //
            ReleaseResourcesOnDeviceLost();

            TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_SharedLost", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        }
        else
        {
            // We're not the first thread to encounter a device lost. The first thread already released the D3D resources
            // inside this CD3D11DeviceInstance. We can just no-op here.

            TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_PreviouslySharedLost", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
        }
    }
    else
    {
        TraceLoggingWrite(g_hTraceProvider, "SharedD3DDevice_NonSharedLost", TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance), TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));
    }
}

bool CD3D11DeviceInstance::IsDeviceLost()
{
    // Default to false - an uninitialized CD3D11DeviceInstance isn't lost
    bool result = false;

    // This test hook forces the CD3D11DeviceInstance to return device lost, but it lives in the same code path
    // as the product code that we're trying to test. Reset the flag after having it return a device lost error
    // to revert to product code behavior. Otherwise the device lost recovery code could be broken yet the test
    // could still pass because of this test hook.
    if (m_testHook_IsDeviceLost)
    {
        result = true;
        m_testHook_IsDeviceLost = false;
    }

    if (m_pDevice != nullptr)
    {
        ASSERT(!m_wasLostDeviceReleased);

        if (FAILED(m_pDevice->GetDeviceRemovedReason()))
        {
            result = true;
        }
    }
    else if (m_wasLostDeviceReleased)
    {
        // If we've started device lost recovery by releasing the old device, then the device is considered lost.
        // The m_wasLostDeviceReleased differentiates this case from having an uninitialized CD3D11DeviceInstance.
        result = true;
    }

    return result;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread.
//      Releases up all system memory surfaces in the pool.
//      Also notifies D2D and drivers to give up intermediate
//      scratch/staging allocations.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::ReleaseScratchResources()
{
    HRESULT hr = S_OK;
    IDXGIDevice3 *pDXGIDevice3 = NULL;

    wil::cs_leave_scope_exit guard = m_lock.lock();

    while (!m_sysMemBitsPool.IsEmpty())
    {
        SystemMemoryBits *pSurface = m_sysMemBitsPool.GetHead(SystemMemoryBits::PoolLinkOffset());

        m_sysMemBitsPool.Remove(pSurface->GetPoolLink());

        // This should be the final release
        UINT32 refCount = pSurface->Release();
        ASSERT(refCount == 0);
        UNREFERENCED_PARAMETER(refCount);
    }

    // Notify the driver to release all the unused scratch surfaces.
    // This affects the actual memory commit size rather than just the working set.
    if (m_pDevice != NULL)
    {
        IFC(m_pDevice->QueryInterface(__uuidof(IDXGIDevice3), reinterpret_cast<void**>(&pDXGIDevice3)));

        // Trim can access the device context so take a lock on it.
        CD3D11SharedDeviceGuard sharedGuard;
        IFC(TakeLockAndCheckDeviceLost(&sharedGuard));
        auto lockedContext = GetLockedDeviceContext(&sharedGuard);

        pDXGIDevice3->Trim();
    }

    // Finally schedule (loose term) a callback to clean up our D2D Resources.  If we are not a shared
    // device we can do this immediately, however, if we are shared, then we don't want to give up
    // resources that are still are usable.  So we will launch a background thread that will wait a bit
    // and then free up anything that hasn't been used recently.
    if (m_pD2DDevice != NULL)
    {
        if (!m_isSharedDevice)
        {
            m_pD2DDevice->ClearResources(0);
        }
        else
        {
            IPALWaitable* pThread = nullptr;
            // Worst possible case if we fail is not cleaning memory up in a timely manner
            IGNOREHR(gps->ThreadCreate(&pThread, &CD3D11DeviceInstance::ReleaseD2DResources,
                1 /* cData */, reinterpret_cast<UINT8 *>(this)));
            if (pThread != nullptr)
            {
                // Close will CloseHandle on the thread handle and call delete this
                pThread->Close();
            }
        }
    }

Cleanup:
    ReleaseInterface(pDXGIDevice3);
    return hr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on a background thread to release unused D2D Resources.
//
//------------------------------------------------------------------------------
XINT32
CD3D11DeviceInstance::ReleaseD2DResources(_In_ UINT8 * pData)
{
    // Sleep for a second.  If we are running less than one frame per second, we have
    // bigger issues.
    const long millisecondDelay = 1000;
    ::Sleep(millisecondDelay);

    std::shared_ptr<CD3D11DeviceInstance> activedevice = GetSharedDeviceInstance();

    if ((UINT8*)activedevice.get() != pData)
    {
        // Our shared device has changed, so this one is going away any way
        return 0;
    }

    activedevice->ReleaseD2DResources(millisecondDelay);

    return 0;
}

void CD3D11DeviceInstance::ReleaseD2DResources(const long millisecondDelay)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();
    if (m_pD2DDevice != nullptr)
    {
        m_pD2DDevice->ClearResources(millisecondDelay);
    }
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread after the render thread has finished reading from a
//      SystemMemoryBits object. This returns a SystemMemoryBits object to the pool.
//
//------------------------------------------------------------------------------
void
CD3D11DeviceInstance::ReturnToSysMemBitsPool(
    _In_ SystemMemoryBits *pSysMemBitsIn
    )
{
    ASSERT(pSysMemBitsIn);

    wil::cs_leave_scope_exit guard = m_lock.lock();

    // Add the surface to the pool, at the end of the list.
    ASSERT(!pSysMemBitsIn->GetPoolLink()->IsOnList());
    m_sysMemBitsPool.PushBack(pSysMemBitsIn->GetPoolLink());

    // The pool now has a reference to the surface
    pSysMemBitsIn->AddRef();

    // Record the time that the surface was added to the pool
    pSysMemBitsIn->SetTimeStamp(gps->GetCPUMilliseconds());
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread to allocate a SystemMemoryBits from the pool.
//      This will create a new SystemMemoryBits object if no suitable one is found.
//
//      8bpp textures are allocated in the heap. They will be copied out and
//      expanded to 32bpp before they're uploaded to DComp. 32bpp textures are
//      created in driver-visible memory.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11DeviceInstance::AllocateFromSysMemBitsPool(
    UINT32 width,
    UINT32 height,
    DXGI_FORMAT dxgiFormat,
    bool hasNative8BitSupport,
    _Outptr_ SystemMemoryBits **ppOut
    )
{
    HRESULT hr = S_OK;
    SystemMemoryBits *pResult = NULL;

    *ppOut = NULL;

    // This surface must be allocated on the heap if there's no native support for the requested pixel format.
    BOOL fUseHeapMemory = dxgiFormat == DXGI_FORMAT_A8_UNORM && !hasNative8BitSupport;

    wil::cs_leave_scope_exit guard = m_lock.lock();

    // Search the pool for a matching surface.
    for (IntrusiveList<SystemMemoryBits>::Iterator it = m_sysMemBitsPool.Begin(SystemMemoryBits::PoolLinkOffset());
        it != m_sysMemBitsPool.End();
        ++it)
    {
        // If a matching surface has been found, then the loop should not still be running.
        ASSERT(!pResult);

        SystemMemoryBits *pPooledSurface = *it;

        // Allow any surface large enough to service this request to be re-used. This limits peak working set by
        // minimizing the need to allocate new textures to service requests, and makes searching the pool cheap.
        if ((pPooledSurface->GetWidth() >= width) &&
            (pPooledSurface->GetHeight() >= height) &&
            (pPooledSurface->GetDxgiFormat() == dxgiFormat) &&
            (!pPooledSurface->IsDriverVisible() != !fUseHeapMemory))
        {
            // Once we found a matching surface, return it.
            //
            // If the match is driver-visible, it needs to be mapped. If the map fails, it's because this surface is still in use
            // by the GPU. Since we iterate through the pool in LRU order, if this surface is still in use, any other matches
            // will be in use as well. Break out of the loop here and create a new surface.
            if (   !pPooledSurface->IsDriverVisible()
                // Don't allow Map calls to block on GPU work here. There might be another available one in the pool.
                || SUCCEEDED(static_cast<SystemMemoryBitsDriver*>(pPooledSurface)->EnsureMapped(FALSE /* allowWait */)))
            {
                m_sysMemBitsPool.Remove(pPooledSurface->GetPoolLink());

                // Reference transferred from pool to to pResult
                pResult = pPooledSurface;
            }

            break;
        }
    }

    // If no surface was found in the pool then create a new one.
    if (!pResult)
    {

        if (fUseHeapMemory)
        {
            IFC(SystemMemoryBitsHeap::Create(
                width,
                height,
                dxgiFormat,
                &pResult
                ));
        }
        else
        {
            if (IsDeviceLost())
            {
                IFC(DXGI_ERROR_DEVICE_REMOVED);
            }

            ASSERT(dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM ||
                dxgiFormat == DXGI_FORMAT_A8_UNORM ||
                dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT);

            auto deviceContext = GetLockableDeviceContext();

            IFC(SystemMemoryBitsDriver::Create(
                m_pDevice,
                deviceContext,
                width,
                height,
                dxgiFormat,
                &pResult
                ));

            // At this point, there are no available textures in the pool that we can reuse. Usually the Map call
            // won't block because the texture was newly created, but some drivers may still block here. If it
            // blocks, we have to wait.
            // Note that this call is expected to complete without blocking. We're working around a possible driver
            // bug here.
            IFC(static_cast<SystemMemoryBitsDriver *>(pResult)->EnsureMapped(TRUE /* allowWait */));
        }
    }

    // Transfer reference to *ppOut
    ASSERT(pResult);
    *ppOut = pResult;
    pResult = NULL;

Cleanup:
    ReleaseInterface(pResult);

    return hr;
}
