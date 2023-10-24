// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// A clock that is shared between the compositor thread and the UI thread.
// It can drift forward because of queuing frames ahead on the compositor thread for
// animation, but reports consistent time with the UI thread since it is shared.
class RefreshAlignedClock final : public CXcpObjectBase<IPALTickableClock>
{
public:
    static _Check_return_ HRESULT Create(_Outptr_ RefreshAlignedClock **ppTimeManagerClock);

    XDOUBLE Tick() override;
    XDOUBLE GetLastTickTimeInSeconds() const override;
    XDOUBLE GetNextTickTimeInSeconds() const override;

    void SetRefreshRateInfo(_In_opt_ IPALRefreshRateInfo *pRefreshRateInfo);

private:
    RefreshAlignedClock();
    ~RefreshAlignedClock() override;

    _Check_return_ HRESULT Initialize();

private:
    IPALClock *m_pIClock;
    mutable wil::critical_section m_Lock;
    XDOUBLE m_lastReportedTime;
    IPALRefreshRateInfo *m_pRefreshRateInfo;
};
