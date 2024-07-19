// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsView.h"
#include "ItemsViewTestHooks.g.h"

class ItemsViewTestHooks :
    public winrt::implementation::ItemsViewTestHooksT<ItemsViewTestHooks>
{
public:
    static com_ptr<ItemsViewTestHooks> GetGlobalTestHooks()
    {
        return s_testHooks;
    }

    static com_ptr<ItemsViewTestHooks> EnsureGlobalTestHooks();

    static winrt::Point GetKeyboardNavigationReferenceOffset(const winrt::ItemsView& itemsView);
    static void NotifyKeyboardNavigationReferenceOffsetChanged(const winrt::ItemsView& itemsView);
    static winrt::event_token KeyboardNavigationReferenceOffsetChanged(winrt::TypedEventHandler<winrt::ItemsView, winrt::IInspectable> const& value);
    static void KeyboardNavigationReferenceOffsetChanged(winrt::event_token const& token);

    static winrt::ScrollView GetScrollViewPart(const winrt::ItemsView& itemsView);
    static winrt::ItemsRepeater GetItemsRepeaterPart(const winrt::ItemsView& itemsView);
    static winrt::SelectionModel GetSelectionModel(const winrt::ItemsView& itemsView);

private:
    static com_ptr<ItemsViewTestHooks> s_testHooks;
    winrt::event<winrt::TypedEventHandler<winrt::ItemsView, winrt::IInspectable>> m_keyboardNavigationReferenceOffsetChangedEventSource;
};
