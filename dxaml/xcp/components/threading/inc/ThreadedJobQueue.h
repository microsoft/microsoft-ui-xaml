// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <functional>
#include <queue>
#include <mutex>
#include <wil\resource.h>
#include <Clock.h>

class ThreadedJobQueueDeferral;

// This class provides functionality to run jobs on another thread.  There will only
// be one thread to run the jobs.
// The thread will also be created if hasn't already to run the queued jobs.
class ThreadedJobQueue final
{
    friend class ThreadedJobQueueDeferral;

public:
    // The threading model is meant to classify the usage of COM in the
    // thread jobs that are given.
    // Simple - No COM CoInitialize will be called.
    // ComSingleThreadedApartment - COM CoInitialize will be called with COINIT_APARTMENTTHREADED
    // ComMultiThreadedApartment - COM CoInitialize will be called with COINIT_MULTITHREADED
    enum class ThreadingModel
    {
        Simple,
        ComSingleThreadedApartment,
        ComMultiThreadedApartment,
    };

    // If terminateOnInactivity is specified, it will terminate after a period of inactivity otherwise
    // it will only terminate when the object is destroyed.
    ThreadedJobQueue(
        ThreadingModel threadingModel,
        bool terminateOnInactivity = true);
    ~ThreadedJobQueue();

    // Queue a job which will run on another thread.
    // Jobs should stow an exception or fail fast.  Alternatively,
    // they can use their own error reporting mechanism via lambda captures
    // or function objects.
    void QueueJob(std::function<void(HWND hwnd)> job);

    // Gets an RAII style deferral object that will keep the thread alive and running as long as the object
    // is kept around.  It is the callers responsibility to ensure all deferral objects are cleaned up
    // prior to deleting the final object.
    wistd::unique_ptr<ThreadedJobQueueDeferral> GetDeferral();

private:

    // Optional methods to track external dependencies on the thread such as waiting for the message pump
    // in ComSingleThreadedApartment mode.
    void IncrementDeferredKeepAliveCount();
    void DecrementDeferredKeepAliveCount();

    void EnsureThreadActive();

    static unsigned long WINAPI StaticThreadCallback(void*);
    void ThreadCallback();

    std::recursive_mutex m_jobMutex;
    ThreadingModel m_threadingModel = ThreadingModel::Simple;
    std::deque<std::function<void(HWND)>> m_jobQueue;
    wil::unique_handle m_jobThread;
    wil::unique_handle m_threadInterrupt;
    Jupiter::HighResolutionClock::time_point m_lastJobTime;
    bool m_shutdown = false;
    bool m_isThreadActive = false;
    uint32_t m_keepAliveCount = 0;
};

// Deferral object for RAII style keep alive of the the thread.
// The only caution with the deferral objects is to make sure they are released prior to the parent ThreadedJobQueue
// being destroyed.  This is currently the callers responsibility to alleviate additional overhead on this class from
// keeping track of all deferral objects.
class ThreadedJobQueueDeferral final
{
public:
    ThreadedJobQueueDeferral(ThreadedJobQueue* threadedJobQueue)
        : m_threadedJobQueue(threadedJobQueue)
    {
        m_threadedJobQueue->IncrementDeferredKeepAliveCount();
    }

    ~ThreadedJobQueueDeferral()
    {
        m_threadedJobQueue->DecrementDeferredKeepAliveCount();
    }

private:
    ThreadedJobQueue* m_threadedJobQueue;
};