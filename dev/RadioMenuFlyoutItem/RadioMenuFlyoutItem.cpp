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

    if (!s_selectionMap)
    {
        // Ensure that this object exists
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
            UncheckPreviousIfNecessary();
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
        UncheckPreviousIfNecessary();
    }
}

void RadioMenuFlyoutItem::UncheckPreviousIfNecessary()
{
    if (IsChecked())
    {
        const auto groupName = GroupName();

        if (s_selectionMap.HasKey(groupName))
        {
            if (const auto previousCheckedItem = s_selectionMap.Lookup(groupName))
            {
                // Uncheck the previously checked item.
                previousCheckedItem.IsChecked(false);
            }
        }
        s_selectionMap.Insert(groupName, *this);
    }
}

void RadioMenuFlyoutItem::OnAreCheckStatesEnabledPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (unbox_value<bool>(args.NewValue()))
    {
        if (auto const& subMenu = sender.try_as<winrt::MenuFlyoutSubItem>())
        {
            // Every time the submenu is loaded, see if it contains a checked RadioMenuFlyoutItem or not.
            subMenu.Loaded(
            {
                [subMenu](winrt::IInspectable const& sender, auto const&)
                {
                    bool isAnyItemChecked = false;
                    for (auto const& item : subMenu.Items())
                    {
                        if (auto const& radioItem = item.try_as<winrt::RadioMenuFlyoutItem>())
                        {
                            isAnyItemChecked = isAnyItemChecked || radioItem.IsChecked();
                        }
                    }

                    winrt::VisualStateManager::GoToState(subMenu, isAnyItemChecked ? L"Checked" : L"Unchecked", false);
                }
            });
        }
    }
}
