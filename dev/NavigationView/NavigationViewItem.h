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

    // These functions are ambiguous with NavigationViewItemBase, disambiguate 
    using NavigationViewItemProperties::EnsureProperties;
    using NavigationViewItemProperties::ClearProperties;

    // IFrameworkElementOverrides
    void OnApplyTemplate() override;

    void OnLoaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&);

    // Property change callbacks
    void OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIconPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMenuItemsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMenuItemsSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHasUnrealizedChildrenPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    winrt::UIElement GetSelectionIndicator() const;

    // IUIElement / IUIElementOverridesHelper
    winrt::AutomationPeer OnCreateAutomationPeer() override;

    // IContentControlOverrides / IContentControlOverridesHelper
    void OnContentChanged(winrt::IInspectable const& oldContent, winrt::IInspectable const& newContent) override;

    // IControlOverrides overrides
    void OnGotFocus(winrt::RoutedEventArgs const& e) override;
    void OnLostFocus(winrt::RoutedEventArgs const& e) override;

    // VisualState is maintained by NavigationViewItem. but actual state should be apply to 
    // NavigationViewItemPresenter. But NavigationViewItemPresenter is created after NavigationViewItem. 
    // It provides a chance for NavigationViewItemPresenter to request visualstate refresh
    void UpdateVisualStateNoTransition();

    void ShowHideChildren();
    bool ShouldRepeaterShowInFlyout() const;

    winrt::ItemsRepeater GetRepeater() const { return m_repeater.get(); };

    void OnExpandCollapseChevronTapped(const winrt::IInspectable& sender, const winrt::TappedRoutedEventArgs& args);
    void RotateExpandCollapseChevron(bool isExpanded);
    bool IsRepeaterVisible() const;
    void PropagateDepthToChildren(int depth);
    bool HasChildren();

private:
    winrt::UIElement const GetPresenterOrItem() const;
    NavigationViewItemPresenter* GetPresenter() const;

    void UpdateNavigationViewItemToolTip();
    void SuggestedToolTipChanged(winrt::IInspectable const& newContent);
    void OnNavigationViewItemBaseDepthChanged() override;
    void OnNavigationViewItemBaseIsSelectedChanged() override;
    void OnNavigationViewItemBasePositionChanged() override;

    void OnPresenterPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerEntered(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerMoved(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPresenterPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    void ResetTrackedPointerId();
    bool IgnorePointerId(const winrt::PointerRoutedEventArgs& args);
    void OnSplitViewPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void UpdateCompactPaneLength();
    void UpdateIsClosedCompact();

    void UpdateVisualStateForIconAndContent(bool showIcon, bool showContent);
    void UpdateVisualStateForNavigationViewPositionChange();
    void UpdateVisualStateForKeyboardFocusedState();
    void UpdateVisualStateForToolTip();
    void UpdateVisualStateForPointer();
    void UpdateVisualStateForChevron();
    void UpdateVisualStateForClosedCompact();

    void UpdateVisualState(bool useTransitions);
    bool ShouldShowIcon();
    bool ShouldShowContent();
    bool ShouldEnableToolTip() const;
    bool IsOnLeftNav() const;
    bool IsOnTopPrimary() const;
    bool IsOutOfControlBounds(const winrt::Point& point);

    void UpdateRepeaterItemsSource();
    void OnItemsSourceViewChanged(const winrt::IInspectable& sender, const winrt::NotifyCollectionChangedEventArgs& args);
    void ReparentRepeater();
    void OnFlyoutClosing(const winrt::IInspectable& sender, const winrt::FlyoutBaseClosingEventArgs& args);
    void UpdateItemIndentation();
    void ShowSelectionIndicator(bool visible);
    void ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    void ProcessPointerOver(const winrt::PointerRoutedEventArgs& args);
    void HookInputEvents(const winrt::IControlProtected& controlProtected);
    void UnhookInputEvents();
    void UnhookEventsAndClearFields();

    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};
    PropertyChanged_revoker m_splitViewCompactPaneLengthChangedRevoker{};

    winrt::UIElement::PointerPressed_revoker m_presenterPointerPressedRevoker{};
    winrt::UIElement::PointerEntered_revoker m_presenterPointerEnteredRevoker{};
    winrt::UIElement::PointerMoved_revoker m_presenterPointerMovedRevoker{};
    RoutedEventHandler_revoker m_presenterPointerReleasedRevoker{};
    RoutedEventHandler_revoker m_presenterPointerExitedRevoker{};
    RoutedEventHandler_revoker m_presenterPointerCanceledRevoker{};
    RoutedEventHandler_revoker m_presenterPointerCaptureLostRevoker{};

    winrt::ItemsRepeater::ElementPrepared_revoker m_repeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_repeaterElementClearingRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceViewCollectionChangedRevoker{};

    winrt::FlyoutBase::Closing_revoker m_flyoutClosingRevoker{};
    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};

    tracker_ref<winrt::ToolTip> m_toolTip{ this };
    NavigationViewItemHelper<NavigationViewItem> m_helper{ this };
    tracker_ref<winrt::NavigationViewItemPresenter> m_navigationViewItemPresenter{ this };
    tracker_ref<winrt::IInspectable> m_suggestedToolTipContent{ this };
    tracker_ref<winrt::ItemsRepeater> m_repeater{ this };
    tracker_ref<winrt::Grid> m_flyoutContentGrid{ this };
    tracker_ref<winrt::Grid> m_rootGrid{ this };

    bool m_isClosedCompact{ false };

    bool m_appliedTemplate{ false };
    bool m_hasKeyboardFocus{ false };

    // Visual state tracking
    winrt::Pointer m_capturedPointer{ nullptr };
    uint32_t m_trackedPointerId{ 0 };
    bool m_isPressed{ false };
    bool m_isPointerOver{ false };

    bool m_isRepeaterParentedToFlyout{ false };
};
