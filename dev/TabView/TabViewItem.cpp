// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TabView.h"
#include "TabViewItem.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "SharedHelpers.h"

TabViewItem::TabViewItem()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TabViewItem);

    SetDefaultStyleKey(this);

    Loaded({ this, &TabViewItem::OnLoaded });
}

void TabViewItem::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_closeButton.set([this, controlProtected]() {
        auto closeButton = GetTemplateChildT<winrt::Button>(L"CloseButton", controlProtected);
        if (closeButton)
        {
            m_closeButtonClickRevoker = closeButton.Click(winrt::auto_revoke, { this, &TabViewItem::OnCloseButtonClick });
        }
        return closeButton;
    }());

    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        m_CanCloseTabsChangedRevoker = RegisterPropertyChanged(tabView, winrt::TabView::CanCloseTabsProperty(), { this, &TabViewItem::OnCloseButtonPropertyChanged });
    }
}

void TabViewItem::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    UpdateCloseButton();
}

void TabViewItem::UpdateCloseButton()
{
    if (auto&& closeButton = m_closeButton.get())
    {
        bool canClose = IsCloseable();
        if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
        {
            canClose = canClose && tabView.CanCloseTabs();
        }

        closeButton.Visibility(canClose ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

void TabViewItem::OnCloseButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        auto internalTabView = winrt::get_self<TabView>(tabView);
        internalTabView->CloseTab(*this);
    }
}

void TabViewItem::OnCloseButtonPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateCloseButton();
}

void TabViewItem::OnIsCloseablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateCloseButton();
}
