// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <wil/resource.h>
#include <palnetwork.h>
#include <deque>

class CCoreServices;

struct IImageTask;

class ImageTaskDispatcher final : public CXcpObjectBase< IPALExecuteOnUIThread >
{
public:
    ImageTaskDispatcher(_In_opt_ CCoreServices* core);

    _Check_return_ HRESULT QueueTask(_In_ IImageTask* task);

    void Shutdown();

    // IPALExecuteOnUIThread
    _Check_return_ HRESULT Execute() override;

    void ThrottleExecution_TestHook(bool enableThrottling, unsigned int numberOfTasksAllowedToDispatch);
    void RequestExecution_TestHook();

private:
    ~ImageTaskDispatcher() override;

    CCoreServices* m_core;
    std::deque<xref_ptr<IImageTask>> m_tasks;
    wil::critical_section m_taskLock;
    LONG m_hasCallbackQueued = FALSE;
    bool m_shuttingdown = false;

    // This test hook controls how many tasks are dispatched by the ImageTaskDispatcher. When the flag is true, we will
    // count down from m_tasksAllowedToDispatch_TestHook as we dispatch image tasks. When we reach 0, no more tasks are
    // allowed to execute until a test hook bumps up the allowable count or a test hook disables throttled execution.
    bool m_throttledExecution_TestHook = false;
    size_t m_numberOfTasksAllowedToDispatch_TestHook = 0;
};
