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
    void RevokeListeners();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // internal
    void SetEllipsisBreadcrumbItem(const winrt::BreadcrumbItem& ellipsisBreadcrumbItem);
    void SetIndex(const uint32_t index);
    void OnClickEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    
private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);
    void OnPointerEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerEnteredEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerPressedEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerReleasedEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);
    void OnPointerExitedEvent(const winrt::IInspectable& sender, const winrt::PointerRoutedEventArgs& args);

    void OnVisualPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void UpdateCommonVisualState();
    
    // BreadcrumbItem visual representation
    tracker_ref<winrt::ContentPresenter> m_dropDownItemContentPresenter{ this };

    // BreadcrumbItem that owns the flyout
    tracker_ref<winrt::BreadcrumbItem> m_ellipsisBreadcrumbItem{ this };
    
    winrt::ContentPresenter::Loaded_revoker m_dropDownItemContentPresenterLoadedRevoker{};

    bool m_isPressed{};
    bool m_isPointerOver{};
    uint32_t m_index{};

    winrt::UIElement::PointerEntered_revoker m_breadcrumbItemPointerEnteredRevoker{};
    winrt::UIElement::PointerExited_revoker m_breadcrumbItemPointerExitedRevoker{};
    winrt::UIElement::PointerPressed_revoker m_breadcrumbItemPointerPressedRevoker{};
    winrt::UIElement::PointerReleased_revoker m_breadcrumbItemPointerReleasedRevoker{};
    winrt::UIElement::PointerCanceled_revoker m_breadcrumbItemPointerCanceledRevoker{};
    winrt::UIElement::PointerCaptureLost_revoker m_breadcrumbItemPointerCaptureLostRevoker{};

    RoutedEventHandler_revoker m_dropDownItemKeyDownRevoker{};
};
