// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <Windows.h>
#include <vector>
#include <memory>
#include <TestEvent.h>
#include "ThreadedJobQueue.h"
#include "ThreadedJobQueueUnitTests.h"

using namespace Microsoft::UI::Xaml::Tests::Common;

HINSTANCE g_hInstance = nullptr;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Threading {

bool ThreadedJobQueueUnitTests::ClassSetup()
{
    g_hInstance = GetModuleHandle(nullptr);
    return true;
}

void ThreadedJobQueueUnitTests::QueueSingleJob()
{
    auto threadEvent = std::make_shared<Event>();

    ThreadedJobQueue jobQueue(
        ThreadedJobQueue::ThreadingModel::Simple,
        false /* terminateOnInactivity */);

    jobQueue.QueueJob([threadEvent](HWND)
    {
        threadEvent->Set();
    });

    // Check that the job was processed
    threadEvent->WaitForDefault();
}

void ThreadedJobQueueUnitTests::QueueMultipleJobs()
{
    QueueMultipleJobsHelper(
        ThreadedJobQueue::ThreadingModel::Simple,
        true /* terminateOnInactivity */);
}

void ThreadedJobQueueUnitTests::QueueMultipleJobsTerminateEarly()
{
    const auto JobCount = 10;
    ThreadedJobQueue jobQueue(
        ThreadedJobQueue::ThreadingModel::Simple,
        true /* terminateOnInactivity */);

    for (auto i = 0; i < JobCount; i++)
    {
        jobQueue.QueueJob([](HWND)
        {
            // Make sure the jobs don't terminate early since this
            // tests that the jobQueue destructor effectively clears
            // the queue and terminates the thread.
            ::Sleep(1);
        });
    }
}

void ThreadedJobQueueUnitTests::QueueMultipleJobsThreadRestart()
{
    const auto JobCount = 2;
    std::vector<std::shared_ptr<Event>> allThreadEvents;
    ThreadedJobQueue jobQueue(
        ThreadedJobQueue::ThreadingModel::Simple,
        true /* terminateOnInactivity */);

    for (auto i = 0; i < JobCount; i++)
    {
        auto threadEvent = std::make_shared<Event>();

        jobQueue.QueueJob([threadEvent](HWND)
        {
            threadEvent->Set();
        });

        // Give it enough time that the thread terminates and has to be recreated.
        // Equivalent to ThreadTimeoutMs + 10ms
        ::Sleep(1010);

        allThreadEvents.push_back(threadEvent);
    }

    // Check that all jobs were processed
    for (const auto& threadEvent : allThreadEvents)
    {
        threadEvent->WaitForDefault();
    }
}

void ThreadedJobQueueUnitTests::QueueMultipleJobsSTA()
{
    QueueMultipleJobsHelper(
        ThreadedJobQueue::ThreadingModel::ComSingleThreadedApartment,
        true /* terminateOnInactivity */);
}

void ThreadedJobQueueUnitTests::QueueMultipleJobsMTA()
{
    QueueMultipleJobsHelper(
        ThreadedJobQueue::ThreadingModel::ComMultiThreadedApartment,
        true /* terminateOnInactivity */);
}

void ThreadedJobQueueUnitTests::QueueMultipleJobsHelper(
    ThreadedJobQueue::ThreadingModel threadingModel,
    bool terminateOnInactivity)
{
    const auto JobCount = 10;
    std::vector<std::shared_ptr<Event>> allThreadEvents;
    ThreadedJobQueue jobQueue(
        threadingModel,
        terminateOnInactivity);

    for (auto i = 0; i < JobCount; i++)
    {
        auto threadEvent = std::make_shared<Event>();

        jobQueue.QueueJob([threadEvent] (HWND)
        {
            threadEvent->Set();
        });

        // Give a small enough time between queueing each job to make sure the thread
        // tests spinning with no work and accepting new jobs after a period of time.
        ::Sleep(10);

        allThreadEvents.push_back(threadEvent);
    }

    // Check that all jobs were processed
    for (const auto& threadEvent : allThreadEvents)
    {
        threadEvent->WaitForDefault();
    }
}

void ThreadedJobQueueUnitTests::QueueJobExtendsShutdown()
{
    auto jobStartedEvent = std::make_shared<Event>();
    auto jobCompletedEvent = std::make_shared<Event>();

    {
        ThreadedJobQueue jobQueue(
            ThreadedJobQueue::ThreadingModel::Simple,
            false /* terminateOnInactivity */);

        jobQueue.QueueJob([&](HWND)
        {
            jobStartedEvent->Set();
            Sleep(1000);
            jobCompletedEvent->Set();
        });

        // Don't start the tear down until after we've started the job.
        jobStartedEvent->WaitForDefault();
    }

    // Exiting the scope will start the destruction of the job queue.
    // We expect the long running job will force the shutdown code to wait for the thread to exit.

    // Check that the job was processed, and we didn't hit any failures internally while shutting down.
    jobCompletedEvent->WaitForDefault();
}

} } } } } }
