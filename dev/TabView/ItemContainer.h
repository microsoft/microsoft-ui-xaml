// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemContainer.g.h"
#include "ItemContainer.properties.h"

class ItemContainer :
    public winrt::implementation::ItemContainerT<ItemContainer, winrt::composable>,
    public ItemContainerProperties
{

public:
    void OnIsSelectedPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnPointerPressed(const winrt::PointerRoutedEventArgs& args);
    void OnPointerReleased(const winrt::PointerRoutedEventArgs& args);
    void OnPointerEntered(const winrt::PointerRoutedEventArgs& args);
    void OnPointerExited(const winrt::PointerRoutedEventArgs& args);
    void OnPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    void OnPointerCaptureLost(const winrt::PointerRoutedEventArgs& args);
    virtual void OnApplyTemplate();

    void OnSelectionChanged(const winrt::SelectionModel& sender, const winrt::SelectionModelSelectionChangedEventArgs& args);

    int RepeatedIndex();
    void RepeatedIndex(int index);
    void SelectionModel(const winrt::SelectionModel& value);
    void UpdateVisualState(bool useTransitions);

private:
    // Visual state tracking
    bool m_isPressed{ false };
    bool m_isPointerOver{ false };

    int m_repeatedIndex{ -1 };
    winrt::SelectionModel m_selectionModel{ nullptr };
    winrt::SelectionModel::SelectionChanged_revoker m_selectionChangedRevoker{};
};
