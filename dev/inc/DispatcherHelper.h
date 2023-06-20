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

    void RunAsync(std::function<void()> func, bool fallbackToThisThread = false) const
    {
        if (dispatcherQueue)
        {
            const auto result = dispatcherQueue.TryEnqueue(winrt::Windows::System::DispatcherQueueHandler(func));
            if (!result)
            {
                if (fallbackToThisThread)
                {
                    func();
                }
            }
        }
        else if (coreDispatcher)
        {
            auto asyncOp = coreDispatcher.TryRunAsync(winrt::CoreDispatcherPriority::Normal, winrt::DispatchedHandler(func));

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
                    func();
                }
            });
        }
        else
        {
            if (fallbackToThisThread)
            {
                func();
            }
        }
    }

    auto DispatcherQueue() { return dispatcherQueue; }

private:
    winrt::Windows::System::DispatcherQueue dispatcherQueue{ nullptr };
    winrt::CoreDispatcher coreDispatcher{ nullptr };
};
