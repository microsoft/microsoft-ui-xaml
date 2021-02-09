// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadioMenuFlyoutItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

RadioMenuFlyoutItem::RadioMenuFlyoutItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadioMenuFlyoutItem);

    m_InternalIsCheckedChangedRevoker = RegisterPropertyChanged(*this, winrt::ToggleMenuFlyoutItem::IsCheckedProperty(), { this, &RadioMenuFlyoutItem::OnInternalIsCheckedChanged });

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
        // Since this item is checked, uncheck all siblings
        if (auto parent = winrt::VisualTreeHelper::GetParent(*this))
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
        }

    }
}
