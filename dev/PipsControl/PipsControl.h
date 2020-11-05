// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "PipsControl.g.h"
#include "PipsControl.properties.h"

class PipsControl :
    public ReferenceTracker<PipsControl, winrt::implementation::PipsControlT>,
    public PipsControlProperties
{

public:
    PipsControl();
    ~PipsControl();

    // IFrameworkElement
    void OnApplyTemplate();
    void OnPipsControlPointerEntered(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args);
    void OnPipsControlPointerExited(winrt::IInspectable sender, winrt::PointerRoutedEventArgs args);
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
       

    /* Property changed handlers */
    void OnNumberOfPagesChanged(const int oldValue);
    void OnSelectedPageIndexChange(const int oldValue);
    void OnMaxDisplayedPagesChanged(const int oldValue);


private:

    /* UI updating */
    void UpdateNavigationButtonVisualStates();
    void HideNavigationButtons();

    /* Eventing */
    void RaiseSelectedIndexChanged();

    /* Interaction event listeners */
    void OnRootGridKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnPreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnNextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& args);


    // Pips Logic
    void UpdateVerticalPips(const int numberOfPages, const int maxDisplayedPages);
    void MovePipIdentifierToElement(int index);
    void OnElementPrepared(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void setVerticalPipsSVMaxSize();
    void ScrollToCenterOfViewport(winrt::UIElement sender);

    // Tracker refs
    tracker_ref<winrt::ItemsRepeater> m_verticalPipsRepeater{ this };
    tracker_ref<winrt::FxScrollViewer> m_verticalPipsScrollViewer{ this };

    // Revokers
 
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_verticalPipsElementPreparedRevoker{};
    winrt::Grid::KeyDown_revoker m_rootGridKeyDownRevoker{};
    winrt::Grid::PointerEntered_revoker m_rootGridPointerEnteredRevoker{};
    winrt::Grid::PointerExited_revoker m_rootGridPointerExitedRevoker{};

    // Elements
    winrt::IObservableVector<IInspectable> m_verticalPipsElements{};

    int m_lastSelectedPageIndex = -1;
    int m_lastNumberOfPagesCount = 0;
    int m_lastMaxDisplayedPages = 0;
    bool m_isPointerOver = false;
};
