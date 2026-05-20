// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Adapted for the public SDK from TAEF's Tailored.h
// Note: There are two of these things. One in test\native\external\inc and one in test\infra\client\inc. And they have
// different RunOnUIThread implementations.

#pragma once

#include <TestEvent.h>
#include <TestServices.h>
#include "HostingDispatcher.h"
#include <XamlLogging.h>

extern "C" __declspec(dllimport) HRESULT __stdcall Thread_Wait_For(HANDLE handle, unsigned long milliseconds);

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        using namespace std::chrono_literals;

        template<typename TDelegateInterface, typename TCallback>
        Microsoft::WRL::ComPtr<TDelegateInterface> AgileCallback(TCallback&& callback) noexcept
        {
            using TDelegateInterface2 = Microsoft::WRL::Implements<
                Microsoft::WRL::RuntimeClassFlags<wrl::ClassicCom>,
                TDelegateInterface,
                Microsoft::WRL::FtmBase>;

            return Microsoft::WRL::Callback<TDelegateInterface2, TCallback>(Microsoft::WRL::Details::Forward<TCallback>(callback));
        }

        template <typename TFunction>
        void RunOnUIThread(const TFunction& function)
        {
            RunOnDispatcherThread(HostingDispatcher::Get()->GetDispatcher(), function);
        }

        template <typename TFunction>
        void RunOnDispatcherThread(
            const wrl::ComPtr<msy::IDispatcherQueue>& dispatcherQueue,
            const TFunction& function,
            msy::DispatcherQueuePriority priority = msy::DispatcherQueuePriority::DispatcherQueuePriority_Normal,
            std::chrono::milliseconds timeout = 5min)
        {
            wrl::ComPtr<msy::IDispatcherQueue2> dq2;
            LogThrow_IfFailed(dispatcherQueue.As(&dq2));
            boolean hasThreadAccess = false;
            LogThrow_IfFailed(dq2->get_HasThreadAccess(&hasThreadAccess));
            if (hasThreadAccess)
            {
                function();
            }
            else
            {
                Event completedEvent(L"UIOperationCompleted");

                boolean success;
                auto dqCallback = AgileCallback<msy::IDispatcherQueueHandler>([&]() -> HRESULT
                {
                    auto scopeExit = wil::scope_exit([&] {
                        WEX::SafeInvoke([&]() -> bool { completedEvent.Set(); return true; });
                    });
                    // because this this will be run on the core dispatcher if it throws we
                    // both don't want that failure to bubble up to the CoreDispatcher itself
                    // and we want to capture that failure and surface it in the test execution
                    // path. WEX::SafeInvoke will catch typical exceptions, log them, and set
                    // the test to failing.

                    WEX::SafeInvoke([&]() -> bool { function(); return true; });

                    return S_OK;
                });

                WEX::Common::Throw::IfNull(dispatcherQueue.Get(), L"Dispatch callback could not be allocated.");

                dispatcherQueue->TryEnqueueWithPriority(priority, dqCallback.Get(), &success);
                WEX::Common::Throw::IfFalse(success, E_FAIL, L"DispatcherQueue failed to queue item.");

                completedEvent.WaitFor(timeout);
            }
        }

        // Need to keep a version of this function that uses Windows.System.DispatcherQueue.
        // It is still needed in three places where the test infra is trying to retrieve the DispatcherQueue with only access to CoreWindow and running not on the required thread:
        // dxaml\test\infra\client\lib\HostingDispatcher.cpp
        // dxaml\test\infra\client\lib\TestServices.cpp
        // dxaml\test\infra\client\lib\WindowHelper.cpp
        // ...do we really? UWPs aren't supported anymore.
        template <typename TFunction>
        void RunOnDispatcherThread(
            const wrl::ComPtr<wsy::IDispatcherQueue>& spDispatcher,
            const TFunction& function,
            wsy::DispatcherQueuePriority priority = wsy::DispatcherQueuePriority::DispatcherQueuePriority_Normal,
            std::chrono::milliseconds timeout = 5min)
        {
            HostingDispatcher* hostingHelper = HostingDispatcher::Get();

            if(hostingHelper->IsUIThread())
            {
                function();
            }
            else
            {
                Event completedEvent(L"UIOperationCompletedWindowsDispacherQueue");

                boolean success;
                auto dqCallback = AgileCallback<wsy::IDispatcherQueueHandler>([&]() -> HRESULT
                {
                    auto scopeExit = wil::scope_exit([&] {
                        WEX::SafeInvoke([&]() -> bool { completedEvent.Set(); return true; });
                    });
                    // because this this will be run on the core dispatcher if it throws we
                    // both don't want that failure to bubble up to the CoreDispatcher itself
                    // and we want to capture that failure and surface it in the test execution
                    // path. WEX::SafeInvoke will catch typical exceptions, log them, and set
                    // the test to failing.

                    WEX::SafeInvoke([&]() -> bool { function(); return true; });

                    return S_OK;
                });

                WEX::Common::Throw::IfNull(spDispatcher.Get(), L"Dispatch callback could not be allocated.");

                spDispatcher->TryEnqueueWithPriority(priority, dqCallback.Get(), &success);
                WEX::Common::Throw::IfFalse(success, E_FAIL, L"DispatcherQueue failed to queue item.");

                completedEvent.WaitFor(timeout);
            }
        }

        static WEX::Common::String GetTestDeploymentDir()
        {
            WEX::Common::String deploymentDir;
            LogThrow_IfFailed(
                WEX::TestExecution::RuntimeParameters::TryGetValue(WEX::TestExecution::RuntimeParameterConstants::c_szTestDeploymentDir, deploymentDir));
            if(deploymentDir.Right(1) != "\\")
            {
                deploymentDir.Append(L"\\");
            }
            return deploymentDir;
        }
    }
} } } }

namespace XamlOneCoreTransforms {
    // OneCoreTransforms not supported yet
    constexpr bool IsEnabled() { return false; }
}