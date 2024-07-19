// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "SelectorBarTrace.h"
#include "SelectorBar.g.h"
#include "SelectorBar.properties.h"

class SelectorBar :
    public ReferenceTracker<SelectorBar, winrt::implementation::SelectorBarT>,
    public SelectorBarProperties
{

public:
    SelectorBar();
    ~SelectorBar();

    // Invoked by SelectorBarTestHooks
    winrt::ItemsView GetItemsViewPart() const;

    // IFrameworkElement
    void OnApplyTemplate();

    // IControlOverrides
    void OnGotFocus(
        const winrt::RoutedEventArgs& args);

    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

private:
    void OnItemsViewSelectedItemPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyProperty& args);

    void OnLoaded(
        const winrt::IInspectable& sender,
        const winrt::RoutedEventArgs& args);

    void RaiseSelectionChanged();
    void SelectFirstFocusableItem();
    void UpdateItemsViewSelectionFromSelectedItem();
    void UpdateSelectedItemFromItemsView();
    void ValidateSelectedItem();

    tracker_ref<winrt::ItemsView> m_itemsView{ this };

    winrt::FrameworkElement::Loaded_revoker m_loadedRevoker{};

    PropertyChanged_revoker m_itemsViewSelectedItemPropertyChangedRevoker{};

    static constexpr std::wstring_view s_itemsViewPartName{ L"PART_ItemsView"sv };
};
