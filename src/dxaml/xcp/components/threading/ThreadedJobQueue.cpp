// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.h>
#include <functional>
#include <queue>
#include <mutex>
#include <chrono>
#include <wil\resource.h>
#include <Clock.h>
#include <MUX-ETWEvents.h>
#include "ThreadedJobQueue.h"

extern HINSTANCE g_hInstance;

static ATOM s_urlmonMessageWindowClass = 0;

namespace
{
    HWND CreateMessageWindow()
    {
        // RegisterClassEx should not be called on multiple threads at the same time. If it does,
        // one thread will get an "already registered" failure. Use a mutex to ensure that
        // doesn't happen.
        static std::mutex s_mutex;
        std::lock_guard<std::mutex> lock(s_mutex);

        if (!s_urlmonMessageWindowClass)
        {
            // Create a message-only window to receive urlmon download notifications.
            // The class will be unregistered when it terminates.
            WNDCLASSEX wndClass = {};
            wndClass.cbSize = sizeof(wndClass);
            wndClass.lpfnWndProc = DefWindowProc;
            wndClass.hInstance = g_hInstance;
            wndClass.lpszClassName = L"WinUI_urlmonMessageWindow";

            s_urlmonMessageWindowClass = ::RegisterClassEx(&wndClass);
            IFCW32FAILFAST(s_urlmonMessageWindowClass); // should be non-zero, otherwise fail with GetLastError()
            FAIL_FAST_ASSERT(s_urlmonMessageWindowClass != 0);
        }

        // CreateWindowEx requires loword to be the atom and hiword to be 0 then packed in a pointer to WCHAR
        WCHAR* packedClassAtom = reinterpret_cast<WCHAR*>(static_cast<uintptr_t>(MAKELONG(s_urlmonMessageWindowClass, 0)));
        return ::CreateWindowEx(0, packedClassAtom, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, g_hInstance, nullptr);
    }

    void ProcessMessagePump()
    {
        MSG msg = {};
        while (::PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            ::DispatchMessage(&msg);
        }
    }
}

ThreadedJobQueue::ThreadedJobQueue(
    ThreadingModel threadingModel,
    bool terminateOnInactivity)
    : m_threadingModel(threadingModel)
    // Initialize the m_keepAliveCount to 0 if the thread should terminate after a period of inactivity.
    // Otherwise initialize to 1 in the case that the thread lifetime should be tied to this objects lifetime.
    , m_keepAliveCount(terminateOnInactivity ? 0 : 1)
{
}

ThreadedJobQueue::~ThreadedJobQueue()
{
    // Use a local variable to check if thread shutdown is necessary so the mutex isn't
    // held while triggering the thread event.
    bool waitForThreadCompletion = false;

    // NOTE: Scope blocks are used for RAII mutex locks on selective sections of code to properly
    //       handle errors and ensure the mutex is always released.  The lock shouldn't be
    //       taken for the entire thread or when processing the job (which could take an indeterminate
    //       amount of time).
    {
        std::lock_guard<std::recursive_mutex> guard(m_jobMutex);

        // Clear the job queue
        m_jobQueue.clear();

        if (m_jobThread != nullptr)
        {
            ASSERT(m_threadInterrupt != nullptr);

            m_shutdown = true;
            ::SetEvent(m_threadInterrupt.get());

            waitForThreadCompletion = true;
        }
    }

    // Wait for the thread to complete if it is active
    if (waitForThreadCompletion)
    {
        // Wait for the thread to complete.
        // Things are shutting down anyway, so ignore failures.
        TraceThreadedJobQueueShutdownWaitBegin(reinterpret_cast<uint64_t>(this));
        auto waitCode = ::WaitForSingleObject(m_jobThread.get(), INFINITE);
        ASSERT(waitCode == WAIT_OBJECT_0);
        TraceThreadedJobQueueShutdownWaitEnd(reinterpret_cast<uint64_t>(this), waitCode);
    }

    // Thread is complete, close the handles.
    // NOTE: These will call CloseHandle
    m_threadInterrupt.reset();
    m_jobThread.reset();
}

