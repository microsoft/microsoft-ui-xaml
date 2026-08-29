// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//
// A class that returns the refresh rate and waits for a vblank.
//
// The refresh rate comes from DComp's GetFrameStatistics. The scheduling thread doesn't have the DComp device; the UI
// thread does, so the UI thread queries it and sets it on this object. The schduling thread reads it off this object.
//

class RefreshRateInfo
    : public CXcpObjectBase<CXcpObjectAddRefPolicy>
{
    friend class DCompTreeHost;

public:
    static _Check_return_ HRESULT Create(_Outptr_ RefreshRateInfo** ppRefreshRateInfo);

    float GetRefreshIntervalInMilliseconds();

    _Check_return_ HRESULT WaitForRefreshInterval();

private:
    RefreshRateInfo(_In_ IPALClock* clock);
    ~RefreshRateInfo();

    // DCompTreeHost
    void SetRefreshIntervalInMilliseconds(float refreshIntervalInMilliseconds);

    // WaitForCompositorClock
    auto GetWaitForCompositorClockFn();
    _Check_return_ HRESULT TryWaitForCompositorClock(_Out_ bool* waitForCompositorClockFound);

private:

    float m_refreshIntervalInMilliseconds;
    wrl::ComPtr<IPALClock> m_clock;

    // WaitForCompositorClock
    HMODULE m_dcompModule = 0;
};
