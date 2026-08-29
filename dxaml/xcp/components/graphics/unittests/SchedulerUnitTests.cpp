// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlLogging.h"
#include "SchedulerUnitTests.h"
#include "Scheduler.h"

//
// Mock classes used by the Scheduler
//
// Scheduler has the scheduling algorithm, which depends on a RefreshRateInfo that provides the refresh interval and a way
// to wait on VBlanks plus a RefreshAlignedClock that provides the current time. Mock those dependencies and inject known numbers.
//

class RefreshRateInfo : public CXcpObjectBase<CXcpObjectAddRefPolicy>
{
public:
    // WaitForRefreshInterval - track whether it was called and verify.
    bool m_waitForRefreshIntervalCalled {false};
    _Check_return_ HRESULT WaitForRefreshInterval();
    bool GetWaitForRefreshIntervalCalled()
    {
        bool wasCalled = m_waitForRefreshIntervalCalled;
        m_waitForRefreshIntervalCalled = false;
        return wasCalled;
    }

    // Refresh interval - return a provided constant
    float m_refreshIntervalInMilliseconds {0.0f};
    float GetRefreshIntervalInMilliseconds();
    void SetRefreshIntervalInMilliseconds(float refreshRate) { m_refreshIntervalInMilliseconds = refreshRate; }


    void CheckCleanState()
    {
        VERIFY_IS_TRUE(!m_waitForRefreshIntervalCalled);
    }
};

_Check_return_ HRESULT RefreshRateInfo::WaitForRefreshInterval()
{
    VERIFY_IS_FALSE(m_waitForRefreshIntervalCalled);
    m_waitForRefreshIntervalCalled = true;
    return S_OK;
}

float RefreshRateInfo::GetRefreshIntervalInMilliseconds()
{
    return m_refreshIntervalInMilliseconds;
}

class RefreshAlignedClock : public CXcpObjectBase<>
{
public:
    // GetAbsoluteTimeInMilliseconds - queue up a list of times and return one each time the scheduling algorithm asks.
    // Verify that it has asked as many times as we expected.
    std::queue<float> m_times;
    float m_currentTime {0.0f};
    float GetAbsoluteTimeInMilliseconds();
    void QueueGetTimeAfter(float timeDiffInMilliseconds)
    {
        m_currentTime += timeDiffInMilliseconds;
        m_times.push(m_currentTime);
    }
    bool HasQueuedTimes() { return !m_times.empty(); }


    void CheckCleanState()
    {
        VERIFY_IS_TRUE(!HasQueuedTimes());
    }
};

