// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsRepeater.h"
#include "WinEventLogLevels.h"
#include "RepeaterTestHooks.g.h"

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
    static hstring GetLayoutId(winrt::IInspectable const& layout);
    static void SetLayoutId(winrt::IInspectable const& layout, const hstring& id);

    static int GetElementFactoryElementIndex();
    static void SetElementFactoryElementIndex(int index);
    static int GetLogItemIndex();
    static void SetLogItemIndex(int logItemIndex);

private:
    static int s_elementFactoryElementIndex;
    static RepeaterTestHooks* s_testHooks;
    static void EnsureHooks();    
    winrt::event<winrt::TypedEventHandler<winrt::IInspectable, winrt::IInspectable>> m_buildTreeCompleted;    
};
