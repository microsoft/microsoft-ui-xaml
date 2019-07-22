// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct WorkInfo
{
    WorkInfo(int priority, const std::function<void()>& workFunc) :
        m_priority(priority),
        m_workFunc(workFunc)
    {}

    [[nodiscard]] int Priority() const { return m_priority; }
    void InvokeWorkFunc() const { m_workFunc(); }

private:
    int m_priority;
    std::function<void()> m_workFunc;
};

// High performance time management using QueryPerformanceCounter
class BuildTreeScheduler final
{
public:
    static void RegisterWork(int priority, const std::function<void()>& workFunc);
    static bool ShouldYield();

private:
    static void OnRendering(const winrt::IInspectable& sender, const winrt::IInspectable& args);
    static void QueueTick();

    static double m_budgetInMs;

    static thread_local QPCTimer m_timer;
    static thread_local std::vector<WorkInfo> m_pendingWork;
    static thread_local winrt::event_token m_renderingToken;
};