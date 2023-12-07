// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemContainerTrace.h"
#include "ItemContainer.g.h"
#include "ItemContainer.properties.h"
#include "PointerInfo.h"

class ItemContainer :
    public ReferenceTracker<ItemContainer, winrt::implementation::ItemContainerT>,
    public ItemContainerProperties
{

public:
    ItemContainer();
    ~ItemContainer();

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnDoubleTapped(winrt::DoubleTappedRoutedEventArgs const& args);
    void OnKeyDown(winrt::KeyRoutedEventArgs const& args);
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerMoved(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnTapped(winrt::TappedRoutedEventArgs const& args);

    bool RaiseItemInvoked(const winrt::ItemContainerInteractionTrigger& interactionTrigger, const winrt::IInspectable& originalSource);

#ifndef MUX_PRERELEASE
    winrt::ItemContainerUserSelectMode CanUserSelectInternal() const
    {
        return m_canUserSelectInternal;
    }

    winrt::ItemContainerUserInvokeMode CanUserInvokeInternal() const
    {
        return m_canUserInvokeInternal;
    }

    winrt::ItemContainerMultiSelectMode MultiSelectModeInternal() const
    {
        return m_multiSelectModeInternal;
    }

    void CanUserSelectInternal(const winrt::ItemContainerUserSelectMode& value)
    {
        if (m_canUserSelectInternal != value)
        {
            m_canUserSelectInternal = value;
        }
    }

    void CanUserInvokeInternal(const winrt::ItemContainerUserInvokeMode& value)
    {
        if (m_canUserInvokeInternal != value)
        {
            m_canUserInvokeInternal = value;
        }
    }

    void MultiSelectModeInternal(const winrt::ItemContainerMultiSelectMode& value)
    {
        if (m_multiSelectModeInternal != value)
        {
            m_multiSelectModeInternal = value;

            // Same code as in ItemContainer::OnPropertyChanged, when MultiSelectMode changed.
            if (m_pointerInfo != nullptr)
            {
                UpdateVisualState(true);
                UpdateMultiSelectState(true);
            }
        }
    }
#endif

#ifdef DBG
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);
#endif

private:
    void OnIsEnabledChanged(winrt::IInspectable const& sender, winrt::DependencyPropertyChangedEventArgs const& args);

    void GoToState(std::wstring_view const& stateName, bool useTransitions);
    void UpdateVisualState(bool useTransitions);
    void UpdateMultiSelectState(bool useTransitions);

    void ProcessPointerOver(winrt::PointerRoutedEventArgs const& args, bool isPointerOver);
    void ProcessPointerCanceled(winrt::PointerRoutedEventArgs const& args);

    bool CanRaiseItemInvoked();
    
    void LoadSelectionCheckbox();
    void OnCheckToggle(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void UpdateCheckboxState();    
    void UpdateMousePointerOverInstance(bool isPointerOver);

#ifdef DBG
    static winrt::hstring DependencyPropertyToString(const winrt::IDependencyProperty& dependencyProperty);
#endif

    winrt::Control::IsEnabledChanged_revoker m_isEnabledChangedRevoker{};
    winrt::CheckBox::Checked_revoker m_checked_revoker{};
    winrt::CheckBox::Unchecked_revoker m_unchecked_revoker{};
    std::shared_ptr<PointerInfo<ItemContainer>> m_pointerInfo{ nullptr };

    tracker_ref<winrt::CheckBox> m_selectionCheckbox{ this };
    tracker_ref<winrt::Panel> m_rootPanel{ this };

#ifndef MUX_PRERELEASE
    winrt::ItemContainerUserSelectMode  m_canUserSelectInternal{ winrt::ItemContainerUserSelectMode::Auto };
    winrt::ItemContainerUserInvokeMode  m_canUserInvokeInternal{ winrt::ItemContainerUserInvokeMode::Auto };
    winrt::ItemContainerMultiSelectMode m_multiSelectModeInternal{ winrt::ItemContainerMultiSelectMode::Auto };
#endif

    static constexpr std::wstring_view s_disabledStateName{ L"Disabled"sv };
    static constexpr std::wstring_view s_enabledStateName{ L"Enabled"sv };
    static constexpr std::wstring_view s_selectedPressedStateName{ L"SelectedPressed"sv };
    static constexpr std::wstring_view s_unselectedPressedStateName{ L"UnselectedPressed"sv };
    static constexpr std::wstring_view s_selectedPointerOverStateName{ L"SelectedPointerOver"sv };
    static constexpr std::wstring_view s_unselectedPointerOverStateName{ L"UnselectedPointerOver"sv };
    static constexpr std::wstring_view s_selectedNormalStateName{ L"SelectedNormal"sv };
    static constexpr std::wstring_view s_unselectedNormalStateName{ L"UnselectedNormal"sv };
    static constexpr std::wstring_view s_multipleStateName{ L"Multiple"sv };
    static constexpr std::wstring_view s_singleStateName{ L"Single"sv };
    static constexpr std::wstring_view s_selectionCheckboxName{ L"PART_SelectionCheckbox" };
    static constexpr std::wstring_view s_containerRootName{ L"PART_ContainerRoot" };
};
