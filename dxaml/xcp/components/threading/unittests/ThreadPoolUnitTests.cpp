// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <TestEvent.h>
#include "ThreadPoolService.h"
#include "ThreadPoolUnitTests.h"

using namespace Microsoft::UI::Xaml::Tests::Common;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Threading {

bool ThreadPoolUnitTests::ClassSetup()
{
    return (wf::Initialize(RO_INIT_MULTITHREADED) == S_OK);
}

bool ThreadPoolUnitTests::ClassCleanup()
{
    wf::Uninitialize();
    return true;
}

void ThreadPoolUnitTests::RunAsyncFreeThreaded()
{
    auto spThreadPoolFactory = ThreadPoolService::GetInstance().GetThreadPoolFactory();

    auto spTimerCalled = std::make_shared<Event>();
    auto asyncJob = wrl::Callback<FreeThreaded<wsyt::IWorkItemHandler>>(
        [spTimerCalled]
        (wf::IAsyncAction* timer) -> HRESULT
        {
            spTimerCalled->Set();
            return S_OK;
        });

    wrl::ComPtr<wf::IAsyncAction> spAsyncAction;
    VERIFY_SUCCEEDED(spThreadPoolFactory->RunAsync(asyncJob.Get(), &spAsyncAction));
    spTimerCalled->WaitForDefault();
}

void ThreadPoolUnitTests::CreateTimerFreeThreaded()
{
    auto spThreadPoolTimerFactory = ThreadPoolService::GetInstance().GetThreadPoolTimerFactory();

    wf::TimeSpan delay;
    delay.Duration = 10000; // 1 millisecond

    auto spTimerCalled = std::make_shared<Event>();
    auto callback = wrl::Callback<FreeThreaded<wsyt::ITimerElapsedHandler>>(
        [spTimerCalled]
        (wsyt::IThreadPoolTimer* timer) -> HRESULT
        {
            spTimerCalled->Set();
            return S_OK;
        });

    wrl::ComPtr<wsyt::IThreadPoolTimer> timer;
    VERIFY_SUCCEEDED(spThreadPoolTimerFactory->CreateTimer(callback.Get(), delay, &timer));
    spTimerCalled->WaitForDefault();
}

void ThreadPoolUnitTests::RunAsyncUIThread()
{
    auto spThreadPoolFactory = ThreadPoolService::GetInstance().GetThreadPoolFactory();

    auto spTimerCalled = std::make_shared<Event>();
    auto asyncJob = wrl::Callback<wsyt::IWorkItemHandler>(
        [spTimerCalled]
        (wf::IAsyncAction* timer) -> HRESULT
        {
            spTimerCalled->Set();
            return S_OK;
        });

    wrl::ComPtr<wf::IAsyncAction> spAsyncAction;
    VERIFY_SUCCEEDED(spThreadPoolFactory->RunAsync(asyncJob.Get(), &spAsyncAction));
    spTimerCalled->WaitForDefault();
}

void ThreadPoolUnitTests::CreateTimerUIThread()
{
    auto spThreadPoolTimerFactory = ThreadPoolService::GetInstance().GetThreadPoolTimerFactory();

    wf::TimeSpan delay;
    delay.Duration = 10000; // 1 millisecond

    auto spTimerCalled = std::make_shared<Event>();
    auto callback = wrl::Callback<wsyt::ITimerElapsedHandler>(
        [spTimerCalled]
        (wsyt::IThreadPoolTimer* timer) -> HRESULT
        {
            spTimerCalled->Set();
            return S_OK;
        });

    wrl::ComPtr<wsyt::IThreadPoolTimer> timer;
    VERIFY_SUCCEEDED(spThreadPoolTimerFactory->CreateTimer(callback.Get(), delay, &timer));
    spTimerCalled->WaitForDefault();
}

} } } } } }
