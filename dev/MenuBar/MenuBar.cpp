// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "MenuBar.h"
#include "MenuBarItem.h"
#include "Vector.h"
#include "VectorIterator.h"
#include "MenuBarAutomationPeer.h"

MenuBar::MenuBar()
{
    SetDefaultStyleKey(this);

    auto items = winrt::make<ObservableVector<winrt::MenuBarItem>>();
    SetValue(s_ItemsProperty, items);
}

// IUIElement / IUIElementOverridesHelper
winrt::AutomationPeer MenuBar::OnCreateAutomationPeer()
{
    return winrt::make<MenuBarAutomationPeer>(*this);
}

void MenuBar::OnApplyTemplate()
{
    SetUpTemplateParts();
}

void MenuBar::SetUpTemplateParts()
{
    winrt::IControlProtected thisAsControlProtected = *this;
    if (auto layoutRoot = GetTemplateChildT<winrt::Grid>(L"LayoutRoot", thisAsControlProtected))
    {
        m_layoutRoot.set(layoutRoot);
    }
    if (auto contentRoot = GetTemplateChildT<winrt::ItemsControl>(L"ContentRoot", thisAsControlProtected))
    {
        if (contentRoot.try_as<winrt::IUIElement5>())
        {
            contentRoot.XYFocusKeyboardNavigation(winrt::XYFocusKeyboardNavigationMode::Enabled);
        }

        auto observableVector = Items().try_as<winrt::IObservableVector<winrt::MenuBarItem>>();
        contentRoot.ItemsSource(observableVector);

        m_contentRoot.set(contentRoot);
    }
}

void MenuBar::RequestPassThroughElement(const winrt::Microsoft::UI::Xaml::Controls::MenuBarItem& menuBarItem)
{
    // To enable switching flyout on hover, every menubar item needs the MenuBar root to include it for hit detection with flyouts open
    winrt::get_self<MenuBarItem>(menuBarItem)->AddPassThroughElement(m_layoutRoot.get());
}

void MenuBar::IsFlyoutOpen(bool state)
{
    m_isFlyoutOpen = state;
}

