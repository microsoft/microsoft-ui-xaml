// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RepeaterTestHooksFactory.h"

#include "RepeaterTestHooks.properties.cpp"

RepeaterTestHooks* RepeaterTestHooks::s_testHooks = nullptr;

void RepeaterTestHooks::EnsureHooks()
{
    if (!s_testHooks)
    {
        s_testHooks = winrt::make_self<RepeaterTestHooks>().detach();
    }
}

winrt::event_token RepeaterTestHooks::BuildTreeCompleted(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value)
{
    EnsureHooks();
    return s_testHooks->BuildTreeCompletedImpl(value);
}

void RepeaterTestHooks::BuildTreeCompleted(winrt::event_token const& token)
{
    if (s_testHooks)
    {
        s_testHooks->BuildTreeCompletedImpl(token);
    }
}

void RepeaterTestHooks::NotifyBuildTreeCompleted()
{
    if (s_testHooks)
    {
        s_testHooks->NotifyBuildTreeCompletedImpl();
    }
}
