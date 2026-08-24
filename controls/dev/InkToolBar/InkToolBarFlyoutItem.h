// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarFlyoutItem.g.h"
#include "InkToolbarFlyoutItem.properties.h"

class InkToolbarFlyoutItem :
    public ReferenceTracker<InkToolbarFlyoutItem, winrt::implementation::InkToolbarFlyoutItemT>,
    public InkToolbarFlyoutItemProperties
{
public:
    enum class RelativeItem
    {
        Next,
        Previous,
        RadioGroupNext,
        RadioGroupPrevious
    };

    InkToolbarFlyoutItem() { SetDefaultStyleKey(this); }
    ~InkToolbarFlyoutItem();

    // IFrameworkElementOverrides / IUIElementOverrides
    void OnApplyTemplate();
    winrt::AutomationPeer OnCreateAutomationPeer();
    // DependencyObject property-changed hook (IsChecked drives radio-group + Checked/Unchecked events).
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // IInkToolbarFlyoutItemInternal (plain impl methods in the lift).
    void ConfigureItemEvents(winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase const& parentButton);
    void SetRadioGroupName(winrt::hstring const& radioGroupName);
    winrt::hstring TryGetTextContent();

    void UpdateStates(bool useTransitions);
    void UpdateVisualStatesForAllItems();
    winrt::InkToolbarFlyoutItem GetFirstVisibleItemInGroup();
    bool IsAnySelectedInRadioGroup();

    winrt::InkToolbarFlyoutItem GetRelativeItem(
        RelativeItem relative,
        winrt::InkToolbarFlyoutItem const& anchor,
        bool shouldWrapAround);

    // Called by InkToolbarFlyoutItemAutomationPeer (default operations for narrator/touch).
    void OnInvoked();

    // Cross-item access (UWP read m_radioGroupName via static_cast; lift uses get_self + this getter).
    winrt::hstring RadioGroupName() const { return m_radioGroupName; }

private:
    // Event handlers.
    void OnItemClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnItemPointerEntered(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnItemPointerExited(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnItemPointerPressed(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnItemPointerReleased(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnItemKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnItemKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);

    void OnIsCheckedChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    void ItemAction();
    bool ToggleCheckedState();
    void GoToState(wchar_t const* stateString, bool useTransitions);

    void ForAllInFlyout(std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function);
    void ForAllInGroup(winrt::InkToolbarFlyoutItem const& anchor, std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function);
    void ForAllOthersInGroup(winrt::InkToolbarFlyoutItem const& anchor, std::function<bool(winrt::InkToolbarFlyoutItem const& item)> function);

    winrt::hstring m_radioGroupName;
    winrt::weak_ref<winrt::Microsoft::UI::Xaml::Controls::Primitives::ButtonBase> m_parentButton;

    winrt::event_token m_clickRegistrationToken{};

    winrt::IInspectable m_itemPointerEnteredEventHandler{ nullptr };
    winrt::IInspectable m_itemPointerExitedEventHandler{ nullptr };
    winrt::IInspectable m_itemPointerPressedEventHandler{ nullptr };
    winrt::IInspectable m_itemPointerReleasedEventHandler{ nullptr };
    winrt::IInspectable m_itemKeyDownEventHandler{ nullptr };
    winrt::IInspectable m_itemKeyUpEventHandler{ nullptr };
};

