// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"

#include "RadioButtonsElementFactory.h"
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
    void OnRepeaterElementPrepared(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs& args);
    void OnRepeaterElementClearing(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs& args);
    void OnRepeaterElementIndexChanged(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs& args);
    void OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnChildChecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&);
    void OnChildUnchecked(const winrt::IInspectable& sender, const winrt::RoutedEventArgs&);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnAccessKeyInvoked(const winrt::UIElement&, const winrt::AccessKeyInvokedEventArgs& args);

    winrt::UIElement TryGetRepeaterElementFromRadioButton(const winrt::UIElement& element);
    bool TryGetRepeaterElementIndexFromRadioButton(const winrt::UIElement& element, const winrt::ItemsRepeater& repeater, int& index);
    winrt::ToggleButton TryGetRadioButtonFromRepeaterElement(const winrt::UIElement& element);
    winrt::ToggleButton TryGetRadioButtonFromRepeaterElementIndex(int index, const winrt::ItemsRepeater& repeater);
    bool IsRepeaterOwnedRadioButton(const winrt::DependencyObject& element, const winrt::ItemsRepeater& repeater);

    void UpdateItemsSource();
    winrt::IInspectable GetItemsSource();

    void UpdateSelectedIndex();
    void UpdateSelectedItem();

    void Select(int index);
    winrt::IInspectable GetDataAtIndex(int index, bool containerIsChecked);

    winrt::FindNextElementOptions GetFindNextElementOptions();
    bool MoveFocusNext();
    bool MoveFocusPrevious();
    bool MoveFocus(int initialIndexIncrement);
    bool HandleEdgeCaseFocus(bool first, const winrt::IInspectable& source);

    int m_selectedIndex{ -1 };
    // This is used to guard against reentrency when calling select, since select changes
    // the Selected Index/Item which in turn calls select.
    bool m_currentlySelecting{ false };
    // We block selection before the control has loaded.
    // This is to ensure that we do not overwrite a provided Selected Index/Item value.
    bool m_blockSelecting{ true };

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
