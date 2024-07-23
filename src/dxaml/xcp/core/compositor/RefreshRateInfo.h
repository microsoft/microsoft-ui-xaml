// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <powrprof.h>

//
// A class that returns the refresh rate and waits for a vblank.
//
// The refresh rate comes from DComp's GetFrameStatistics. The scheduling thread doesn't have the DComp device; the UI
// thread does, so the UI thread queries it and sets it on this object. The schduling thread reads it off this object.
//
using unique_powerhpowernotify = wil::unique_any<
    HPOWERNOTIFY,
    decltype(&::PowerSettingUnregisterNotification),
    ::PowerSettingUnregisterNotification>;

class RefreshRateInfo
    : public CXcpObjectBase<CXcpObjectAddRefPolicy>
{
    friend class DCompTreeHost;

public:
    static _Check_return_ HRESULT Create(_Outptr_ RefreshRateInfo** ppRefreshRateInfo);

    float GetRefreshIntervalInMilliseconds();

    _Check_return_ HRESULT WaitForRefreshInterval();

    void SetIsDisplayOn(bool isDisplayOn);

private:
    RefreshRateInfo(_In_ IPALClock* clock);
    ~RefreshRateInfo();

    // DCompTreeHost
    void SetRefreshIntervalInMilliseconds(float refreshIntervalInMilliseconds);

    // WaitForCompositorClock
    auto GetWaitForCompositorClockFn();
    _Check_return_ HRESULT TryWaitForCompositorClock(_Out_ bool* waitForCompositorClockFound);
    void RegisterForPowerNotification();

private:

    float m_refreshIntervalInMilliseconds;
    wrl::ComPtr<IPALClock> m_clock;

    // WaitForCompositorClock
    bool m_isDisplayOn {true};
    unique_powerhpowernotify m_hOcclusion;
    HMODULE m_dcompModule = 0;
};
