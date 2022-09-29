// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SharedHelpers.h"

class DispatcherHelper
{
public:

    DispatcherHelper(const winrt::DependencyObject& dependencyObject = nullptr)
    {
        if (SharedHelpers::IsDispatcherQueueAvailable())
        {
            dispatcherQueue = winrt::Windows::System::DispatcherQueue::GetForCurrentThread();
        }

        if (!dispatcherQueue)
        {
            if (dependencyObject)
            {
                coreDispatcher = dependencyObject.Dispatcher();
            }
            else if (auto window = winrt::Windows::UI::Core::CoreWindow::GetForCurrentThread())
            {
                coreDispatcher = window.Dispatcher();
            }
        }
    }

    // RunAsync for callers that only want to run on the UI thread
    void RunAsync(std::function<void()> func) const
    {
        RunAsync(
            [=](bool isOnUIThread) {
                MUX_ASSERT(isOnUIThread);
                func();
            }
        );
    }

    // RunAsync -- if callers want the option of always running, the parameter to the callback says whether we ran on the UI thread or not.
    void RunAsync(std::function<void(bool /* isOnUIThread */)> func, bool fallbackToThisThread = false) const
    {
        if (dispatcherQueue)
        {
            auto result = dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler(
                [=]()
                {
                    func(true);
                }));
            if (!result)
            {
                if (fallbackToThisThread)
                {
                    func(false);
                }
            }
        }
        else if (coreDispatcher)
        {
            auto asyncOp = coreDispatcher.TryRunAsync(winrt::CoreDispatcherPriority::Normal, winrt::DispatchedHandler(
                [=]()
                {
                    func(true);
                }));

            asyncOp.Completed([func, fallbackToThisThread](auto& asyncInfo, auto& asyncStatus)
            {
                bool reRunOnThisThread = false;

                if (asyncStatus == winrt::AsyncStatus::Completed)
                {
                    auto succeeded = asyncInfo.GetResults();
                    if (!succeeded)
                    {
                        if (fallbackToThisThread)
                        {
                            reRunOnThisThread = true;
                        }
                    }
                }

                if (reRunOnThisThread)
                {
                    func(false);
                }
            });
        }
        else
        {
            if (fallbackToThisThread)
            {
                func(false);
            }
        }
    }

    auto DispatcherQueue() { return dispatcherQueue; }

private:
    winrt::Windows::System::DispatcherQueue dispatcherQueue{ nullptr };
    winrt::CoreDispatcher coreDispatcher{ nullptr };
};