void ThreadedJobQueue::QueueJob(
    std::function<void(HWND)> job)
{
    std::lock_guard<std::recursive_mutex> guard(m_jobMutex);

    TraceThreadedJobQueueSubmitJobInfo(
        reinterpret_cast<uint64_t>(this),
        reinterpret_cast<uint64_t>(job.target<void()>()));

    // Push the job and wake the thread up to process the request.
    m_jobQueue.push_back(std::move(job));

    EnsureThreadActive();
    ::SetEvent(m_threadInterrupt.get());
}

wistd::unique_ptr<ThreadedJobQueueDeferral> ThreadedJobQueue::GetDeferral()
{
    return wil::make_unique_failfast<ThreadedJobQueueDeferral>(this);
}

void ThreadedJobQueue::IncrementDeferredKeepAliveCount()
{
    std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
    m_keepAliveCount++;

    TraceThreadedJobQueueUpdateExternalRefInfo(reinterpret_cast<uint64_t>(this), m_keepAliveCount);
}

void ThreadedJobQueue::DecrementDeferredKeepAliveCount()
{
    std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
    ASSERT(m_keepAliveCount > 0);
    m_keepAliveCount--;

    // Interrupt at 0 ref count to see if the thread should terminate.
    if (m_keepAliveCount == 0)
    {
        ::SetEvent(m_threadInterrupt.get());
    }

    TraceThreadedJobQueueUpdateExternalRefInfo(reinterpret_cast<uint64_t>(this), m_keepAliveCount);
}

void ThreadedJobQueue::EnsureThreadActive()
{
    std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
    if (!m_isThreadActive)
    {
        m_lastJobTime = Jupiter::HighResolutionClock::now();

        wil::unique_handle threadInterrupt(::CreateEvent(
            nullptr,
            FALSE /* manualReset */,
            FALSE /* initialState */,
            nullptr));
        m_threadInterrupt = std::move(threadInterrupt);
        FAIL_FAST_ASSERT(m_threadInterrupt.get() != nullptr);

        wil::unique_handle jobThread(::CreateThread(
            nullptr,
            0,
            StaticThreadCallback,
            reinterpret_cast<void*>(this),
            0,
            nullptr));
        m_jobThread = std::move(jobThread);
        FAIL_FAST_ASSERT(m_jobThread.get() != nullptr);

        m_isThreadActive = true;
    }
}

unsigned long WINAPI ThreadedJobQueue::StaticThreadCallback(
    void* voidThis)
{
    ThreadedJobQueue* threadedJobQueue = reinterpret_cast<ThreadedJobQueue*>(voidThis);
    threadedJobQueue->ThreadCallback();

    return 0;
}

