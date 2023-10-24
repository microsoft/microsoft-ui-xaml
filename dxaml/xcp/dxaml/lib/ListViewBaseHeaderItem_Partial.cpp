// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseHeaderItem.g.h"
#include "ListViewBaseHeaderItemAutomationPeer.g.h"
#include "ListViewBase.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "VisualTreeHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListViewBaseHeaderItem::SetParent(
    _In_ xaml_controls::IListViewBase* pParent)
{
    HRESULT hr = S_OK;

    IFC(ctl::AsWeak(ctl::iinspectable_cast(pParent), &m_wrParentListViewBase));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseHeaderItem::GetParent(
    _Outptr_ xaml_controls::IListViewBase** ppParent)
{
    HRESULT hr = S_OK;
    IFCPTR(ppParent);
    *ppParent = nullptr;

    IFC(m_wrParentListViewBase.CopyTo(ppParent));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItem::OnApplyTemplate()
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseHeaderItemGenerated::OnApplyTemplate());
    IFC(ChangeVisualState(false));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItem::OnGotFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnGotFocus(pArgs));
    IFC(ChangeVisualState(false));

    IFC(m_wrParentListViewBase.As(&spParent));

    if (spParent)
    {
        IFC(spParent.Cast<ListViewBase>()->GroupHeaderItemFocused(this));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItem::OnLostFocus(
    _In_ xaml::IRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnLostFocus(pArgs));
    IFC(ChangeVisualState(true));

    IFC(m_wrParentListViewBase.As(&spParent));

    if (spParent)
    {
        IFC(spParent.Cast<ListViewBase>()->GroupHeaderItemUnfocused(this));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItem::OnKeyDown(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnKeyDown(pArgs));

    IFC(m_wrParentListViewBase.As(&spParent));

    if (spParent)
    {
        wsy::VirtualKey originalKey = wsy::VirtualKey_None;
        wsy::VirtualKey key = wsy::VirtualKey_None;
        BOOLEAN isHandled = FALSE;

        IFC(pArgs->get_OriginalKey(&originalKey));
        IFC(pArgs->get_Key(&key));

        // for gamepad, this action happens on key up. Toggling sezo on KeyDown will cause a key up 
        // in the other view and toggle it back.
        if (originalKey != wsy::VirtualKey_GamepadA)
        {
            IFC(spParent.Cast<ListViewBase>()->OnGroupHeaderKeyDown(ctl::as_iinspectable(this), originalKey, key, &isHandled));
        }

        // Inform ListView that the keydown came from an Item. If it came from a header, or blank area
        // instead of an item ListView will just forward the event back to the ScrollView.
        if (!isHandled)
        {
            spParent.Cast<ListViewBase>()->SetHandleKeyDownArgsFromItem(true /* fFromItem */);
        }

        if (isHandled)
        {
            IFC(pArgs->put_Handled(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Handle Key Up for gamepad and toggle
IFACEMETHODIMP ListViewBaseHeaderItem::OnKeyUp(
    _In_ xaml_input::IKeyRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnKeyUp(pArgs));

    IFC(m_wrParentListViewBase.As(&spParent));

    if (spParent)
    {
        wsy::VirtualKey originalKey = wsy::VirtualKey_None;
        wsy::VirtualKey key = wsy::VirtualKey_None;
        BOOLEAN isHandled = FALSE;

        IFC(pArgs->get_OriginalKey(&originalKey));
        IFC(pArgs->get_Key(&key));

        if (originalKey == wsy::VirtualKey_GamepadA)
        {
            IFC(spParent.Cast<ListViewBase>()->OnGroupHeaderKeyUp(ctl::as_iinspectable(this), originalKey, key, &isHandled));
        }

        if (isHandled)
        {
            IFC(pArgs->put_Handled(TRUE));
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the user presses a pointer down over the ListViewBaseHeaderItem.
IFACEMETHODIMP ListViewBaseHeaderItem::OnPointerPressed(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseHeaderItemGenerated::OnPointerPressed(pArgs));

    if (!m_IsPressed)
    {
        m_IsPressed = true;

        IFC(ChangeVisualState(true /* useTransitions */));
    }

Cleanup:
    RRETURN(hr);
}

// Called when the user releases a pointer over the ListViewBaseHeaderItem.
IFACEMETHODIMP ListViewBaseHeaderItem::OnPointerReleased(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnPointerPressed(pArgs));

    m_IsPressed = false;

    IFC(ChangeVisualState(true /* useTransitions */));

    // if we are in a semantic zoom, handle the pointer released key
    // so that the ScrollViewer above does not handle it and move focus
    // to itself.
    IFC(m_wrParentListViewBase.As(&spParent));
    if (spParent)
    {
        ctl::ComPtr<ISemanticZoom> semanticZoomOwner;
        IFC(spParent->get_SemanticZoomOwner(&semanticZoomOwner));
        if (semanticZoomOwner)
        {
            pArgs->put_Handled(TRUE);
        }
    }

Cleanup:
    RRETURN(hr);
}

// Called when the ListViewBaseHeaderItem or its children lose pointer capture.
IFACEMETHODIMP ListViewBaseHeaderItem::OnPointerCaptureLost(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(__super::OnPointerCaptureLost(pArgs));

    m_IsPressed = false;
    m_IsPointerOver = false;

    IFC(ChangeVisualState(true /* useTransitions */));

Cleanup:
    RRETURN(hr);
}

// Called when a pointer enters a ListViewBaseHeaderItem.
IFACEMETHODIMP ListViewBaseHeaderItem::OnPointerEntered(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseHeaderItemGenerated::OnPointerEntered(pArgs));

    m_IsPointerOver = true;

    IFC(ChangeVisualState(true /* useTransitions */));

Cleanup:
    RRETURN(hr);
}

// Called when a pointer exits a ListViewBaseHeaderItem.
IFACEMETHODIMP ListViewBaseHeaderItem::OnPointerExited(
    _In_ IPointerRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseHeaderItemGenerated::OnPointerExited(pArgs));

    m_IsPressed = false;
    m_IsPointerOver = false;

    IFC(ChangeVisualState(true /* useTransitions */));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseHeaderItem::ChangeVisualState(
    _In_ bool useTransitions)
{
    HRESULT hr = S_OK;
    BOOLEAN isEnabled = FALSE;
    BOOLEAN ignored = FALSE;
    xaml::FocusState focusState = xaml::FocusState_Unfocused;

    IFC(get_IsEnabled(&isEnabled));
    IFC(get_FocusState(&focusState));

    // Common state group
    if (!isEnabled)
    {
        IFC(GoToState(useTransitions, L"Disabled", &ignored));
    }
    else if (m_IsPressed)
    {
        IFC(GoToState(useTransitions, L"Pressed", &ignored));
    }
    else if (m_IsPointerOver)
    {
        IFC(GoToState(useTransitions, L"PointerOver", &ignored));
    }
    else
    {
        IFC(GoToState(useTransitions, L"Normal", &ignored));
    }

    // Focus state group
    if (focusState == xaml::FocusState_Keyboard ||
        focusState == xaml::FocusState_Programmatic)
    {
        IFC(GoToState(useTransitions, L"Focused", &ignored));
    }
    else if (focusState == xaml::FocusState_Pointer)
    {
        IFC(GoToState(useTransitions, L"PointerFocused", &ignored));
    }
    else
    {
        IFC(GoToState(useTransitions, L"Unfocused", &ignored));
    }

Cleanup:
    RRETURN(hr);
}

// determines if this element should be transitioned using the passed in transition
IFACEMETHODIMP ListViewBaseHeaderItem::GetCurrentTransitionContext(
    _In_ INT layoutTickId,
    _Out_ ThemeTransitionContext* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spParentAsDO;
    ctl::ComPtr<IChildTransitionContextProvider> spContextProviderPanel;

    IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    if (spParentAsDO)
    {
        IFC(spParentAsDO.As<IChildTransitionContextProvider>(&spContextProviderPanel));
        IFC(spContextProviderPanel->GetChildTransitionContext(this, layoutTickId, pReturnValue));
    }
    else
    {
        *pReturnValue = ThemeTransitionContext::None;
    }

Cleanup:
    RRETURN(hr);
}

// determines if mutations are going fast
IFACEMETHODIMP ListViewBaseHeaderItem::IsCollectionMutatingFast(
    _Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spParentAsDO;

    *pReturnValue = FALSE;

    IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
    if (spParentAsDO)
    {        
        auto spContextProviderPanel = spParentAsDO.AsOrNull<IChildTransitionContextProvider>();
        if (spContextProviderPanel)
        {
            IFC(spContextProviderPanel->IsCollectionMutatingFast(pReturnValue));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseHeaderItem::GetDropOffsetToRoot(
    _Out_ wf::Point* pReturnValue)
{
    pReturnValue->X = 0;
    pReturnValue->Y = 0;
    RRETURN(E_NOTIMPL);
}

_Check_return_ HRESULT ListViewBaseHeaderItem::OnTapped(
    _In_ ITappedRoutedEventArgs* pArgs)
{
    HRESULT hr = S_OK;
    BOOLEAN isHandled = FALSE;
    ctl::ComPtr<IListViewBase> spParent;

    IFC(ListViewBaseHeaderItemGenerated::OnTapped(pArgs));

    IFC(pArgs->get_Handled(&isHandled));
    if (!isHandled)
    {
        IFC(m_wrParentListViewBase.As(&spParent));
        if(spParent)
        {
            IFC(spParent.Cast<ListViewBase>()->OnHeaderItemTap(ctl::as_iinspectable(this), &isHandled));
            IFC(pArgs->put_Handled(isHandled));
        }
    }
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItem::ArrangeOverride(
    // The computed size that is used to arrange the content.
    _In_ wf::Size arrangeSize,
    // The size of the control.
    _Out_ wf::Size* returnValue)
{
    HRESULT hr = S_OK;

    IFC(ListViewBaseHeaderItemGenerated::ArrangeOverride(arrangeSize, returnValue));
    m_invalidateGroupBoundsCache = TRUE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseHeaderItem::GetGroupBounds(
    _Out_ wf::Rect* pRect)
{
    HRESULT hr = S_OK;

    if (m_invalidateGroupBoundsCache)
    {
        ctl::ComPtr<xaml_controls::IListViewBase> spParentListViewBase;
        ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
        ctl::ComPtr<IModernCollectionBasePanel> spItemsHostPanelModernCollection;
        ctl::ComPtr<IDependencyObject> spItemContainerAsDO;
        INT indexHeader = 0;
        INT32 indexCacheStart = 0;
        INT32 indexCacheEnd = 0;
        INT32 indexStartItem = 0;
        INT32 countItemInGroup = 0;

        m_rectGroupBoundsCache.left = 0;
        m_rectGroupBoundsCache.right = 0;
        m_rectGroupBoundsCache.top = 0;
        m_rectGroupBoundsCache.bottom = 0;

        IFC(static_cast<CUIElement*>(GetHandle())->GetGlobalBounds(&m_rectGroupBoundsCache));

        IFC(m_wrParentListViewBase.As(&spParentListViewBase));

        // If the header is recycled, the parent ListViewBase is set 
        // to null. We could potentially end up in this method when 
        // the header is recycled since focus events are async (bug 676039)
        if (spParentListViewBase.Get())
        {
            IFC(spParentListViewBase.Cast<ListViewBase>()->get_ItemsHost(&spItemsHostPanel));
            IFC(spItemsHostPanel.As(&spItemsHostPanelModernCollection));
            IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->IndexFromHeader(this, FALSE /*excludeHiddenEmptyGroups*/, &indexHeader));
            IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromGroupIndex(indexHeader, &indexStartItem, &countItemInGroup));
            IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&indexCacheEnd));

            INT32 indexRealizedStart = (indexCacheStart > indexStartItem) ? indexCacheStart : indexStartItem;
            INT32 indexRealizedEnd = (indexCacheEnd < (indexStartItem + countItemInGroup - 1)) ? indexCacheEnd : (indexStartItem + countItemInGroup - 1);
            for (int indexContainer = indexRealizedStart; indexContainer <= indexRealizedEnd; ++indexContainer)
            {
                IFC(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->ContainerFromIndex(indexContainer, &spItemContainerAsDO));
                if (spItemContainerAsDO)
                {
                    XRECTF_RB elementBounds;
                    IFC(static_cast<CUIElement*>(spItemContainerAsDO.Cast<DependencyObject>()->GetHandle())->GetGlobalBounds(&elementBounds));
                    UnionRectF(&m_rectGroupBoundsCache, &elementBounds);
                }
            }
        }
    }

    pRect->X = m_rectGroupBoundsCache.left;
    pRect->Y = m_rectGroupBoundsCache.top;
    pRect->Width = m_rectGroupBoundsCache.right - m_rectGroupBoundsCache.left;
    pRect->Height = m_rectGroupBoundsCache.bottom - m_rectGroupBoundsCache.top;

Cleanup:
    RRETURN(hr);
}

_Check_return_ 
HRESULT 
ListViewBaseHeaderItem::GetPlainText(_Out_ HSTRING* strPlainText)
{
    RRETURN(ContentControl::GetPlainText(strPlainText));
}

_Check_return_ HRESULT ListViewBaseHeaderItem::GetLogicalParentForAPProtected(_Outptr_ DependencyObject** ppLogicalParentForAP)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IListViewBase> spListView;
    ctl::ComPtr<IDependencyObject> spListViewAsDO;

    IFCPTR(ppLogicalParentForAP);
    *ppLogicalParentForAP = nullptr;

    IFC(GetParent(&spListView));
    if (spListView)
    {
        IFC(spListView.As(&spListViewAsDO));
       *ppLogicalParentForAP = static_cast<DependencyObject*>(spListViewAsDO.Detach());
    }

Cleanup:
    RRETURN(hr);
}

// Sticky Headers
// Computes the offsets resulting from Sticky Headers
// We delegate the computations to the panel
_Check_return_ HRESULT ListViewBaseHeaderItem::CoerceStickyHeaderOffsets(
    _In_ INT cOffsets,
    _Inout_updates_(cOffsets) DOUBLE *pOffsets)
{
    HRESULT hr = S_OK;
    UIElement::VirtualizationInformation *pVirtualizationInformation = GetVirtualizationInformation();

    if (pVirtualizationInformation && pVirtualizationInformation->GetIsSticky())
    {
        ctl::ComPtr<IDependencyObject> spParentAsDO;
        IFC(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
        if (spParentAsDO)
        {
            ctl::ComPtr<IModernCollectionBasePanel> spParentPanel = spParentAsDO.AsOrNull<IModernCollectionBasePanel>();
            if (spParentPanel)
            {
                IFC(spParentPanel.Cast<ModernCollectionBasePanel>()->CoerceStickyHeaderOffsets(this, cOffsets, pOffsets));
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

// Supports the ILayoutTransitionStoryboardNotification interface.
_Check_return_ 
    HRESULT 
    ListViewBaseHeaderItem::QueryInterfaceImpl(
    _In_ REFIID iid,
    _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(ILayoutTransitionStoryboardNotification)))
    {
        *ppObject = static_cast<ILayoutTransitionStoryboardNotification*>(this);
    }
    else
    {
        RRETURN(ListViewBaseHeaderItemGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Callback when a LayoutTransition storyboard is created for this element
// Gives us a chance to call back into the panel and allow it to take action, e.g., temporarily hide Sticky Headers
_Check_return_ HRESULT ListViewBaseHeaderItem::NotifyLayoutTransitionStart()
{
    if (GetVirtualizationInformation() && GetVirtualizationInformation()->GetIsSticky())
    {
        ctl::ComPtr<IDependencyObject> spParentAsDO;
        IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
        if (spParentAsDO)
        {
            ctl::ComPtr<IModernCollectionBasePanel> spParentPanel = spParentAsDO.AsOrNull<IModernCollectionBasePanel>();
            if (spParentPanel)
            {
                IFC_RETURN(spParentPanel.Cast<ModernCollectionBasePanel>()->NotifyLayoutTransitionStart(this));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseHeaderItem::NotifyLayoutTransitionEnd()
{
    if (GetVirtualizationInformation() && GetVirtualizationInformation()->GetIsSticky())
    {
        ctl::ComPtr<IDependencyObject> spParentAsDO;
        IFC_RETURN(VisualTreeHelper::GetParentStatic(this, &spParentAsDO));
        if (spParentAsDO)
        {
            ctl::ComPtr<IModernCollectionBasePanel> spParentPanel = spParentAsDO.AsOrNull<IModernCollectionBasePanel>();
            if (spParentPanel)
            {
                IFC_RETURN(spParentPanel.Cast<ModernCollectionBasePanel>()->NotifyLayoutTransitionEnd(this));
            }
        }
    }

    return S_OK;
}


// Called when the element leaves the tree. Clears the sticky header wrapper.
_Check_return_ HRESULT ListViewBaseHeaderItem::LeaveImpl(
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    HRESULT hr = S_OK;

    if (GetVirtualizationInformation())
    {
        GetVirtualizationInformation()->m_stickyHeaderWrapper = nullptr;
    }

    IFC(ListViewBaseHeaderItemGenerated::LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

Cleanup:
    RRETURN(hr);
}
