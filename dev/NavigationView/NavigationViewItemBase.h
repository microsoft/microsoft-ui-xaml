// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.g.h"
#include "NavigationViewHelper.h"
#include "NavigationViewItemBase.properties.h"
#include "NavigationViewItemTemplateSettings.h"

class NavigationViewItemBase :
    public ReferenceTracker<NavigationViewItemBase, winrt::implementation::NavigationViewItemBaseT, winrt::composable>,
	public NavigationViewItemBaseProperties
{
public:
    // Promote all overrides that our derived classes want into virtual so that our shim will call them.
    // IFrameworkElementOverrides
    virtual void OnApplyTemplate()
    {
        __super::OnApplyTemplate();
    }

    // IUIElementOverrides
    virtual winrt::AutomationPeer OnCreateAutomationPeer()
    {
        return __super::OnCreateAutomationPeer();
    }

    // IContentControlOverrides
    virtual void OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent)
    {
        __super::OnContentChanged(oldContent, newContent);
    }

    // IControlOverrides overrides
    virtual void OnGotFocus(winrt::RoutedEventArgs const& e)
    {
        __super::OnGotFocus(e);
    }

    virtual void OnLostFocus(winrt::RoutedEventArgs const& e)
    {
        __super::OnLostFocus(e);
    }

    virtual void OnNavigationViewListPositionChanged() {}

    NavigationViewListPosition Position();
    void Position(NavigationViewListPosition value);
    
    winrt::NavigationView GetNavigationView();
    winrt::SplitView GetSplitView();
    winrt::NavigationViewList GetNavigationViewList();

private:
    NavigationViewListPosition m_position{ NavigationViewListPosition::LeftNav };

public:
    NavigationViewItemBase();
};