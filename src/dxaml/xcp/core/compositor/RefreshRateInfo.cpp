// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RefreshRateInfo.h"
#include "DCompTreeHost.h"
#include "XamlTelemetry.h"

/* static */ _Check_return_ HRESULT RefreshRateInfo::Create(_Outptr_ RefreshRateInfo** ppRefreshRateInfo)
{
    wrl::ComPtr<IPALClock> clock;
    IFC_RETURN(gps->CreateClock(clock.ReleaseAndGetAddressOf()));

    wrl::ComPtr<RefreshRateInfo> refreshRateInfo;
    refreshRateInfo.Attach(new RefreshRateInfo(clock.Get()));
    refreshRateInfo->RegisterForPowerNotification();
    *ppRefreshRateInfo = refreshRateInfo.Detach();

    return S_OK;
}

RefreshRateInfo::RefreshRateInfo(_In_ IPALClock* clock)
    : m_refreshIntervalInMilliseconds(DefaultRefreshIntervalInMilliseconds)
    , m_clock(clock)
{
}

RefreshRateInfo::~RefreshRateInfo()
{
}

float RefreshRateInfo::GetRefreshIntervalInMilliseconds()
{
    // Set to 0 if it's already 0 -- we're interested in only the return value.
    LONG refreshRateBitPattern = InterlockedCompareExchange(
        reinterpret_cast<LONG*>(&m_refreshIntervalInMilliseconds),
        0,
        0);

    return *reinterpret_cast<float*>(&refreshRateBitPattern);
}

