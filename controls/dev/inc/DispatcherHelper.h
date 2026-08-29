// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SharedHelpers.h"

class DispatcherHelper
{
public:

    DispatcherHelper(const winrt::DependencyObject& dependencyObject = nullptr)
    {
        dispatcherQueue = winrt::DispatcherQueue::GetForCurrentThread();
    }

    void RunAsync(std::function<void()> func, bool fallbackToThisThread = false) const
    {
        auto result = dispatcherQueue.TryEnqueue(winrt::DispatcherQueueHandler(func));
        if (!result)
        {
            if (fallbackToThisThread)
            {
                func();
            }
        }
    }

private:
    winrt::DispatcherQueue dispatcherQueue{ nullptr };
};
