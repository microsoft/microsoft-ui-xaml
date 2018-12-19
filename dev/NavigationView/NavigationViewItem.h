// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct bringintoview_event_revoker;

#include "NavigationViewItemBase.h"
#include "NavigationViewItem.g.h"
#include "NavigationViewItemPresenter.h"
#include "NavigationViewItem.properties.h"

class NavigationViewItem :
    public winrt::implementation::NavigationViewItemT<NavigationViewItem, NavigationViewItemBase>,
    public NavigationViewItemProperties
{
public:
    ForwardRefToBaseReferenceTracker(NavigationViewItemBase)

    NavigationViewItem();
    virtual ~NavigationViewItem();

	// These functions are ambiguous with NavigationViewItemBase, disambiguate 
	using NavigationViewItemProperties::EnsureProperties;
	using NavigationViewItemProperties::ClearProperties;

    // IFrameworkElementOverrides
    void OnApplyTemplate() override;

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    winrt::UIElement GetSelectionIndicator();
    winrt::ToolTip GetToolTip();

    // IUIElement / IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer() override;

    // IContentControlOverrides / IContentControlOverridesHelper
    void OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent) override;

    // IControlOverrides overrides
    void OnGotFocus(winrt::RoutedEventArgs const& e) override;
    void OnLostFocus(winrt::RoutedEventArgs const& e) override;
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);

    // VisualState is maintained by NavigationViewItem. but actual state should be apply to 
    // NavigationViewItemPresenter. But NavigationViewItemPresenter is created after NavigationViewItem. 
    // It provides a chance for NavigationViewItemPresenter to request visualstate refresh
    void UpdateVisualStateNoTransition();
    
    bool IsContentChangeHandlingDelayedForTopNav() { return m_isContentChangeHandlingDelayedForTopNav; }
    void ClearIsContentChangeHandlingDelayedForTopNavFlag() { m_isContentChangeHandlingDelayedForTopNav = false; }

    winrt::event_token AddExpandedChanged(winrt::TypedEventHandler<winrt::NavigationViewItem, winrt::DependencyPropertyChangedEventArgs> const& value);
    void RemoveExpandedChanged(winrt::event_token token);

    bool HasRegisteredWithViewModelForExpandEvent() { return m_registeredWithViewModelForExpandedChangedEvent; };

    void SetDepth(int depth);
    int GetDepth();

    void SetParentItem(winrt::NavigationViewItem const& item);

private:
    void UpdateNavigationViewItemToolTip();
    void SuggestedToolTipChanged(winrt::IInspectable const& newContent);
    void OnNavigationViewListPositionChanged() override;

    void OnLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnUnloaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OnSplitViewPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void UpdateCompactPaneLength();
    void UpdateIsClosedCompact();

    void UpdateVisualStateForIconAndContent(bool showIcon, bool showContent);
    void UpdateVisualStateForNavigationViewListPositionChange();
    void UpdateVisualStateForKeyboardFocusedState();
    void UpdateVisualStateForToolTip();

    void UpdateVisualState(bool useTransitions);
    bool ShouldShowIcon();
    bool ShouldShowContent();
    bool ShouldEnableToolTip();
    bool IsOnLeftNav();
    bool IsOnTopPrimary();
    
    NavigationViewItemPresenter * GetPresenter();

    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};
    PropertyChanged_revoker m_splitViewCompactPaneLengthChangedRevoker{};

    tracker_ref<winrt::ToolTip> m_toolTip{ this };
    NavigationViewItemHelper<NavigationViewItem> m_helper{ this };
    tracker_ref<winrt::NavigationViewItemPresenter> m_navigationViewItemPresenter{ this };
    tracker_ref<winrt::IInspectable> m_suggestedToolTipContent{ this };

    bool m_isClosedCompact{ false };

    bool m_appliedTemplate{ false };
    bool m_hasKeyboardFocus{ false };
    bool m_isContentChangeHandlingDelayedForTopNav{ false };

    bool m_registeredWithViewModelForExpandedChangedEvent{ false };
    event_source<winrt::TypedEventHandler<winrt::NavigationViewItem, winrt::DependencyPropertyChangedEventArgs>> m_expandedChangedEventSource{ this };

    int m_depth{ 0 };
    void UpdateItemDepth(int depth);

    tracker_ref<winrt::NavigationViewItem> m_parentItem{ this };

    void InformViewModelOfAbilityToExpand();
};