// Called by the UI thread to set the refresh interval.
void RefreshRateInfo::SetRefreshIntervalInMilliseconds(float refreshIntervalInMilliseconds)
{
    if (m_refreshIntervalInMilliseconds != refreshIntervalInMilliseconds)
    {
        TraceLoggingProviderWrite(
            XamlTelemetry, "RefreshRateInfo_SetRefreshIntervalInMilliseconds",
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
            TraceLoggingFloat32(m_refreshIntervalInMilliseconds, "PreviousRefreshIntervalInMilliseconds"),
            TraceLoggingFloat32(refreshIntervalInMilliseconds, "NewRefreshIntervalInMilliseconds"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        LONG refreshRateBitPattern = *reinterpret_cast<LONG*>(&refreshIntervalInMilliseconds);

        InterlockedExchange(
            reinterpret_cast<LONG*>(&m_refreshIntervalInMilliseconds),
            refreshRateBitPattern);
    }
}

// Code copied from CIdealRenderTarget::WaitForVBlank
// Try to use WaitForCompositorClock to wait for VBlank, like the lifted compositor does.

// The maximum nubmer of objects we allow users to wait on the compositor clock
#define DCOMPOSITION_MAX_WAITFORCOMPOSITORCLOCK_OBJECTS 32

// default 80ms timeout is used internally by DxgkWaitForVerticalBlankEvent2
static constexpr DWORD c_vBlankWaitTimeoutMs = 80;

STDAPI_(DWORD) DCompositionWaitForCompositorClock(
    _In_range_(0, DCOMPOSITION_MAX_WAITFORCOMPOSITORCLOCK_OBJECTS) UINT count,
    _In_reads_opt_(count) const HANDLE* handles,
    _In_ DWORD timeoutInMs);

#define STATUS_GRAPHICS_PRESENT_OCCLUDED ((NTSTATUS)0xC01E0006L)

auto RefreshRateInfo::GetWaitForCompositorClockFn()
{
    // Dynamically load this function from dcomp - we can't link directly against it cause it might not exist downlevel.
    if (!m_dcompModule)
    {
        m_dcompModule = LoadLibraryExW(L"dcomp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        ASSERT(m_dcompModule);
    }

    using proc_type = decltype(DCompositionWaitForCompositorClock);
    auto proc = reinterpret_cast<proc_type*>(GetProcAddress(m_dcompModule, "DCompositionWaitForCompositorClock"));
    return proc;
}

_Check_return_ HRESULT RefreshRateInfo::TryWaitForCompositorClock(_Out_ bool* pWaitForCompositorClockSucceeded)
{
    HRESULT hr = S_OK;
    *pWaitForCompositorClockSucceeded = false;

    static auto pfn_WaitForCompositorClock = GetWaitForCompositorClockFn();
    if (pfn_WaitForCompositorClock != nullptr)
    {
        // BUG 40562975: Wow64 thunk generation is broken for the WaitForCompositorClock function
        // and causes access violations if we pass null for the list of handles (even though count
        // is 0)
        // This only affects Wow scenarios, and only while under a debugger (shows up as a
        // first-chance exception that is continuable).
        HANDLE dummyHandles[1] = {};

        // We want to wait for the compositor clock instead of WaitForVBlank because it also
        // responds to framerate boosting. But this API only exists on SV1+.
        DWORD dwWaitResult = pfn_WaitForCompositorClock(
            0, // count
            dummyHandles, // handles
            c_vBlankWaitTimeoutMs /*timeout*/);

        if (STATUS_GRAPHICS_PRESENT_OCCLUDED == (NTSTATUS) dwWaitResult)
        {
            // This covers the "display powered off" case where the wait returns immediately. Treat it as if we couldn't
            // use WaitForCompositorClock. Xaml will fall back to its old simulated vblank waiting code path.
            hr = S_OK;
            *pWaitForCompositorClockSucceeded = false;
        }
        else
        {
            // We can get 0xc0000001 (operation failed) if we were connected through Remote Desktop and it
            // disconnects (and we haven't received the "display off" notification yet). Consider failed results as
            // failed to wait, and fall back to simulated vblanks. Force a succeeded return value afterwards so we
            // don't store a bad HR on the render thread and crash the UI thread when it finds an unrecognized
            // failure.
            *pWaitForCompositorClockSucceeded = SUCCEEDED((HRESULT) dwWaitResult);
            hr = S_OK;
        }
    }

    return hr;
}

ULONG CALLBACK
PowerNotification(_In_ PVOID pvContext, ULONG /*type*/, _In_ PVOID pvSetting)
{
    auto* pSetting = reinterpret_cast<POWERBROADCAST_SETTING*>(pvSetting);
    if (IsEqualGUID(pSetting->PowerSetting, GUID_SESSION_DISPLAY_STATUS))
    {
        ASSERT(pSetting->DataLength == sizeof(DWORD));
        DWORD displayState = *reinterpret_cast<DWORD*>(pSetting->Data);

        auto* refreshRateInfo = reinterpret_cast<RefreshRateInfo*>(pvContext);
        switch (displayState)
        {
            case 0: // Off
                refreshRateInfo->SetIsDisplayOn(false);
                break;
            case 1: // On
                refreshRateInfo->SetIsDisplayOn(true);
                break;
            case 2: // Dimmed
                break;
            default:
                ASSERT(false);
                break;
        }
    }

    return ERROR_SUCCESS;
}

// End copied code

void RefreshRateInfo::RegisterForPowerNotification()
{
    DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS notification = {};
    notification.Callback = PowerNotification;
    notification.Context = this;

    // Registration will immediately call back with the current state.
    IFCFAILFAST(HRESULT_FROM_WIN32(
        PowerSettingRegisterNotification(
            &GUID_SESSION_DISPLAY_STATUS,
            DEVICE_NOTIFY_CALLBACK,
            &notification,
            wil::out_param(m_hOcclusion))));
}

void RefreshRateInfo::SetIsDisplayOn(bool isDisplayOn)
{
    m_isDisplayOn = isDisplayOn;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Waits for the vBlank (hardware interrupt).
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
RefreshRateInfo::WaitForRefreshInterval()
{
    bool waitForCompositorClockSucceeded = false;

    //
    // The lifted compositor uses DCompositionWaitForCompositorClock to handle waiting, which deals with high refresh
    // rate monitors. If DCompositionWaitForCompositorClock doesn't exist, we fall back to WaitForVBlank provided that
    // the display isn't turned off.
    //
    // Note for Task 43816828: Consume lifted Compositor's WaitForCompositorClock function for throttling lifted Xaml.
    // Be careful around what the return value of the lifted function is. A failure to wait could return a failed HR, so
    // we might fall back to simulated VBlanks rather than bubble up the failure.
    //
    if (m_isDisplayOn)
    {
        TraceLoggingProviderWrite(
            XamlTelemetry, "RefreshRateInfo_WaitForCompositorClock",
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
            TraceLoggingBoolean(true, "IsStart"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        IFC_RETURN(TryWaitForCompositorClock(&waitForCompositorClockSucceeded));

        TraceLoggingProviderWrite(
            XamlTelemetry, "RefreshRateInfo_WaitForCompositorClock",
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
            TraceLoggingBoolean(false, "IsStart"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));
    }

    if (!waitForCompositorClockSucceeded)
    {
        float refreshIntervalInMilliseconds = GetRefreshIntervalInMilliseconds();
        // If the wait failed then the display is turned off. Manually throttle via a Sleep.
        // Note: we're slightly slower than the target frame rate, because we're waiting for the full frame time rather
        // than the amount of time remaining in the frame. We don't care because the screen is turned off - we can run a
        // bit under the target frame rate and that's fine.
        TraceRenderThreadSimulateVBlankBegin();

        TraceLoggingProviderWrite(
            XamlTelemetry, "RefreshRateInfo_SimulateWaitForVBlank",
            TraceLoggingUInt64(reinterpret_cast<uint64_t>(this), "ObjectPointer"),
            TraceLoggingFloat32(refreshIntervalInMilliseconds, "RefreshIntervalInMilliseconds"),
            TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE));

        Sleep(static_cast<uint32_t>(refreshIntervalInMilliseconds));
        TraceRenderThreadSimulateVBlankEnd();
    }

    return S_OK;
}
