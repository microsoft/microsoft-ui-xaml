// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "BreadcrumbBar.g.h"
#include "BreadcrumbBar.properties.h"
#include "SplitButton.h"
#include "BreadcrumbBarElementFactory.h"
#include "Vector.h"
#include "BreadcrumbIterable.h"
#include "BreadcrumbLayout.h"

class BreadcrumbBar :
    public ReferenceTracker<BreadcrumbBar, winrt::implementation::BreadcrumbBarT>,
    public BreadcrumbBarProperties
{

public:
    BreadcrumbBar();
    ~BreadcrumbBar() {}

    // IFrameworkElement
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void RevokeListeners();

    void RaiseItemClickedEvent(const winrt::IInspectable& content, const uint32_t index);
    winrt::IVector<winrt::IInspectable> HiddenElements() const;
    void ReIndexVisibleElementsForAccessibility() const;

private:
    void OnBreadcrumbBarItemsRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnElementPreparedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs&);
    void OnElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs&);
    void OnElementClearingEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs&);
    void OnBreadcrumbBarItemsSourceCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void OnFlowDirectionChanged(winrt::DependencyObject const&, winrt::DependencyProperty const&);

    // Keyboard navigation
    void OnGettingFocus(const winrt::IInspectable&, const winrt::GettingFocusEventArgs& args);
    void OnAccessKeyInvoked(const winrt::UIElement&, const winrt::AccessKeyInvokedEventArgs& args);

    winrt::FindNextElementOptions GetFindNextElementOptions();
    void FocusElementAt(int index);
    bool MoveFocus(int initialIndexIncrement);
    bool MoveFocusPrevious();
    bool MoveFocusNext();
    bool HandleEdgeCaseFocus(bool first, const winrt::IInspectable& source);
    void OnChildPreviewKeyDown(const winrt::IInspectable& sender, const winrt::KeyRoutedEventArgs& args);

    void UpdateItemsRepeaterItemsSource();
    void UpdateItemTemplate();
    void UpdateEllipsisBreadcrumbBarItemDropDownItemTemplate();
    void UpdateBreadcrumbBarItemsFlowDirection();

    void ResetLastBreadcrumbBarItem();
    void ForceUpdateLastElement();
    void UpdateLastElement(const winrt::BreadcrumbBarItem& newLastBreadcrumbBarItem);

    winrt::IVector<winrt::IInspectable> GetHiddenElementsList(uint32_t firstShownElement) const;
    
    winrt::Control::Loaded_revoker m_itemsRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_itemsRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_itemsRepeaterElementIndexChangedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_itemsRepeaterElementClearingRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceChanged{};
    RoutedEventHandler_revoker m_breadcrumbKeyDownHandlerRevoker{};
    
    tracker_ref<winrt::INotifyCollectionChanged> m_notifyCollectionChanged{ this };
    winrt::event_token m_itemsSourceAsCollectionChanged{};
    winrt::event_token m_itemsSourceAsBindableVectorChanged{};
    winrt::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_itemsSourceAsObservableVectorChanged{};

    // This collection is only composed of the consumer defined objects, it doesn't
    // include the extra ellipsis/nullptr element. This variable is only used to capture
    // changes in the ItemsSource
    winrt::ItemsSourceView m_breadcrumbItemsSourceView{ nullptr };

    // This is the "element collection" provided to the underlying ItemsRepeater, so it
    // includes the extra ellipsis/nullptr element in the position 0.
    com_ptr<BreadcrumbIterable> m_itemsIterable{ nullptr };

    tracker_ref<winrt::ItemsRepeater> m_itemsRepeater { this };
    com_ptr<BreadcrumbElementFactory> m_itemsRepeaterElementFactory{ nullptr };
    com_ptr<BreadcrumbLayout> m_itemsRepeaterLayout{ nullptr };

    // Pointers to first and last items to update visual states
    tracker_ref<winrt::BreadcrumbBarItem> m_ellipsisBreadcrumbBarItem { this };
    tracker_ref<winrt::BreadcrumbBarItem> m_lastBreadcrumbBarItem { this };

    // Index of the last focused item when breadcrumb lost focus
    int m_focusedIndex{ 1 };

    // Template Parts
    static constexpr std::wstring_view s_itemsRepeaterPartName{ L"PART_ItemsRepeater"sv };
};
