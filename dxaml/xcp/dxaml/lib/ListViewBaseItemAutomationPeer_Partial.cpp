// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseItemAutomationPeer.g.h"
#include "localizedResource.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

IFACEMETHODIMP ListViewBaseItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ListViewBaseItemAutomationPeer::ShouldSupportInvokePattern(
    _In_ ListViewBaseItem* listViewbaseItem,
    _Out_ bool* supportInvokePatternOut)
{
    *supportInvokePatternOut = false;

    ctl::ComPtr<ListViewBase> parentListViewBase;
    parentListViewBase = listViewbaseItem->GetParentListView();

    if (parentListViewBase)
    {
        BOOLEAN isItemClickEnabled = FALSE;
        IFC_RETURN(parentListViewBase->get_IsItemClickEnabled(&isItemClickEnabled));

        if (isItemClickEnabled)
        {
            *supportInvokePatternOut = true;
        }
        else
        {
            // Support invoke pattern if ListViewBaseItem is in a Semantic Zoom's zoomed-out view.
            // Invoke will switch to zoomed-in view.
            ctl::ComPtr<ISemanticZoom> semanticZoomOwner;
            IFC_RETURN(parentListViewBase->get_SemanticZoomOwner(&semanticZoomOwner));

            if (semanticZoomOwner)
            {
                BOOLEAN canChangeViews = FALSE;
                IFC_RETURN(semanticZoomOwner->get_CanChangeViews(&canChangeViews));

                if (canChangeViews)
                {
                    BOOLEAN isZoomedInView = FALSE;
                    IFC_RETURN(parentListViewBase->get_IsZoomedInView(&isZoomedInView));

                    if (!isZoomedInView)
                    {
                        *supportInvokePatternOut = true;
                    }
                }
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP ListViewBaseItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    BOOLEAN isDragEnabled = FALSE;
    ctl::ComPtr<ListViewBase> spListView;
    ctl::ComPtr<IUIElement> spOwner;

    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    IFC(get_Owner(&spOwner));

    if (patternInterface == xaml_automation_peers::PatternInterface_Drag)
    {
        spListView = spOwner.Cast<ListViewBaseItem>()->GetParentListView();
        if (spListView)
        {
            IFC(spListView->get_CanDragItems(&isDragEnabled));
            if (isDragEnabled)
            {
                ctl::ComPtr<IFrameworkElementAutomationPeer> spThis = this;
                IFC(spThis.CopyTo(ppReturnValue));
            }
        }
    }
    else if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        bool supportInvokePattern = false;
        IFC(ShouldSupportInvokePattern(spOwner.Cast<ListViewBaseItem>(), &supportInvokePattern));

        if (supportInvokePattern)
        {
            // Create adapter to perfom Invoke action, if needed
            if (!m_spInvokeAdapter)
            {
                ctl::ComPtr<ItemInvokeAdapter> spItemInvokeAdapter;

                IFC(ctl::make<ItemInvokeAdapter>(&spItemInvokeAdapter));
                IFCPTR(spItemInvokeAdapter.Get());

                m_spInvokeAdapter = spItemInvokeAdapter;
                IFC(m_spInvokeAdapter->put_Owner(this));
            }
            IFC(m_spInvokeAdapter.CopyTo(ppReturnValue));
        }
    }
    else
    {
        IFC(ListViewBaseItemAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

// IDragProvider -----------------------------------------------------------------------------------------------------------------

_Check_return_ HRESULT ListViewBaseItemAutomationPeer::get_IsGrabbedImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwnerUie;
    ctl::ComPtr<ListViewBaseItem> spOwner;

    IFC(get_Owner(&spOwnerUie));
    IFC(spOwnerUie ? S_OK : E_FAIL);
    IFC(ListViewBaseItem::QueryFor(spOwnerUie.Get(), &spOwner));
    IFC(spOwner->GetDragIsGrabbed(pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItemAutomationPeer::get_DropEffectImpl(_Out_ HSTRING* pValue)
{
    // In the future the effect text should be updated based on what is possible to do with this item.
    RRETURN(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_DRAG_MOVE, pValue));
}

_Check_return_ HRESULT ListViewBaseItemAutomationPeer::get_DropEffectsImpl(_Out_ UINT* returnValueCount, _Out_ HSTRING** returnValue)
{
    // In the future the list of effect should be updated based on what is possible to do with this item.
    HRESULT hr = S_OK;
    HSTRING* phsArray = static_cast<HSTRING*>(CoTaskMemAlloc(sizeof(HSTRING)));
    IFCOOMFAILFAST(phsArray);

    IFC(DXamlCore::GetCurrentNoCreate()->GetLocalizedResourceString(UIA_DRAG_MOVE, phsArray));

    *returnValueCount = 1;
    *returnValue = phsArray;
    phsArray = nullptr;

Cleanup:
    DELETE_ARRAY(phsArray);
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseItemAutomationPeer::GetGrabbedItemsImpl(_Out_ UINT* returnValueCount,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwnerUie;
    ctl::ComPtr<ListViewBaseItem> spOwner;
    ListViewBase* pListViewBase;

    *returnValueCount = 0;
    *returnValue = nullptr;

    IFC(get_Owner(&spOwnerUie));
    IFC(spOwnerUie ? S_OK : E_FAIL);
    IFC(ListViewBaseItem::QueryFor(spOwnerUie.Get(), &spOwner));
    pListViewBase = spOwner->GetParentListView();
    if (pListViewBase)
    {
        IFC(pListViewBase->GetDragGrabbedItems(returnValueCount, returnValue));
    }

Cleanup:
    RRETURN(hr);
}
