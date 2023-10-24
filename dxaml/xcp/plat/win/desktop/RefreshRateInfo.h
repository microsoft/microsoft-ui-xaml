// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <powrprof.h>

//  Synopsis:
//      A class that returns the refresh rate and waits for a vblank.

using unique_powerhpowernotify = wil::unique_any<
    HPOWERNOTIFY,
    decltype(&::PowerSettingUnregisterNotification),
    ::PowerSettingUnregisterNotification>;

class RefreshRateInfo
    : public CXcpObjectBase<IPALRefreshRateInfo, CXcpObjectAddRefPolicy>
{
public:
    static _Check_return_ HRESULT Create(
        _In_opt_ IDXGIOutput *pOutput,
        _In_ IDXGIFactory1 *pFactory,
        _Outptr_ RefreshRateInfo **ppRefreshRateInfo
        );

    virtual XFLOAT GetRefreshIntervalInMilliseconds();

    virtual _Check_return_ HRESULT WaitForRefreshInterval();

    virtual bool IsValid();

    void SetIsDisplayOn(bool isDisplayOn);

private:
    RefreshRateInfo(
        _In_opt_ IDXGIOutput *pOutput,
        _In_ IDXGIFactory1 *pFactory,
        _In_ IPALClock *pIClock
        );
    ~RefreshRateInfo();

    auto GetWaitForCompositorClockFn();
    _Check_return_ HRESULT TryWaitForCompositorClock(_Out_ bool* waitForCompositorClockFound);

    void RegisterForPowerNotification();

private:
    IDXGIOutput *m_pIOutput;
    IDXGIFactory1 *m_pIFactory;

    XFLOAT m_refreshIntervalInMilliseconds;

    IPALClock *m_pIClock;

    bool m_isDisplayOn {true};
    unique_powerhpowernotify m_hOcclusion;

    HMODULE m_dcompModule = 0;
};
