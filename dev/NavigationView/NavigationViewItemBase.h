// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.g.h"
#include "NavigationViewHelper.h"

class NavigationViewItemBase :
    public ReferenceTracker<NavigationViewItemBase, winrt::implementation::NavigationViewItemBaseT, winrt::composable>
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

    virtual void OnPointerReleased(winrt::PointerRoutedEventArgs const& args) {
        __super::OnPointerReleased(args);
    };

    virtual void OnKeyDown(winrt::KeyRoutedEventArgs const& args) {
        __super::OnKeyDown(args);
    };

    virtual void OnKeyUp(winrt::KeyRoutedEventArgs const& args) {
        __super::OnKeyUp(args);
    };

    virtual void OnNavigationViewListPositionChanged() {}

    NavigationViewListPosition Position();
    void Position(NavigationViewListPosition value);
    
    winrt::NavigationView GetNavigationView();
    winrt::SplitView GetSplitView();
    winrt::NavigationViewList GetNavigationViewList();

    void SetNavigationViewParent(winrt::NavigationView const& navigationView);

protected:
    winrt::weak_ref<winrt::NavigationView> m_navigationView{ nullptr };

private:
    NavigationViewListPosition m_position{ NavigationViewListPosition::LeftNav };

    // Event Tokens
    winrt::SelectionModel::SelectionChanged_revoker m_selectionChangedEventToken{};

};
