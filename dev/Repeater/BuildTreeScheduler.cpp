// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "QPCTimer.h"
#include "BuildTreeScheduler.h"
#include "RepeaterTestHooks.h"

double BuildTreeScheduler::m_budgetInMs = 40.0;
thread_local QPCTimer BuildTreeScheduler::m_timer{};
thread_local std::vector<WorkInfo> BuildTreeScheduler::m_pendingWork{};
thread_local winrt::event_token BuildTreeScheduler::m_renderingToken{};

void BuildTreeScheduler::RegisterWork(int priority, const std::function<void()>& workFunc)
{
    MUX_ASSERT(priority >= 0);
    MUX_ASSERT(workFunc != nullptr);

    QueueTick();
    m_pendingWork.push_back(WorkInfo(priority, workFunc));
}

bool BuildTreeScheduler::ShouldYield()
{
    return m_timer.DurationInMilliSeconds() > m_budgetInMs;
}

void BuildTreeScheduler::OnRendering(const winrt::IInspectable&, const winrt::IInspectable&)
{
    const bool budgetReached = ShouldYield();
    if (!budgetReached && m_pendingWork.size() > 0)
    {
        // Sort in descending order of priority and work from the end of the list to avoid moving around during erase.
        std::sort(m_pendingWork.begin(), m_pendingWork.end(), [](const auto& lhs, const auto& rhs) { return lhs.Priority() > rhs.Priority(); });
        int currentIndex = static_cast<int>(m_pendingWork.size()) - 1;

        do
        {
            m_pendingWork[currentIndex].InvokeWorkFunc();
            m_pendingWork.erase(m_pendingWork.begin() + currentIndex);
        } while (--currentIndex >= 0 && !ShouldYield());
    }

    if (m_pendingWork.empty())
    {
        // No more pending work, unhook from rendering event since being hooked up will case wux to try to 
        // call the event at 60 frames per second
        winrt::Windows::UI::Xaml::Media::CompositionTarget::CompositionTarget::Rendering(m_renderingToken);
        m_renderingToken.value = 0;
        RepeaterTestHooks::NotifyBuildTreeCompleted();
    }

    // Reset the timer so it snaps the time just before rendering
    m_timer.Reset();
}

void  BuildTreeScheduler::QueueTick()
{
    if (m_renderingToken.value == 0)
    {
        m_renderingToken = winrt::Windows::UI::Xaml::Media::CompositionTarget::Rendering(OnRendering);
    }
}
