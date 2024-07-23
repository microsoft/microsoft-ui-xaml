// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class RefreshRateInfo;
class RefreshAlignedClock;

//
// Contains algorithms for the scheduling thread. Covered by unit tests.
// Code from CompositorScheduler should move into this class.
//
class Scheduler : public CXcpObjectBase<>
{
public:
    static bool ShouldWaitForVBlank(float currentTimeInMilliseconds, float previousFrameTimeInMilliseconds, float refreshIntervalInMilliseconds);

    Scheduler(_In_ RefreshRateInfo* refreshRateInfo, _In_ RefreshAlignedClock* clock);
    ~Scheduler();

    _Check_return_ HRESULT OnImmediateUIThreadFrame();

private:
    // Used to avoid WaitForVBlank when the previous frame was more than a vblank ago.
    float m_previousFrameTimeInMilliseconds {0.0f};

    wrl::ComPtr<RefreshRateInfo> m_refreshRateInfo;
    wrl::ComPtr<RefreshAlignedClock> m_clock;
};
