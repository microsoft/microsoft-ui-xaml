// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SelectorAutomationPeer.g.h"
#include "Selector.g.h"
#include "SelectionChangedEventArgs.g.h"
#include "ItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SelectorAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_primitives::ISelector* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISelectorAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISelectorAutomationPeer* pInstance = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;
    IInspectable* pInner = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<Selector*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<SelectorAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the SelectorAutomationPeer class.
SelectorAutomationPeer::SelectorAutomationPeer()
{
}

// Deconstructor
SelectorAutomationPeer::~SelectorAutomationPeer()
{
}

IFACEMETHODIMP SelectorAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    if (patternInterface == xaml_automation_peers::PatternInterface_Selection)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        bool isSelectionPatternApplicable = false;

        IFC_RETURN(get_Owner(&owner));
        IFCPTR_RETURN(owner);
        IFC_RETURN(static_cast<Selector*>(owner.Get())->IsSelectionPatternApplicable(&isSelectionPatternApplicable));
        if (isSelectionPatternApplicable)
        {
            *ppReturnValue = ctl::as_iinspectable(this);
            ctl::addref_interface(this);
        }
    }
    else
    {
        IFC_RETURN(SelectorAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

    return S_OK;
}

IFACEMETHODIMP SelectorAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_List;
    RRETURN(S_OK);
}

//-------------------------------------------------------------------
//  ISelectionProvider
//-------------------------------------------------------------------
_Check_return_ HRESULT SelectorAutomationPeer::GetSelectionImpl(_Out_ UINT* returnValueCount, _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple*** returnValue)
{
    HRESULT hr = S_OK;
    UINT count = 0;
    xaml::IUIElement* pOwner = NULL;
    IInspectable* item = NULL;
    xaml_automation_peers::ISelectorItemAutomationPeer* itemPeer = NULL;
    xaml_automation_peers::IAutomationPeer* itemPeerAsAP = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pProvider = NULL;
    wfc::IVector<IInspectable*>* pSelectedItems = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<Selector*>(pOwner))->get_SelectedItemsInternal(&pSelectedItems));
    if (pSelectedItems)
    {
        IFC(pSelectedItems->get_Size(&count));
        if (count > 0)
        {
            UINT allocSize = sizeof(IIRawElementProviderSimple*) * count;
            *returnValue = static_cast<IIRawElementProviderSimple**>(CoTaskMemAlloc(allocSize));
            IFCOOMFAILFAST(*returnValue);
            ZeroMemory(*returnValue, allocSize);
            for (UINT i = 0; i < count; i++)
            {
                IFC(pSelectedItems->GetAt(i, &item));
                if (item)
                {
                    IFC(GetPeerForSelectedItem(item, &itemPeer));
                    IFC(ctl::do_query_interface(itemPeerAsAP, itemPeer));
                    IFC(ProviderFromPeer(itemPeerAsAP, &pProvider));
                    (*returnValue)[i] = pProvider;

                    pProvider = NULL;
                    ReleaseInterface(item);
                    ReleaseInterface(itemPeer);
                    ReleaseInterface(itemPeerAsAP);
                    ReleaseInterface(pProvider);
                }
            }
        }
    }

    *returnValueCount = count;

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pProvider);
    ReleaseInterface(pSelectedItems);
    ReleaseInterface(item);
    ReleaseInterface(itemPeer);
    ReleaseInterface(itemPeerAsAP);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorAutomationPeer::get_CanSelectMultipleImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    BOOLEAN canSelectMultiple;
    xaml::IUIElement* pOwner = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<Selector*>(pOwner))->get_CanSelectMultiple(&canSelectMultiple));

    *pValue = canSelectMultiple;

Cleanup:
    ReleaseInterface(pOwner);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorAutomationPeer::get_IsSelectionRequiredImpl(_Out_ BOOLEAN* pValue)
{
    *pValue = FALSE;
    RRETURN(S_OK);
}

// Helper function to get the automation peer for the selected item.
_Check_return_ HRESULT SelectorAutomationPeer::GetPeerForSelectedItem(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::ISelectorItemAutomationPeer** ppPeer)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pPeer = NULL;
    xaml_automation_peers::ISelectorItemAutomationPeer* pPeerAsSI = NULL;

    IFC(CreateItemAutomationPeer(item, &pPeer));
    IFC(ctl::do_query_interface(pPeerAsSI,pPeer));
    *ppPeer = pPeerAsSI;
    pPeerAsSI = NULL;

