// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsViewTestHooksFactory.h"

com_ptr<ItemsViewTestHooks> ItemsViewTestHooks::s_testHooks{};

com_ptr<ItemsViewTestHooks> ItemsViewTestHooks::EnsureGlobalTestHooks()
{
    static bool s_initialized = []()
    {
        s_testHooks = winrt::make_self<ItemsViewTestHooks>();
        return true;
    }();
    return s_testHooks;
}

winrt::Point ItemsViewTestHooks::GetKeyboardNavigationReferenceOffset(const winrt::ItemsView& itemsView)
{
    if (itemsView)
    {
        return winrt::get_self<ItemsView>(itemsView)->GetKeyboardNavigationReferenceOffset();
    }
    else
    {
        return winrt::Point{ -1.0f, -1.0f };
    }
}

void ItemsViewTestHooks::NotifyKeyboardNavigationReferenceOffsetChanged(const winrt::ItemsView& itemsView)
{
    auto hooks = EnsureGlobalTestHooks();
    if (hooks->m_keyboardNavigationReferenceOffsetChangedEventSource)
    {
        hooks->m_keyboardNavigationReferenceOffsetChangedEventSource(itemsView, nullptr);
    }
}

winrt::event_token ItemsViewTestHooks::KeyboardNavigationReferenceOffsetChanged(winrt::TypedEventHandler<winrt::ItemsView, winrt::IInspectable> const& value)
{
    auto hooks = EnsureGlobalTestHooks();
    return hooks->m_keyboardNavigationReferenceOffsetChangedEventSource.add(value);
}

void ItemsViewTestHooks::KeyboardNavigationReferenceOffsetChanged(winrt::event_token const& token)
{
    auto hooks = EnsureGlobalTestHooks();
    hooks->m_keyboardNavigationReferenceOffsetChangedEventSource.remove(token);
}

winrt::ScrollView ItemsViewTestHooks::GetScrollViewPart(const winrt::ItemsView& itemsView)
{
    if (itemsView)
    {
        return winrt::get_self<ItemsView>(itemsView)->GetScrollViewPart();
    }

    return nullptr;
}

winrt::ItemsRepeater ItemsViewTestHooks::GetItemsRepeaterPart(const winrt::ItemsView& itemsView)
{
    if (itemsView)
    {
        return winrt::get_self<ItemsView>(itemsView)->GetItemsRepeaterPart();
    }

    return nullptr;
}

winrt::SelectionModel ItemsViewTestHooks::GetSelectionModel(const winrt::ItemsView& itemsView)
{
    if (itemsView)
    {
        return winrt::get_self<ItemsView>(itemsView)->GetSelectionModel();
    }

    return nullptr;
}
