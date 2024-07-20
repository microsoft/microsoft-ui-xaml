// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImageTaskDispatcher.h>
#include <ImageProviderInterfaces.h>
#include <corep.h>
#include "GraphicsTelemetry.h"

ImageTaskDispatcher::ImageTaskDispatcher(_In_opt_ CCoreServices *core)
    : m_core(core)
{
    XCP_WEAK(&m_core);
}

ImageTaskDispatcher::~ImageTaskDispatcher()
{
}

_Check_return_ HRESULT
ImageTaskDispatcher::QueueTask(_In_ IImageTask* task)
{
    // Do not queue more tasks if the ImageTaskDispatcher is shutdown
    if (!m_shuttingdown)
    {
        {
            auto guard = m_taskLock.lock();

            // Tasks with non-zero request id contain responses from the long running asynchronous
            // requests and may contain large payload and/or be expensive to process.
            // We are only interested in the most recent request state so we discard the
            // previous task under the same request id.

            bool existingTaskReplaced = false;
            auto requestId = task->GetRequestId();
            if (requestId != 0)
            {
                //
                // Check if there's something else in the queue with the same request ID and replace it if found.
                // Note that this cannot replace the currently running task. ImageTaskDispatcher::Execute will pop
                // a task off the front of the queue before executing it, so it won't be found in the queue again.
                // We can only replace a task that's still in the queue but hasn't executed yet.
                //
                for (auto &existingTask: m_tasks)
                {
                    if (existingTask->GetRequestId() == requestId)
                    {
                        existingTask.reset(task);
                        existingTaskReplaced = true;
                        break;
                    }
                }
            }

            if (!existingTaskReplaced)
            {
                m_tasks.emplace_back(task);
            }
        }

        if (InterlockedCompareExchange(&m_hasCallbackQueued, TRUE, FALSE) == FALSE)
        {
            GraphicsTelemetry::ImageTaskDispatcher_QueueTask(true /* WorkQueued */, m_tasks.size());
            // unit tests call Execute manually
            if (m_core != nullptr)
            {
                IFC_RETURN(m_core->ExecuteOnUIThread(this, ReentrancyBehavior::BlockReentrancy));
            }
        }
        else
        {
            GraphicsTelemetry::ImageTaskDispatcher_QueueTask(false /* WorkQueued */, m_tasks.size());
        }
    }

    return S_OK;
}

void ImageTaskDispatcher::ThrottleExecution_TestHook(bool enableThrottling, unsigned int numberOfTasksAllowedToDispatch)
{
    auto guard = m_taskLock.lock();

    m_throttledExecution_TestHook = enableThrottling;
    m_numberOfTasksAllowedToDispatch_TestHook = static_cast<size_t>(numberOfTasksAllowedToDispatch);

    // Note: We should be checking whether there's work that was put off and requesting another pass (via
    // m_core->ExecuteOnUIThread) if there is any. We don't because this is a test hook that's currently used to repro a
    // specific timing where we want CCoreServices::ResetCoreWindowVisualTree to be the place that calls into
    // ImageTaskDispatcher::Execute, and requesting a pass messes up that timing. Instead, there's a separate
    // RequestExecution_TestHook that manually does that.
}

void ImageTaskDispatcher::RequestExecution_TestHook()
{
    IFCFAILFAST(m_core->ExecuteOnUIThread(this, ReentrancyBehavior::BlockReentrancy));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Callback handler for processing tasks. Runs on Framework thread
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ImageTaskDispatcher::Execute()
{
    // m_shuttingdown is only accessed from framework thread.
    if (!m_shuttingdown &&
        (!m_throttledExecution_TestHook || m_numberOfTasksAllowedToDispatch_TestHook > 0))
    {
        ::InterlockedExchange(&m_hasCallbackQueued, FALSE);

        //
        // Note: Take a count of the number of tasks currently queued up, and only execute that many tasks. The case we
        // want to avoid is a task queuing more tasks to handle a failure (as CSvgImageSource::OnDownloadImageAvailableImpl
        // does in response to a device lost error), which can create an infinite loop of the ImageTaskDispatcher
        // constantly executing the new tasks without giving the UI thread a chance to tick (which recovers from the
        // device lost error).
        //
        // This means we count up how many tasks there are before we start executing. At the end, check whether the queue
        // is empty (i.e. everything dispatched and executed without queueing any more) or whether there are still tasks
        // in the queue (i.e. some task queued another while executing). If there is still more work, request another pass
        // of ImageTaskDispatcher::Execute.
        //
        size_t size = 0;
        size_t sizeOfNewlyQueuedWork = 0;
        bool hasMoreWork = false;

        {
            auto guard = m_taskLock.lock();
            size = m_tasks.size();

            if (m_throttledExecution_TestHook)
            {
                size = std::min(size, m_numberOfTasksAllowedToDispatch_TestHook);
            }
        }

        GraphicsTelemetry::ImageTaskDispatcher_Execute(size);

        for (size_t i = 0; i < size; i++)
        {
            xref_ptr<IImageTask> task;
            {
                auto guard = m_taskLock.lock();

                if (!m_tasks.empty())
                {
                    task = std::move(m_tasks.front());
                    m_tasks.pop_front();
                }
            }

            if (task != nullptr)
            {
                IGNOREHR(task->Execute());
            }
            else
            {
                break;
            }
        }

        {
            auto guard = m_taskLock.lock();
            sizeOfNewlyQueuedWork = m_tasks.size();
            hasMoreWork = !m_tasks.empty();

            if (m_throttledExecution_TestHook)
            {
                m_numberOfTasksAllowedToDispatch_TestHook -= size;
                hasMoreWork &= (m_numberOfTasksAllowedToDispatch_TestHook > 0);
            }
        }

        GraphicsTelemetry::ImageTaskDispatcher_Execute_End(hasMoreWork, sizeOfNewlyQueuedWork);

        // Handle the case of a task queueing another - there are new tasks in the queue, so request another pass
        // of ImageTaskDispatcher::Execute on the UI thread.
        if (hasMoreWork && m_core != nullptr)
        {
            // When the next task executes, block reentrancy by having CoreMessaging PauseNewDispatch. We're seeing
            // instances where loading a decoder kicks off an RPC call, which does a ModalLoop to pump messages, which
            // dispatches another imaging task or runs a UI thread tick, causing reentrancy. We want to avoid these
            // cases of reentrancy and just wait until the imaging task is done executing.
            IFC_RETURN(m_core->ExecuteOnUIThread(this, ReentrancyBehavior::BlockReentrancy));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the dispatcher for shutdown. Runs on Framework thread
//
//------------------------------------------------------------------------

void ImageTaskDispatcher::Shutdown()
{
    auto guard = m_taskLock.lock();

    // m_shuttingdown is only accessed from framework thread.
    m_shuttingdown = true;

    // Clear all tasks since the ImageTaskDispatcher is being shutdown
    ::InterlockedExchange(&m_hasCallbackQueued, FALSE);

    m_tasks.clear();
}
