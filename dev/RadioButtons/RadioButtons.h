// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

#include "RadioButtons.g.h"
#include "RadioButtons.properties.h"
#include "ColumnMajorUniformToLargestGridLayout.h"

class ChildHandlers : public winrt::implements<ChildHandlers, winrt::IInspectable>
{
public:
    winrt::ToggleButton::Checked_revoker checkedRevoker;
    winrt::ToggleButton::Unchecked_revoker uncheckedRevoker;
};

class RadioButtons :
    public ReferenceTracker<RadioButtons, winrt::implementation::RadioButtonsT>,
    public RadioButtonsProperties
{

public:
    RadioButtons();
    // IFrameworkElement
    void OnApplyTemplate();

    winrt::UIElement ContainerFromItem(winrt::IInspectable const& item);
    winrt::UIElement ContainerFromIndex(int index);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    GlobalDependencyProperty s_childHandlersProperty{ nullptr };

    //Test hooks helpers, only function while m_testHooksEnabled = true
    void SetTestHooksEnabled(bool enabled);
    ~RadioButtons();
    int GetRows();
    int GetColumns();
    int GetLargerColumns();

private:
    enum class MissStrategy
    {
        next,
        previous,
        aroundLeft,
        aroundRight,
    };

    void OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args);
    void OnRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void KeyDownHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args);
    void KeyUpHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args);
    void OnRepeaterElementPrepared(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args);
    void OnRepeaterElementClearing(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args);
    void OnRepeaterElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);
    void OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnSelectionChanged(const winrt::IInspectable&, const winrt::SelectionModelSelectionChangedEventArgs& args);
    void OnChildChecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnChildUnchecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnChildKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnChildGotFocus(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&);

    void UpdateItemsSource();
    winrt::IInspectable GetItemsSource();
    void UpdateMaximumColumns();

    void UpdateSelectedItem();
    void UpdateSelectedIndex();

    void UpdateSelectionDPs(const int newIndex);

    bool MoveFocusNext();
    bool MoveFocusPrevious();
    bool MoveFocusRight(const winrt::UIElement& focusedElement);
    bool MoveFocusLeft(const winrt::UIElement& focusedElement);
    bool MoveFocus(int initialIndexIncrement, MissStrategy missStrategy);
    static std::tuple<bool, int, int> GetNextIndex(
        MissStrategy missStrategy,
        int focusedIndex,
        const std::vector<int>& visited,
        int originalFocusedIndex,
        int fromIndex,
        int distance,
        int itemCount,
        int maxColumns);

    static int ColumnFromIndex(int index, int itemCount, int maxColumns);
    static int IncrementForRightMove(int index, int itemCount, int maxColumns);
    static int IncrementForLeftMove(int index, int itemCount, int maxColumns);
    static int IncrementForHorizontalMove(int index, int itemCount, int maxColumns, int numberOfSmallerColumnsToAccept);

    bool m_isControlDown{ false };

    tracker_ref<winrt::ItemsRepeater> m_repeater{ this };

    winrt::SelectionModel m_selectionModel{};
    winrt::SelectionModel::SelectionChanged_revoker m_selectionChangedRevoker{};
    winrt::Control::Loaded_revoker m_repeaterLoadedRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceChanged{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_repeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_repeaterElementClearingRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_repeaterElementIndexChangedRevoker{};

    //Test hooks helpers, only function while m_testHooksEnabled == true
    bool m_testHooksEnabled{ false };
    void OnLayoutChanged(const winrt::ColumnMajorUniformToLargestGridLayout&, const winrt::IInspectable&);
    winrt::event_token m_layoutChangedToken{};
    void AttachToLayoutChanged();
    void DetatchFromLayoutChanged();
    ColumnMajorUniformToLargestGridLayout* GetLayout();

    static constexpr wstring_view s_repeaterName{ L"InnerRepeater"sv };
    static constexpr wstring_view s_childHandlersPropertyName{ L"ChildHandlers"sv };
};
