// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RepeaterTestHooks.g.h"
#include "WinEventLogLevels.h"

class RepeaterTestHooks :
    public winrt::implementation::RepeaterTestHooksT<RepeaterTestHooks>
{
public:
    winrt::event_token BuildTreeCompletedImpl(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value); // subscribe
    void BuildTreeCompletedImpl(winrt::event_token const& token); // unsubscribe
    void NotifyBuildTreeCompletedImpl();

    static com_ptr<RepeaterTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks->get_strong();
    }

    static winrt::event_token BuildTreeCompleted(winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable> const& value); // subscribe
    static void BuildTreeCompleted(winrt::event_token const& token); // unsubscribe
    static void NotifyBuildTreeCompleted();

    static winrt::IInspectable CreateRepeaterElementFactoryGetArgs();
    static winrt::IInspectable CreateRepeaterElementFactoryRecycleArgs();
    static int GetElementFactoryElementIndex(winrt::IInspectable const& getArgs);

    static hstring GetLayoutId(winrt::IInspectable const& layout);
    static void SetLayoutId(winrt::IInspectable const& layout, const hstring& id);

private:
    static RepeaterTestHooks* s_testHooks;

    static void EnsureHooks();

private:
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable>> m_buildTreeCompleted;
};