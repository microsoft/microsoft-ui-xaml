﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "PipsPager.g.h"
#include "PipsPager.properties.h"

class PipsPagerViewItemRevokers : public winrt::implements<PipsPagerViewItemRevokers, winrt::IInspectable>
{
public:
    winrt::Button::Click_revoker clickRevoker{};
};

class PipsPager :
    public ReferenceTracker<PipsPager, winrt::implementation::PipsPagerT>,
    public PipsPagerProperties
{
public:
    PipsPager();

    /* IFrameworkElement */
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    /* Accessibility */
    winrt::AutomationPeer OnCreateAutomationPeer();
    void UpdateSizeOfSetForElements(const int numberOfPages, const int numberOfItems);

    void OnPointerEntered(const winrt::PointerRoutedEventArgs& args);
    void OnPointerExited(const winrt::PointerRoutedEventArgs& args);
    void OnPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    void OnKeyDown(const winrt::KeyRoutedEventArgs& args);
    void LosingFocus(const IInspectable& sender, const winrt::LosingFocusEventArgs& args);
    void OnLostFocus(const winrt::RoutedEventArgs& args);
    void OnGotFocus(const winrt::RoutedEventArgs& args);

    /* Property changed handlers */
    void OnNumberOfPagesChanged();
    void OnSelectedPageIndexChanged(const int oldValue);
    void OnMaxVisiblePipsChanged();
    void OnNavigationButtonVisibilityChanged(
        const winrt::PipsPagerButtonVisibility visibility,
        const wstring_view& collapsedStateName,
        const wstring_view& disabledStateName);
    void OnOrientationChanged();

    winrt::UIElement GetSelectedItem();

    /* Dependency property for pip buttons revokers */
    GlobalDependencyProperty s_pipButtonHandlersProperty;

private:
    /* UI updating */
    void UpdateNavigationButtonVisualStates();
    void SetScrollViewerMaxSize();
    bool IsOutOfControlBounds(const winrt::Point& point);
    void UpdateIndividualNavigationButtonVisualState(
        const bool hiddenOnEdgeCondition,
        const winrt::PipsPagerButtonVisibility visibility,
        const wstring_view& visibleStateName,
        const wstring_view& hiddenStateName,
        const wstring_view& enabledStateName,
        const wstring_view& disabledStateName);
    winrt::Size GetDesiredPipSize(const winrt::Style& style);
    void ScrollToCenterOfViewport(const winrt::UIElement sender, const int index);
    double CalculateScrollViewerSize(const double defaultPipSize, const double selectedPipSize, const int numberOfPages, int maxVisualIndicators);
    void UpdateSelectedPip(const int index);
    void UpdatePipOrientation(const winrt::Control& pip);
    void ApplyStyleToPipAndUpdateOrientation(const winrt::FrameworkElement& pip, const winrt::Style& style);
    /* Eventing */
    void RaiseSelectedIndexChanged();

    /* Interaction event listeners */
    void OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPipsAreaGettingFocus(const IInspectable& sender, const winrt::GettingFocusEventArgs& args);
    void OnPipsAreaBringIntoViewRequested(const IInspectable& sender, const winrt::BringIntoViewRequestedEventArgs& args);
    void OnScrollViewerBringIntoViewRequested(const IInspectable& sender, const winrt::BringIntoViewRequestedEventArgs& args);

    /* Pips Logic */
    void UpdatePipsItems(const int numberOfPages, int maxVisualIndicators);
    void OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnElementIndexChanged(const winrt::ItemsRepeater& repeater, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);

    /* Refs */
    tracker_ref<winrt::ItemsRepeater> m_pipsPagerRepeater{ this };
    tracker_ref<winrt::FxScrollViewer> m_pipsPagerScrollViewer{ this };
    tracker_ref<winrt::Button> m_previousPageButton{ this };
    tracker_ref<winrt::Button> m_nextPageButton{ this };

    /* Revokers */
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_pipsPagerElementPreparedRevoker{};
    winrt::UIElement::GettingFocus_revoker m_pipsAreaGettingFocusRevoker{};
    winrt::ItemsRepeater::BringIntoViewRequested_revoker m_pipsAreaBringIntoViewRequestedRevoker{};
    winrt::FxScrollViewer::BringIntoViewRequested_revoker m_scrollViewerBringIntoViewRequestedRevoker{};
    /* Items */
    winrt::IObservableVector<int> m_pipsPagerItems{};

    /* Additional variables class variables*/
    winrt::Size m_defaultPipSize{ 0.0,0.0 };
    winrt::Size m_selectedPipSize{ 0.0, 0.0 };
    int m_lastSelectedPageIndex{ -1 };
    bool m_isPointerOver{ false };
    bool m_isFocused{ false };
};
