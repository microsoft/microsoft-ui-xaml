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

winrt::AutomationPeer TabViewItem::OnCreateAutomationPeer()
{
    return winrt::make<TabViewItemAutomationPeer>(*this);
}

void TabViewItem::OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    UpdateCloseButton();
    OnHeaderPropertyChanged(nullptr);
}

void TabViewItem::UpdateCloseButton()
{
    if (auto&& closeButton = m_closeButton.get())
    {
        bool canClose = IsCloseable();
        if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
        {
            if (!tabView.CanCloseTabs())
            {
                if (!canClose || ReadLocalValue(IsCloseableProperty()) == winrt::DependencyProperty::UnsetValue())
                {
                    canClose = false;
                }
            }
        }

        closeButton.Visibility(canClose ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }
}

void TabViewItem::OnCloseButtonClick(const winrt::IInspectable&, const winrt::RoutedEventArgs&)
{
    if (auto tabView = SharedHelpers::GetAncestorOfType<winrt::TabView>(winrt::VisualTreeHelper::GetParent(*this)))
    {
        auto args = winrt::make_self<TabViewTabClosingEventArgs>(*this);
        m_tabClosingEventSource(*this, *args);

        if (!args->Cancel())
        {
            auto internalTabView = winrt::get_self<TabView>(tabView);
            internalTabView->CloseTab(*this);
        }
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

void TabViewItem::OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto toolTipContent = winrt::ToolTipService::GetToolTip(*this);
    if (!toolTipContent)
    {
        // App author has not specified a tooltip; use our own
        auto headerContent = Header();
        auto potentialString = safe_try_cast<winrt::IPropertyValue>(headerContent);

        if (potentialString && potentialString.Type() == winrt::PropertyType::String)
        {
            winrt::ToolTipService::SetToolTip(*this, headerContent);
            winrt::ToolTipService::SetPlacement(*this, winrt::Controls::Primitives::PlacementMode::Mouse);
        }
        else
        {
            winrt::ToolTipService::SetToolTip(*this, nullptr);
        }
    }
}