void ThreadedJobQueue::ThreadCallback()
{
    TraceThreadedJobQueueThreadLifetimeBegin(reinterpret_cast<uint64_t>(this));
    auto traceEtwOnExit = wil::scope_exit([&]
    {
        TraceThreadedJobQueueThreadLifetimeEnd(reinterpret_cast<uint64_t>(this));
    });

    const std::chrono::milliseconds ThreadTimeoutMs(1000);

    ASSERT(m_threadInterrupt.get() != nullptr);

    // Setup the thread to be able to use COM and setup some variable state
    // so a mutex lock isn't necessary for some variables.
    bool comInitialized = false;
    bool singleApartmentThreaded = false;
    HANDLE threadInterruptHandle = nullptr;
    {
        std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
        switch (m_threadingModel)
        {
        default:
            // Unknown enumeration added
            ASSERT(false);
            break;
        case ThreadingModel::Simple:
            // No initialization necessary
            break;
        case ThreadingModel::ComSingleThreadedApartment:
            IFCFAILFAST(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
            comInitialized = true;
            singleApartmentThreaded = true;
            break;
        case ThreadingModel::ComMultiThreadedApartment:
            IFCFAILFAST(CoInitializeEx(nullptr, COINIT_MULTITHREADED));
            comInitialized = true;
            break;
        }

        threadInterruptHandle = m_threadInterrupt.get();
    }

    auto comUninitializeOnExit = wil::scope_exit([&comInitialized]
    {
        if (comInitialized)
        {
            CoUninitialize();
        }
    });

    // Setup a message window to handle the message pump for this thread.
    auto hwnd = CreateMessageWindow();
    FAIL_FAST_ASSERT(hwnd != nullptr);
    auto destroyWindowOnExit = wil::scope_exit([hwnd]
    {
        DestroyWindow(hwnd);
    });

    // Run the thread keep alive loop looking for work or shutdown request.
    do
    {
        std::function<void(HWND)> job = nullptr;
        bool queueEmpty = true;

        // Step 1: If single apartment threaded, process the message pump.
        if (singleApartmentThreaded)
        {
            // Process any window message for this thread.  Job's like urlmon require the message pump to be
            // checked in order to handle asynchronous requests.
            ProcessMessagePump();
        }

        // Step 2: Check the queue and terminate on inactivity
        {
            std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
            queueEmpty = m_jobQueue.empty();
            if (queueEmpty)
            {
                if (m_keepAliveCount == 0)
                {
                    // If there hasn't been a job for a certain amount of time, shut down the thread.
                    auto diffTime = Jupiter::HighResolutionClock::now() - m_lastJobTime;
                    if (diffTime > ThreadTimeoutMs)
                    {
                        m_isThreadActive = false;

                        // Terminate the thread
                        return;
                    }
                }
            }
            else
            {
                // Pop an entry
                job = std::move(m_jobQueue.front());
                m_jobQueue.pop_front();
            }
        }

        // Step 3: Run the queued job if there is one and update the timestamp on completion.
        if (job != nullptr)
        {
            TraceThreadedJobQueueJobBegin(
                reinterpret_cast<uint64_t>(this),
                reinterpret_cast<uint64_t>(job.target<void()>()));

            // Run the job without the mutex since it is not in the queue anymore and holding the mutex
            // could block threads from queueing more jobs.
            job(hwnd);

            TraceThreadedJobQueueJobEnd(
                reinterpret_cast<uint64_t>(this),
                reinterpret_cast<uint64_t>(job.target<void()>()));

            {
                // Remember the last time a job was processed
                // This should be done after the job finishes execution so the job
                // time doesn't contribute to the timeout when checking how long
                // the queue has been empty.
                std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
                m_lastJobTime = Jupiter::HighResolutionClock::now();
            }
        }

        // Step 4: When the queue is done processing, sleep the thread until interrupted.
        //         There is also a periodic timeout to check the message queue and whether
        //         to keep the thread alive.
        if (queueEmpty)
        {
            // If there are keep alive references, set the timeout to infinite.  If it reaches zero, an interrupt
            // will be sent.  Otherwise, wakup after the thread timeout interval to check for thread termination.
            DWORD waitTimeout = (m_keepAliveCount > 0) ? INFINITE : static_cast<DWORD>(ThreadTimeoutMs.count());

            // Yield the thread if there is nothing in the queue and it is waiting for more work.
            // It yields using MsgWaitForMultipleObjects which allows it  to interrupt for the purpose of
            // processing the queue, pump more messages, checking the keep alive count or to shut down.
            TraceThreadedJobQueueThreadWaitBegin(reinterpret_cast<uint64_t>(this));

            VERIFY_COND(::MsgWaitForMultipleObjects(
                1,
                &threadInterruptHandle,
                FALSE,
                waitTimeout,
                QS_ALLPOSTMESSAGE),
                != WAIT_FAILED);

            TraceThreadedJobQueueThreadWaitEnd(reinterpret_cast<uint64_t>(this));

            {
                std::lock_guard<std::recursive_mutex> guard(m_jobMutex);
                if (m_shutdown)
                {
                    m_isThreadActive = false;

                    // Terminate the thread
                    return;
                }
            }
        }
    } while (true);
}
