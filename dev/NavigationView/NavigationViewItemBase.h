// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.g.h"
#include "NavigationViewHelper.h"
#include "NavigationViewItemBase.properties.h"

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

    NavigationViewRepeaterPosition Position();
    void Position(NavigationViewRepeaterPosition value);
    virtual void OnNavigationViewRepeaterPositionChanged() {}

    void Depth(int depth);
    int Depth();
    virtual void OnNavigationViewItemBaseDepthChanged() {}
    
    winrt::NavigationView GetNavigationView();
    winrt::SplitView GetSplitView();
    void SetNavigationViewParent(winrt::NavigationView const& navigationView);

    // TODO: Constant is a temporary mesure. Potentially expose using TemplateSettings.
    static constexpr int c_itemIndentation = 25;

    void IsTopLevelItem(bool isTopLevelItem) { m_isTopLevelItem = isTopLevelItem; };
    bool IsTopLevelItem() { return m_isTopLevelItem; };

protected:

    winrt::weak_ref<winrt::NavigationView> m_navigationView{ nullptr };

private:

    NavigationViewRepeaterPosition m_position{ NavigationViewRepeaterPosition::LeftNav };
    int m_depth{ 0 };
    bool m_isTopLevelItem{ false };
};
