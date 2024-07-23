// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemPresenterTemplateSettings.h"
#include "NavigationViewItemPresenter.g.h"
#include "NavigationViewHelper.h"
#include "NavigationViewItemPresenter.properties.h"

class NavigationViewItem;

class NavigationViewItemPresenter:
    public ReferenceTracker<NavigationViewItemPresenter, winrt::implementation::NavigationViewItemPresenterT>,
    public NavigationViewItemPresenterProperties
{
public:
    NavigationViewItemPresenter();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // IFrameworkElementOverrides
    bool GoToElementStateCore(winrt::hstring const& stateName, bool useTransitions);

    winrt::UIElement GetSelectionIndicator();

    void UpdateContentLeftIndentation(double leftIndentation);

    void RotateExpandCollapseChevron(bool isExpanded);

    void UpdateCompactPaneLength(double compactPaneLength,bool shouldUpdate);

    void UpdateClosedCompactVisualState(bool isTopLevelItem, bool isClosedCompact);

    void LoadChevron();

private:
    NavigationViewItem * GetNavigationViewItem();
    void UpdateMargin();
    void UnhookEventsAndClearFields();
    void ResetTrackedPointerId();
    bool IgnorePointerId(const winrt::PointerRoutedEventArgs& args);

    void OnExpandCollapseChevronPointerPressed(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnExpandCollapseChevronPointerReleased(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnExpandCollapseChevronPointerExited(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnExpandCollapseChevronPointerCanceled(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnExpandCollapseChevronPointerCaptureLost(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);

    bool m_isChevronPressed{ false };
    double m_compactPaneLengthValue { 40 };
    double m_leftIndentation{ 0 };
    uint32_t m_trackedPointerId{ 0 };

    NavigationViewItemHelper<NavigationViewItemPresenter> m_helper{ this };
    tracker_ref<winrt::Grid> m_contentGrid{ this };
    tracker_ref<winrt::ContentPresenter> m_infoBadgePresenter{ this };
    tracker_ref<winrt::Grid> m_expandCollapseChevron{ this };

    winrt::UIElement::PointerPressed_revoker m_expandCollapseChevronPointerPressedRevoker{};
    winrt::UIElement::PointerReleased_revoker m_expandCollapseChevronPointerReleasedRevoker{};
    RoutedEventHandler_revoker m_expandCollapseChevronPointerExitedRevoker{};
    RoutedEventHandler_revoker m_expandCollapseChevronPointerCanceledRevoker{};
    RoutedEventHandler_revoker m_expandCollapseChevronPointerCaptureLostRevoker{};
    
    tracker_ref<winrt::Storyboard> m_chevronExpandedStoryboard{ this };
    tracker_ref<winrt::Storyboard> m_chevronCollapsedStoryboard{ this };
};