Cleanup:
    ReleaseInterface(pPeerAsSI);
    ReleaseInterface(pPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorAutomationPeer::RaiseSelectionEvents(_In_ xaml_controls::ISelectionChangedEventArgs* pSelectionChangedEventArgs) noexcept
{
    HRESULT hr = S_OK;
    UINT numSelected = 0;
    UINT numAdded = 0;
    UINT numRemoved = 0;

    xaml::IUIElement* pOwner = NULL;
    wfc::IVector<IInspectable*>* pSelectedItemsImpl = NULL;
    wfc::IVector<IInspectable*>* pAddedItems = NULL;
    wfc::IVector<IInspectable*>* pRemoveItems = NULL;
    xaml_automation_peers::IItemAutomationPeer* pPeer = NULL;
    IInspectable* pSelectedItem = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);
    IFC((static_cast<Selector*>(pOwner))->get_SelectedItemsInternal(&pSelectedItemsImpl));
    if (pSelectedItemsImpl)
    {
        IFC(pSelectedItemsImpl->get_Size(&numSelected));
    }
    IFC(static_cast<SelectionChangedEventArgs*>(pSelectionChangedEventArgs)->get_AddedItems(&pAddedItems));
    IFC(static_cast<SelectionChangedEventArgs*>(pSelectionChangedEventArgs)->get_RemovedItems(&pRemoveItems));
    if (pAddedItems)
    {
        IFC(pAddedItems->get_Size(&numAdded));
    }
    if (pRemoveItems)
    {
        IFC(pRemoveItems->get_Size(&numRemoved));
    }
    // if we add to selection just 1 item and number of selected items is 1
    // then we want to send ElementSelected event.
    // Scenarios:
    // 1) Single mode. Always send ElementSelected event when we select or reselect the item
    // 2) Multiple mode. Only when we add to selection 1 item and we have 1 selected item afterword.
    //    In case of we got SelectionEvents and we have 1 selected item but numAdded is 0 or more
    //    then 1 we should go through ElementAddedToSelection/ElementRemovedFromSelection events sending
    if(numAdded == 1 && numSelected ==1)
    {
        IFC(pSelectedItemsImpl->GetAt(0, &pSelectedItem));
        if (pSelectedItem)
        {
            IFC(CreateItemAutomationPeer(pSelectedItem, &pPeer));
            if (pPeer != nullptr)
            {
                IFC(static_cast<ItemAutomationPeer*>(pPeer)->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementSelected));
            }
            ReleaseInterface(pSelectedItem);
            ReleaseInterface(pPeer);
            // Aligining with Windows behavior wherein even for single selection they fire remove event for last removed event. It's different behavior than WPF/SL.
            if (numRemoved == 1)
            {
                IFC(pRemoveItems->GetAt(0, &pSelectedItem));
                if (pSelectedItem)
                {
                    IFC(CreateItemAutomationPeer(pSelectedItem, &pPeer));
                    if (pPeer != nullptr)
                    {
                        IFC(static_cast<ItemAutomationPeer*>(pPeer)->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementRemovedFromSelection));
                    }
                }
            }
        }
    }
    else
    {
        if(numAdded + numRemoved > BulkChildrenLimit)
        {
            IFC(RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionPatternOnInvalidated));
        }
        else
        {
            UINT i;
            for(i = 0; i < numAdded; i++)
            {
                IFC(pAddedItems->GetAt(i, &pSelectedItem));
                if (pSelectedItem)
                {
                    IFC(CreateItemAutomationPeer(pSelectedItem, &pPeer));
                    if (pPeer != NULL)
                    {
                        IFC(static_cast<ItemAutomationPeer*>(pPeer)->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementAddedToSelection));
                    }

                    ReleaseInterface(pSelectedItem);
                    ReleaseInterface(pPeer);
                }
            }

            for(i = 0; i < numRemoved; i++)
            {
                IFC(pRemoveItems->GetAt(i, &pSelectedItem));
                if (pSelectedItem)
                {
                    IFC(CreateItemAutomationPeer(pSelectedItem, &pPeer));
                    if (pPeer != NULL)
                    {
                        IFC(static_cast<ItemAutomationPeer*>(pPeer)->RaiseAutomationEvent(xaml_automation_peers::AutomationEvents_SelectionItemPatternOnElementRemovedFromSelection));
                    }

                    ReleaseInterface(pSelectedItem);
                    ReleaseInterface(pPeer);
                }
            }
        }
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pSelectedItem);
    ReleaseInterface(pPeer);
    ReleaseInterface(pSelectedItemsImpl);
    ReleaseInterface(pAddedItems);
    ReleaseInterface(pRemoveItems);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorAutomationPeer::GetSelectedItemAutomationPeer(_Outptr_opt_ xaml_automation_peers::IItemAutomationPeer** pAP)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    wfc::IVector<IInspectable*>* pSelectedItemsImpl = NULL;
    xaml_automation_peers::IItemAutomationPeer* pPeer = NULL;
    IInspectable* pSelectedItem = NULL;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC((static_cast<Selector*>(pOwner))->get_SelectedItemsInternal(&pSelectedItemsImpl));
    if (pSelectedItemsImpl)
    {
        UINT numSelected = 0;
        IFC(pSelectedItemsImpl->get_Size(&numSelected));
        if(numSelected > 0)
        {
            IFC(pSelectedItemsImpl->GetAt(0, &pSelectedItem));
            if (pSelectedItem)
            {
                IFC(CreateItemAutomationPeer(pSelectedItem, &pPeer));
            }
        }
    }

    *pAP = pPeer;
    pPeer = NULL;

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pSelectedItemsImpl);
    ReleaseInterface(pSelectedItem);
    ReleaseInterface(pPeer);
    RRETURN(hr);
}
