// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SelectorBar.h"
#include "SelectorBarItem.h"
#include "SelectorBarTestHooks.h"
#include "SelectorBarSelectionChangedEventArgs.h"
#include "RuntimeProfiler.h"
#include "SharedHelpers.h"
#include "Vector.h"

// Change to 'true' to turn on debugging outputs in Output window
bool SelectorBarTrace::s_IsDebugOutputEnabled{ false };
bool SelectorBarTrace::s_IsVerboseDebugOutputEnabled{ false };

SelectorBar::SelectorBar()
{
    SELECTORBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SelectorBar);

    EnsureProperties();
    SetDefaultStyleKey(this);

    auto items = winrt::make<ObservableVector<winrt::SelectorBarItem>>();
    SetValue(s_ItemsProperty, items);

    m_loadedRevoker = Loaded(winrt::auto_revoke, { this, &SelectorBar::OnLoaded });
}

SelectorBar::~SelectorBar()
{
    SELECTORBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

// Invoked by SelectorBarTestHooks
winrt::ItemsView SelectorBar::GetItemsViewPart() const
{
    return m_itemsView.get();
}

void SelectorBar::OnApplyTemplate()
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    m_itemsViewSelectedItemPropertyChangedRevoker.revoke();

    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_itemsView.set(GetTemplateChildT<winrt::ItemsView>(s_itemsViewPartName, controlProtected));

    if (const auto& itemsView = m_itemsView.get())
    {
        m_itemsViewSelectedItemPropertyChangedRevoker = RegisterPropertyChanged(itemsView, winrt::ItemsView::SelectedItemProperty(), { this, &SelectorBar::OnItemsViewSelectedItemPropertyChanged });

        if (auto scrollView = itemsView.ScrollView())
        {
            // Allow the items to scroll horizontally if the SelectorBar is sized too small horizontally (the default
            // ScrollView ContentOrientation is Vertical for vertical scrolling which is not useful for the SelectorBar).
            scrollView.ContentOrientation(winrt::ScrollingContentOrientation::Horizontal);
        }
    }

    if (SelectedItem() != nullptr)
    {
        ValidateSelectedItem();
        UpdateItemsViewSelectionFromSelectedItem();
    }
}

void SelectorBar::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    const auto dependencyProperty = args.Property();

    if (dependencyProperty == s_SelectedItemProperty)
    {
        SELECTORBAR_TRACE_INFO_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, "SelectedItem property changed");

        ValidateSelectedItem();
        UpdateItemsViewSelectionFromSelectedItem();
        RaiseSelectionChanged();
    }
}

void SelectorBar::OnItemsViewSelectedItemPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyProperty& args)
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateSelectedItemFromItemsView();
}

void SelectorBar::OnGotFocus(
    winrt::RoutedEventArgs const& args)
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnGotFocus(args);

    const auto selectedItem = SelectedItem();
    const auto selectedItemIsFocusable = selectedItem == nullptr ? false : SharedHelpers::IsFocusableElement(selectedItem);

    if (selectedItem == nullptr || !selectedItemIsFocusable)
    {
        // Automatically attempt to select an item when the SelectorBar got focus without any selection.
        if (const auto& itemsView = m_itemsView.get())
        {
            const int currentItemIndex = itemsView.CurrentItemIndex();

            if (currentItemIndex != -1)
            {
                // Select the current item index
                itemsView.Select(currentItemIndex);
            }
        }

        if (SelectedItem() == nullptr)
        {
            SelectFirstFocusableItem();
        }
    }
}

void SelectorBar::OnLoaded(
    const winrt::IInspectable& sender,
    const winrt::RoutedEventArgs& args)
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (SelectedItem() == nullptr)
    {
        UpdateSelectedItemFromItemsView();
    }
}

void SelectorBar::RaiseSelectionChanged()
{
    if (m_selectionChangedEventSource)
    {
        SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

        auto selectorBarSelectionChangedEventArgs = winrt::make_self<SelectorBarSelectionChangedEventArgs>();

        m_selectionChangedEventSource(*this, *selectorBarSelectionChangedEventArgs);
    }
}

void SelectorBar::SelectFirstFocusableItem()
{
    SELECTORBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    MUX_ASSERT(SelectedItem() == nullptr);

    const auto items = Items();
    const auto itemsCount = items.Size();

    for (uint32_t itemIndex = 0; itemIndex < itemsCount; itemIndex++)
    {
        const auto item = items.GetAt(itemIndex);

        if (item != nullptr && SharedHelpers::IsFocusableElement(item))
        {
            SelectedItem(item);
            break;
        }
    }
}

void SelectorBar::UpdateItemsViewSelectionFromSelectedItem()
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (const auto& itemsView = m_itemsView.get())
    {
        const auto selectedItem = SelectedItem();

        if (selectedItem == nullptr)
        {
            itemsView.DeselectAll();
        }
        else
        {
            uint32_t selectedIndex{};
            const bool result = Items().IndexOf(selectedItem, selectedIndex);
            MUX_ASSERT(result);
            itemsView.Select(selectedIndex);
        }
    }
}

void SelectorBar::UpdateSelectedItemFromItemsView()
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    if (const auto& itemsView = m_itemsView.get())
    {
        const auto selectedItem = SelectedItem();
        const auto itemsViewSelectedItem = itemsView.SelectedItem().as<winrt::SelectorBarItem>();

        if (selectedItem != itemsViewSelectedItem)
        {
            SelectedItem(itemsViewSelectedItem);
        }
    }
}

void SelectorBar::ValidateSelectedItem()
{
    SELECTORBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    const auto selectedItem = SelectedItem();

    if (selectedItem)
    {
        uint32_t selectedIndex{};
        const bool result = Items().IndexOf(selectedItem, selectedIndex);

        if (!result)
        {
            throw winrt::hresult_invalid_argument(L"SelectedItem must be an element of Items.");
        }
    }
}