float RefreshAlignedClock::GetAbsoluteTimeInMilliseconds()
{
    VERIFY_IS_FALSE(m_times.empty());
    float time = m_times.front();
    m_times.pop();
    return time;
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

void SchedulerUnitTests::ShouldWaitForVBlank()
{
    // 60Hz tests
    VERIFY_IS_FALSE(Scheduler::ShouldWaitForVBlank(1000.0f, 0.0f, 16.67f));     // The previous tick was much more than 1 frame ago
    VERIFY_IS_FALSE(Scheduler::ShouldWaitForVBlank(26.68f, 10.0f, 16.67f));     // The previous tick was just more than 1 frame ago
    VERIFY_IS_TRUE(Scheduler::ShouldWaitForVBlank(46.66f, 30.0f, 16.67f));      // The previous tick was just less than 1 frame ago
    VERIFY_IS_TRUE(Scheduler::ShouldWaitForVBlank(52.0f, 40.0f, 16.67f));       // The previous tick was much less than 1 frame ago

    // 120Hz tests
    VERIFY_IS_FALSE(Scheduler::ShouldWaitForVBlank(56.66f, 40.0f, 8.33f));      // The previous tick was much more than 1 frame ago
    VERIFY_IS_FALSE(Scheduler::ShouldWaitForVBlank(38.34f, 30.0f, 8.33f));      // The previous tick was just more than 1 frame ago
    VERIFY_IS_TRUE(Scheduler::ShouldWaitForVBlank(18.32f, 10.0f, 8.33f));       // The previous tick was just less than 1 frame ago
    VERIFY_IS_TRUE(Scheduler::ShouldWaitForVBlank(6.0f, 0.0f, 8.33f));          // The previous tick was much less than 1 frame ago
}

void SchedulerUnitTests::OnImmediateUIThreadFrame_60Hz()
{
    wrl::ComPtr<RefreshRateInfo> mockRefreshRateInfo;
    mockRefreshRateInfo.Attach(new RefreshRateInfo());

    wrl::ComPtr<RefreshAlignedClock> mockClock;
    mockClock.Attach(new RefreshAlignedClock());

    Scheduler scheduler(mockRefreshRateInfo.Get(), mockClock.Get());

    mockRefreshRateInfo->SetRefreshIntervalInMilliseconds(16.67f);

    LOG_OUTPUT(L"> 1st tick at 17ms.");
    mockClock->QueueGetTimeAfter(17.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 2nd tick 13ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(13.0f);
    mockClock->QueueGetTimeAfter(3.67f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 3rd tick 10ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(10.0f);
    mockClock->QueueGetTimeAfter(6.67f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 4th tick 250ms later due to timer - no need to throttle.");
    mockClock->QueueGetTimeAfter(250.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 5th tick 250ms later due to timer - no need to throttle.");
    mockClock->QueueGetTimeAfter(250.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 6th tick 12ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(12.0f);
    mockClock->QueueGetTimeAfter(4.67f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 7th tick 17ms later - (barely) no need to throttle.");
    mockClock->QueueGetTimeAfter(17.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    mockRefreshRateInfo->CheckCleanState();
    mockClock->CheckCleanState();
}

void SchedulerUnitTests::OnImmediateUIThreadFrame_120Hz()
{
    wrl::ComPtr<RefreshRateInfo> mockRefreshRateInfo;
    mockRefreshRateInfo.Attach(new RefreshRateInfo());

    wrl::ComPtr<RefreshAlignedClock> mockClock;
    mockClock.Attach(new RefreshAlignedClock());

    Scheduler scheduler(mockRefreshRateInfo.Get(), mockClock.Get());

    mockRefreshRateInfo->SetRefreshIntervalInMilliseconds(8.33f);

    LOG_OUTPUT(L"> 1st tick at 0ms - throttle (implicit first tick at 0ms so this tick is too early).");
    mockClock->QueueGetTimeAfter(0.0f);
    mockClock->QueueGetTimeAfter(8.33f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 2nd tick 3ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(3.0f);
    mockClock->QueueGetTimeAfter(5.33f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 3rd tick 500ms later - no need to throttle.");
    mockClock->QueueGetTimeAfter(500.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 4th tick 8ms later - (barely) need to throttle.");
    mockClock->QueueGetTimeAfter(8.0f);
    mockClock->QueueGetTimeAfter(0.33f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 5th tick 500ms later - no need to throttle.");
    mockClock->QueueGetTimeAfter(500.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 6th tick 9ms later - (barely) no need to throttle.");
    mockClock->QueueGetTimeAfter(9.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 7th tick 5ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(5.0f);
    mockClock->QueueGetTimeAfter(3.33f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    mockRefreshRateInfo->CheckCleanState();
    mockClock->CheckCleanState();
}

void SchedulerUnitTests::OnImmediateUIThreadFrame_60Hzto120Hz()
{
    wrl::ComPtr<RefreshRateInfo> mockRefreshRateInfo;
    mockRefreshRateInfo.Attach(new RefreshRateInfo());

    wrl::ComPtr<RefreshAlignedClock> mockClock;
    mockClock.Attach(new RefreshAlignedClock());

    Scheduler scheduler(mockRefreshRateInfo.Get(), mockClock.Get());

    mockRefreshRateInfo->SetRefreshIntervalInMilliseconds(16.67f);

    LOG_OUTPUT(L"> 1st tick at 50ms - no need to throttle.");
    mockClock->QueueGetTimeAfter(50.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 2nd tick 15ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(15.0f);
    mockClock->QueueGetTimeAfter(1.67f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 3rd tick 2ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(2.0f);
    mockClock->QueueGetTimeAfter(14.67f);   // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> Refresh rate changes to 120Hz.");
    mockRefreshRateInfo->SetRefreshIntervalInMilliseconds(8.33f);

    LOG_OUTPUT(L"> 4th tick 15ms later - no need to throttle under the new refresh rate.");
    mockClock->QueueGetTimeAfter(15.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 5th tick 2ms later - too early and needs throttling.");
    mockClock->QueueGetTimeAfter(2.0f);
    mockClock->QueueGetTimeAfter(6.33f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> 6th tick 10ms later - no need to throttle under the new refresh rate.");
    mockClock->QueueGetTimeAfter(10.0f);
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_FALSE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    LOG_OUTPUT(L"> Refresh rate changes back to 60Hz.");
    mockRefreshRateInfo->SetRefreshIntervalInMilliseconds(16.67f);

    LOG_OUTPUT(L"> 7th tick 10ms later - need throttling under the new refresh rate.");
    mockClock->QueueGetTimeAfter(10.0f);
    mockClock->QueueGetTimeAfter(6.67f);    // Suppose WaitForVBlank waits for the rest of the frame
    VERIFY_SUCCEEDED(scheduler.OnImmediateUIThreadFrame());
    VERIFY_IS_TRUE(mockRefreshRateInfo->GetWaitForRefreshIntervalCalled());
    VERIFY_IS_FALSE(mockClock->HasQueuedTimes());

    mockRefreshRateInfo->CheckCleanState();
    mockClock->CheckCleanState();
}

} } } } }