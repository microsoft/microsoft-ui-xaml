// Copyright (c) Microsoft Corporation. All rights reserved.
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
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnPointerEntered(const winrt::PointerRoutedEventArgs& args);
    void OnPointerExited(const winrt::PointerRoutedEventArgs& args);
    void OnPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    void OnKeyDown(const winrt::KeyRoutedEventArgs& args);

    /* Property changed handlers */
    void OnNumberOfPagesChanged();
    void OnSelectedPageIndexChanged(const int oldValue);
    void OnMaxVisualIndicatorsChanged();
    void OnNavigationButtonVisibilityChanged(const winrt::PipsPagerButtonVisibility visibility, const wstring_view& collapsedStateName);
    void OnOrientationChanged();

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
    void ScrollToCenterOfViewport(const winrt::UIElement sender);
    double CalculateScrollViewerSize(const double defaultPipSize, const double selectedPipSize, const int numberOfPages, int maxVisualIndicators);
    void UpdateSelectedPip(const int index);

    /* Eventing */
    void RaiseSelectedIndexChanged();

    /* Interaction event listeners */
    void OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);

    /* Pips Logic */
    void UpdatePipsItems(const int numberOfPages, int maxVisualIndicators);
    void OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);

    /* Refs */
    tracker_ref<winrt::ItemsRepeater> m_pipsPagerRepeater{ this };
    tracker_ref<winrt::FxScrollViewer> m_pipsPagerScrollViewer{ this };

    /* Revokers */
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_pipsPagerElementPreparedRevoker{};

    /* Items */
    winrt::IObservableVector<IInspectable> m_pipsPagerItems{};

    /* Additional variables class variables*/
    winrt::Size m_defaultPipSize{ 0.0,0.0 };
    winrt::Size m_selectedPipSize{ 0.0, 0.0 };
    int m_lastSelectedPageIndex{ -1 };
    bool m_isPointerOver{ false };
};
