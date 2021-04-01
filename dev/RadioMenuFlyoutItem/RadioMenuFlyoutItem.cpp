// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadioMenuFlyoutItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

winrt::IMap<winrt::hstring, winrt::RadioMenuFlyoutItem> RadioMenuFlyoutItem::s_selectionMap = nullptr;

RadioMenuFlyoutItem::RadioMenuFlyoutItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadioMenuFlyoutItem);

    m_InternalIsCheckedChangedRevoker = RegisterPropertyChanged(*this, winrt::ToggleMenuFlyoutItem::IsCheckedProperty(), { this, &RadioMenuFlyoutItem::OnInternalIsCheckedChanged });

    // ### probably not here -- make an ensure method or something
    if (!s_selectionMap)
    {
        //single_threaded_map single_threaded_observable_map
        s_selectionMap = winrt::single_threaded_map<winrt::hstring, winrt::RadioMenuFlyoutItem>();
    }

    SetDefaultStyleKey(this);
}

void RadioMenuFlyoutItem::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsCheckedProperty)
    {
        if (InternalIsChecked() != IsChecked())
        {
            m_isSafeUncheck = true;
            InternalIsChecked(IsChecked());
            m_isSafeUncheck = false;
            // ### do I need this for some reason??
            UpdateSiblings();
        }
    }
}

void RadioMenuFlyoutItem::OnInternalIsCheckedChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    if (!InternalIsChecked())
    {
        if (m_isSafeUncheck)
        {
            // The uncheck is due to another radio button being checked -- that's all right.
            IsChecked(false);
        }
        else
        {
            // The uncheck is due to user interaction -- not allowed.
            InternalIsChecked(true);
        }
    }
    else if (!IsChecked())
    {
        IsChecked(true);
        UpdateSiblings();
    }
}

void RadioMenuFlyoutItem::UpdateSiblings()
{
    if (IsChecked())
    {
        const auto groupName = GroupName();

        if (s_selectionMap.HasKey(groupName))
        {
            if (const auto previousCheckedItem = s_selectionMap.Lookup(groupName))
            {
                previousCheckedItem.IsChecked(false);
            }
        }
        s_selectionMap.Insert(groupName, *this);


        // Since this item is checked, uncheck all siblings
        /*if (auto parent = winrt::VisualTreeHelper::GetParent(*this))
        {
            const int childrenCount = winrt::VisualTreeHelper::GetChildrenCount(parent);
            for (int i = 0; i < childrenCount; i++)
            {
                auto child = winrt::VisualTreeHelper::GetChild(parent, i);
                if (auto radioItem = child.try_as<winrt::RadioMenuFlyoutItem>())
                {
                    if (winrt::get_self<RadioMenuFlyoutItem>(radioItem) != this
                        && radioItem.GroupName() == GroupName())
                    {
                        radioItem.IsChecked(false);
                    }
                }
            }
        }*/
    }
}

//-----------------------------

//### this should probably actually be the GroupName string or something like that.
void RadioMenuFlyoutItem::OnContainsRadioMenuFlyoutItemsPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args)
{
    OutputDebugString(L"I see this property!\n");

    if (auto const& subMenu = sender.try_as<winrt::MenuFlyoutSubItem>())
    {
        subMenu.Loaded(
        {
            [subMenu](winrt::IInspectable const& sender, auto const&)
            {
                OutputDebugString(L"Loaded now!\n");

                bool isAnyItemChecked = false;
                for (auto const& item : subMenu.Items())
                {
                    // ### and check the group name?
                    if (auto const& radioItem = item.try_as<winrt::RadioMenuFlyoutItem>())
                    {
                        OutputDebugString(radioItem.IsChecked() ? L"Item is checked\n" : L"Item is unchecked\n");
                        isAnyItemChecked = isAnyItemChecked || radioItem.IsChecked();
                    }
                }
                OutputDebugString(isAnyItemChecked ? L"I should be checked\n" : L"I should be unchecked\n");
                winrt::VisualStateManager::GoToState(subMenu, isAnyItemChecked ? L"Checked" : L"Unchecked", false);
            }
        });

    }
}
