﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

#include "RadioButtons.g.h"
#include "RadioButtons.properties.h"
#include "ColumnMajorUniformToLargestGridLayout.h"

// This is an object that RadioButtons intends to attach to its child RadioButton elements.
// It contains the revokers for the events on RadioButton that RadioButttons listens to
// in order to manage selection.  Attaching the revokers to the object allows the parent
// RadioButtons the ability to "Set it and forget it" since the lifetime of the RadioButton
// and these event registrations are now intrically linked.
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

    void OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args);
    void OnRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void KeyDownHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args);
    void KeyUpHandler(const winrt::IInspectable&, const winrt::KeyRoutedEventArgs& args);
    void OnRepeaterElementPrepared(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args);
    void OnRepeaterElementClearing(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args);
    void OnRepeaterElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);
    void OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnChildChecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnChildUnchecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnChildGotFocus(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&);

    void UpdateItemsSource();
    winrt::IInspectable GetItemsSource();
    void UpdateMaximumColumns();

    void UpdateSelectedItem();
    void UpdateSelectedIndex();

    void Select(int index);
    winrt::IInspectable GetDataAtIndex(int index, bool containerIsChecked);

    winrt::FindNextElementOptions GetFindNextElementOptions();
    bool MoveFocusNext();
    bool MoveFocusPrevious();
    bool MoveFocus(int initialIndexIncrement);

    bool m_isControlDown{ false };
    int m_selectedIndex{ -1 };
    bool m_currentlySelecting{ false };

    tracker_ref<winrt::ItemsRepeater> m_repeater{ this };

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
    com_ptr<ColumnMajorUniformToLargestGridLayout> GetLayout();

    static constexpr wstring_view s_repeaterName{ L"InnerRepeater"sv };
    static constexpr wstring_view s_childHandlersPropertyName{ L"ChildHandlers"sv };
};
