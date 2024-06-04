// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewBaseHeaderItem.g.h"
#include "Transition_Partial.h"

namespace DirectUI
{
    PARTIAL_CLASS(ListViewBaseHeaderItem),
        public ILayoutTransitionStoryboardNotification
    {
    public:
        ListViewBaseHeaderItem(): m_invalidateGroupBoundsCache(TRUE),
                                    m_IsPressed(false),
                                    m_IsPointerOver(false) {};

        IFACEMETHOD(ArrangeOverride)(
            _In_ wf::Size finalSize,
            _Out_ wf::Size* returnValue)
            override;

        _Check_return_ HRESULT SetParent(
            _In_opt_ xaml_controls::IListViewBase* pParent);

        _Check_return_ HRESULT GetParent(
            _Outptr_ xaml_controls::IListViewBase** ppParent);

        _Check_return_ HRESULT GetGroupBounds(
            _Out_ wf::Rect* pRect);

        _Check_return_ HRESULT GetPlainText(_Out_ HSTRING* strPlainText) override;

        _Check_return_ HRESULT GetLogicalParentForAPProtected(_Outptr_ DependencyObject** ppLogicalParentForAP) override;

        // Sticky Headers
        // Computes the offsets resulting from Sticky Headers
        _Check_return_ HRESULT CoerceStickyHeaderOffsets(
            _In_ INT cOffsets,
            _Inout_updates_(cOffsets) DOUBLE *pOffsets);

        // Supports the IStoryboardNotification interface.
        _Check_return_ HRESULT QueryInterfaceImpl(
            _In_ REFIID iid,
            _Outptr_ void** ppObject) override;

        _Check_return_ HRESULT NotifyLayoutTransitionStart() override;
        _Check_return_ HRESULT NotifyLayoutTransitionEnd() override;

    protected:
        _Check_return_ HRESULT ChangeVisualState(
            _In_ bool useTransitions) override;

        IFACEMETHOD(OnApplyTemplate)() override;

        IFACEMETHOD(OnGotFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnLostFocus)(
            _In_ xaml::IRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyDown)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        IFACEMETHOD(OnKeyUp)(
            _In_ xaml_input::IKeyRoutedEventArgs* pArgs) override;

        // Called when the user presses a pointer down over the ListViewBaseHeaderItem.
        IFACEMETHOD(OnPointerPressed)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when the user releases a pointer over the ListViewBaseHeaderItem.
        IFACEMETHOD(OnPointerReleased)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when the ListViewBaseHeaderItem or its children lose pointer capture.
        IFACEMETHOD(OnPointerCaptureLost)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when a pointer enters a ListViewBaseHeaderItem.
        IFACEMETHOD(OnPointerEntered)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        // Called when a pointer leaves a ListViewBaseHeaderItem.
        IFACEMETHOD(OnPointerExited)(
            _In_ xaml_input::IPointerRoutedEventArgs* pArgs) override;

        IFACEMETHOD(GetCurrentTransitionContext)(
            _In_ INT layoutTickId,
            _Out_ ThemeTransitionContext* pReturnValue)
            override;

        // determines if mutations are going fast
        IFACEMETHOD(IsCollectionMutatingFast)(
            _Out_ BOOLEAN* returnValue)
            override;

        IFACEMETHOD(GetDropOffsetToRoot)(
            _Out_ wf::Point* pReturnValue)
            override;

        IFACEMETHOD(OnTapped)(
            _In_ xaml_input::ITappedRoutedEventArgs* pArgs) override;

        // Called when the element leaves the tree. Clears the sticky header wrapper.
        _Check_return_ HRESULT LeaveImpl(
            _In_ bool bLive,
            _In_ bool bSkipNameRegistration,
            _In_ bool bCoercedIsEnabled,
            _In_ bool bVisualTreeBeingReset) override;

private:
        ctl::WeakRefPtr m_wrParentListViewBase;
        XRECTF_RB m_rectGroupBoundsCache{};
        BOOL m_invalidateGroupBoundsCache;

        bool m_IsPressed;
        bool m_IsPointerOver;
    };
}
