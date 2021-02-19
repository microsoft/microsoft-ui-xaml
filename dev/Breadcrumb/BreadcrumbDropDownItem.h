// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "BreadcrumbDropDownItem.g.h"

class BreadcrumbDropDownItem :
    public ReferenceTracker<BreadcrumbDropDownItem, winrt::implementation::BreadcrumbDropDownItemT>
{
public:
    BreadcrumbDropDownItem();
    ~BreadcrumbDropDownItem();

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // internal
    void SetEllipsisBreadcrumbItem(const winrt::BreadcrumbItem& ellipsisBreadcrumbItem);
    void OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnPointerEntered(winrt::PointerRoutedEventArgs const& args);
    void OnPointerMoved(winrt::PointerRoutedEventArgs const& args);
    void OnPointerExited(winrt::PointerRoutedEventArgs const& args);
    void OnPointerPressed(winrt::PointerRoutedEventArgs const& args);
    void OnPointerReleased(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCanceled(winrt::PointerRoutedEventArgs const& args);
    void OnPointerCaptureLost(winrt::PointerRoutedEventArgs const& args);

private:
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnIsEnabledChanged(const winrt::IInspectable& sender, const winrt::DependencyPropertyChangedEventArgs& args);

    void HookListeners();
    void RevokeListeners();
    void ResetTrackedPointerId();
    bool IgnorePointerId(const winrt::PointerRoutedEventArgs& args);
    void ProcessPointerOver(const winrt::PointerRoutedEventArgs& args);
    void ProcessPointerCanceled(const winrt::PointerRoutedEventArgs& args);
    void UpdateCommonVisualState(bool useTransitions);

    // BreadcrumbItem that owns the flyout
    tracker_ref<winrt::BreadcrumbItem> m_ellipsisBreadcrumbItem{ this };
    
    RoutedEventHandler_revoker m_keyDownRevoker{};
    IsEnabledChanged_revoker m_isEnabledChangedRevoker{};

    // Visual State tracking
    uint32_t m_trackedPointerId{ 0 };
    bool m_isPressed{ false };
    bool m_isPointerOver{ false };

    // Visual States
    static constexpr std::wstring_view s_normalStateName{ L"Normal"sv };
    static constexpr std::wstring_view s_pointerOverStateName{ L"PointerOver"sv };
    static constexpr std::wstring_view s_pressedStateName{ L"Pressed"sv };
    static constexpr std::wstring_view s_disabledStateName{ L"Disabled"sv };
};
