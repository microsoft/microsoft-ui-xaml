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
    void OnMaxDisplayedPagesChanged(const int oldValue);
    void OnNavigationButtonVisibilityChanged(winrt::PipsPagerButtonVisibility visibility, const wstring_view collapsedStateName);
    void OnPipStyleChanged(winrt::Size& pipSize, const winrt::Style& pipStyle);

    /* Dependency property for pip buttons revokers */
    GlobalDependencyProperty s_pipButtonHandlersProperty;

private:
    /* UI updating */
    void UpdateNavigationButtonVisualStates();
    bool IsOutOfControlBounds(winrt::Point point);
    void UpdateIndividualNavigationButtonVisualState(
        bool hiddenOnEdgeCondition,
        winrt::PipsPagerButtonVisibility visibility,
        const wstring_view visibleStateName,
        const wstring_view hiddenStateName,
        const wstring_view enabledStateName,
        const wstring_view disabledStateName);
    void SetDesiredPipSize(winrt::Size& pipSize, const winrt::Style& style);

    /* Eventing */
    void RaiseSelectedIndexChanged();

    /* Interaction event listeners */
    void OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
  

    /* Pips Logic */
    void UpdateVerticalPips(const int numberOfPages, const int maxDisplayedPages);
    void MovePipIdentifierToElement(int index);
    void OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void SetVerticalPipsSVMaxSize();
    double CalculateSVSize(double defaultPipSize, double selectedPipSize, int numberOfPages);
    void ScrollToCenterOfViewport(winrt::UIElement sender);

    /* Refs */
    tracker_ref<winrt::ItemsRepeater> m_verticalPipsRepeater{ this };
    tracker_ref<winrt::FxScrollViewer> m_verticalPipsScrollViewer{ this };

    /* Revokers */
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_verticalPipsElementPreparedRevoker{};

    /* Elements */
    winrt::IObservableVector<IInspectable> m_verticalPipsElements{};

    /* Additional variables class variables*/
    winrt::Size m_defaultPipSize{ 0.0,0.0 };
    winrt::Size m_selectedPipSize{ 0.0, 0.0 };
    int m_lastSelectedPageIndex{ -1 };
    int m_lastMaxDisplayedPages{ 0 };
    bool m_isPointerOver{ false };
};
