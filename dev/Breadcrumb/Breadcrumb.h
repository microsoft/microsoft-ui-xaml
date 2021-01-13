// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "Breadcrumb.g.h"
#include "Breadcrumb.properties.h"

#include "SplitButton.h"
#include "BreadcrumbElementFactory.h"

#include "Vector.h"
#include "BreadcrumbIterable.h"

class Breadcrumb :
    public ReferenceTracker<Breadcrumb, winrt::implementation::BreadcrumbT>,
    public BreadcrumbProperties
{

public:
    Breadcrumb();
    ~Breadcrumb() {}

    // IFrameworkElement
    void OnApplyTemplate();
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void RevokeListeners();

    void RaiseItemClickedEvent(const winrt::IInspectable& content);
    winrt::IVector<winrt::IInspectable> HiddenElements() const;

private:
    void OnBreadcrumbItemRepeaterLoaded(const winrt::IInspectable&, const winrt::RoutedEventArgs&);
    void OnElementPreparedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementPreparedEventArgs&);
    void OnElementIndexChangedEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementIndexChangedEventArgs&);
    void OnElementClearingEvent(const winrt::ItemsRepeater&, const winrt::ItemsRepeaterElementClearingEventArgs&);
    void OnRepeaterCollectionChanged(const winrt::IInspectable&, const winrt::IInspectable&);

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
    void UpdateEllipsisBreadcrumbItemDropdownItemTemplate();

    void ResetLastBreadcrumbItem();
    void ForceUpdateLastElement();
    void UpdateLastElement(const winrt::BreadcrumbItem& newLastBreadcrumbItem);

    winrt::IVector<winrt::IInspectable> GetHiddenElementsList(uint32_t firstShownElement) const;
    
    winrt::Control::Loaded_revoker m_itemsRepeaterLoadedRevoker{};
    winrt::ItemsRepeater::ElementPrepared_revoker m_itemRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementIndexChanged_revoker m_itemRepeaterElementIndexChangedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_itemRepeaterElementClearingRevoker{};
    winrt::ItemsSourceView::CollectionChanged_revoker m_itemsSourceChanged{};
    
    tracker_ref<winrt::INotifyCollectionChanged> m_notifyCollectionChanged{ this };
    winrt::event_token m_itemsSourceAsCollectionChanged{};
    winrt::event_token m_itemsSourceAsBindableVectorChanged{};
    winrt::IObservableVector<winrt::IInspectable>::VectorChanged_revoker m_itemsSourceAsObservableVectorChanged{};

    // This collection is only composed of the consumer defined objects, it doesn't
    // include the extra ellipsis/nullptr element. This variable is only used to capture
    // changes in the ItemsSource
    winrt::ItemsSourceView m_itemsSourceView{ nullptr };

    // This is the "element collection" provided to the underlying ItemsRepeater, so it
    // includes the extra ellipsis/nullptr element in the position 0.
    com_ptr<BreadcrumbIterable> m_itemsIterable{ nullptr };

    tracker_ref<winrt::ItemsRepeater> m_itemsRepeater { this };
    com_ptr<BreadcrumbElementFactory> m_itemsRepeaterElementFactory{ nullptr };

    // Pointers to first and last items to update visual states
    tracker_ref<winrt::BreadcrumbItem> m_ellipsisBreadcrumbItem { this };
    tracker_ref<winrt::BreadcrumbItem> m_lastBreadcrumbItem { this };

    // Index of the last focused item when breadcrumb lost focus
    int m_focusedIndex{ 1 };
};
