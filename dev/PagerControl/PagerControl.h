// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "PagerControl.g.h"
#include "PagerControl.properties.h"

class PagerControl :
    public ReferenceTracker<PagerControl, winrt::implementation::PagerControlT>,
    public PagerControlProperties
{

public:
    PagerControl();
    ~PagerControl();

    // IFrameworkElement
    void OnApplyTemplate();

    void RaiseSelectedIndexChanged();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnDisplayModeChanged();
    void UpdateDisplayModeAutoState();
    void OnSelectedIndexChanged(const int oldIndex);
    void OnButtonVisibilityChanged(const winrt::PagerControlButtonVisibility visibility,
        const winrt::hstring visibleStateName,
        const winrt::hstring hiddenStateName,
        const int hiddenOnEdgeSelectedIndexCriteria);
    void UpdateOnEdgeButtonVisualStates();

    void FirstButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e);
    void PreviousButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e);
    void NextButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e);
    void LastButtonClicked(const IInspectable& sender, const winrt::RoutedEventArgs& e);

private:
    int m_lastSelectedPageIndex = -1;

    winrt::Button::Click_revoker m_firstPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_previousPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_nextPageButtonClickRevoker{};
    winrt::Button::Click_revoker m_lastPageButtonClickRevoker{};

};
