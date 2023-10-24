// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SelectorItemAutomationPeer_partial.h"
#include "SelectorItem.g.h"
#include "SelectorAutomationPeer.g.h"
#include "Selector.g.h"
#include "ItemCollection.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SelectorItemAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* item,
    _In_ xaml_automation_peers::ISelectorAutomationPeer* parent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ISelectorItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ISelectorItemAutomationPeer* pInstance = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pParentAsItemsControl = NULL;
    IInspectable* pInner = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(parent);
    IFCPTR(item);

    IFC(ctl::do_query_interface(pParentAsItemsControl, parent));
    IFC(ActivateInstance(pOuter,
            static_cast<SelectorAutomationPeer*>(parent)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<SelectorItemAutomationPeer*>(pInstance)->put_Parent(pParentAsItemsControl));
    IFC(static_cast<SelectorItemAutomationPeer*>(pInstance)->put_Item(item));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pParentAsItemsControl);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

// Initializes a new instance of the SelectorItemAutomationPeer class.
SelectorItemAutomationPeer::SelectorItemAutomationPeer()
{
}

// Deconstructor
SelectorItemAutomationPeer::~SelectorItemAutomationPeer()
{
}

IFACEMETHODIMP SelectorItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    ctl::ComPtr<IInspectable> spPatternProvider;

    IFCPTR_RETURN(ppReturnValue);
    *ppReturnValue = NULL;

    IFC_RETURN(SelectorItemAutomationPeerGenerated::GetPatternCore(patternInterface, spPatternProvider.ReleaseAndGetAddressOf()));

    if (spPatternProvider == NULL)
    {
        if(patternInterface == xaml_automation_peers::PatternInterface_SelectionItem)
        {
            bool selectionPatternApplicable = true;

            ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> parent;
            IFC_RETURN(get_ItemsControlAutomationPeer(&parent));
            if (parent)
            {
                ctl::ComPtr<xaml::IUIElement> owner;
                IFC_RETURN(parent.Cast<ItemsControlAutomationPeer>()->get_Owner(&owner));
                ctl::ComPtr<xaml_primitives::ISelector> selector;
                IFC_RETURN(owner.As<ISelector>(&selector));
                IFC_RETURN(selector.Cast<Selector>()->IsSelectionPatternApplicable(&selectionPatternApplicable));
            }

            if (selectionPatternApplicable)
            {
                spPatternProvider = ctl::as_iinspectable(this);
            }
        }
        else if (patternInterface == xaml_automation_peers::PatternInterface_ScrollItem
            && ctl::is<xaml_automation::Provider::IScrollItemProvider>(this))
        {
            spPatternProvider = ctl::as_iinspectable(this);
        }
    }

    IFC_RETURN(spPatternProvider.MoveTo(ppReturnValue));

    return S_OK;
}

_Check_return_ HRESULT SelectorItemAutomationPeer::SelectImpl()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_primitives::ISelector* pSelector = NULL;
    xaml::IUIElement* pSelectorASUIE = NULL;
    wfc::IObservableVector<IInspectable*>* pItems = NULL;
    IInspectable* pItem = NULL;
    UINT index = -1;
    BOOLEAN found = FALSE;
    BOOLEAN bIsEnabled = FALSE;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    IFC(this->get_Item(&pItem));
    if(pParent != NULL)
    {
        IFC(static_cast<ItemsControlAutomationPeer*>(pParent)->get_Owner(&pSelectorASUIE));
        IFC(ctl::do_query_interface(pSelector, pSelectorASUIE));
        if(!pSelector)
        {
            IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
        }

        IFC((static_cast<Selector*>(pSelector))->get_Items(&pItems));
        IFC(static_cast<ItemCollection*>(pItems)->IndexOf(pItem, &index, &found));
        IFC(static_cast<Selector*>(pSelector)->MakeSingleSelection(index, FALSE /*animateIfBringIntoView*/, pItem));
    }

Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pSelectorASUIE);
    ReleaseInterface(pSelector);
    ReleaseInterface(pItems);
    ReleaseInterface(pItem);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItemAutomationPeer::AddToSelectionImpl()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_primitives::ISelector* pSelector = NULL;
    xaml::IUIElement* pSelectorASUIE = NULL;
    wfc::IObservableVector<IInspectable*>* pItems = NULL;
    IInspectable* pItem = NULL;
    UINT index = -1;
    BOOLEAN found = FALSE;
    BOOLEAN bIsEnabled = FALSE;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    IFC(this->get_Item(&pItem));
    if(pParent != NULL)
    {
        IFC(static_cast<ItemsControlAutomationPeer*>(pParent)->get_Owner(&pSelectorASUIE));
        IFC(ctl::do_query_interface(pSelector, pSelectorASUIE));
        if(!pSelector)
        {
            IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
        }

        IFC((static_cast<Selector*>(pSelector))->get_Items(&pItems));
        IFC(static_cast<ItemCollection*>(pItems)->IndexOf(pItem, &index, &found));
        IFC(static_cast<Selector*>(pSelector)->AutomationPeerAddToSelection(index, pItem));
    }

Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pSelectorASUIE);
    ReleaseInterface(pSelector);
    ReleaseInterface(pItems);
    ReleaseInterface(pItem);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItemAutomationPeer::RemoveFromSelectionImpl()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_primitives::ISelector* pSelector = NULL;
    xaml::IUIElement* pSelectorASUIE = NULL;
    wfc::IObservableVector<IInspectable*>* pItems = NULL;
    IInspectable* pItem = NULL;
    UINT index = -1;
    BOOLEAN found = FALSE;
    BOOLEAN bIsEnabled = FALSE;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    IFC(this->get_Item(&pItem));
    if(pParent != NULL)
    {
        IFC(static_cast<ItemsControlAutomationPeer*>(pParent)->get_Owner(&pSelectorASUIE));
        IFC(ctl::do_query_interface(pSelector, pSelectorASUIE));
        if(!pSelector)
        {
            IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
            IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
        }

        IFC((static_cast<Selector*>(pSelector))->get_Items(&pItems));
        IFC(static_cast<ItemCollection*>(pItems)->IndexOf(pItem, &index, &found));
        IFC(static_cast<Selector*>(pSelector)->AutomationPeerRemoveFromSelection(index, pItem));
    }

Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pSelector);
    ReleaseInterface(pSelectorASUIE);
    ReleaseInterface(pItems);
    ReleaseInterface(pItem);
    RRETURN(hr);
}


_Check_return_ HRESULT SelectorItemAutomationPeer::get_IsSelectedImpl(_Out_ BOOLEAN* pValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_primitives::ISelector* pSelector = NULL;
    xaml::IUIElement* pSelectorASUIE = NULL;
    IInspectable* pItem = NULL;
    BOOLEAN bIsEnabled = FALSE;

    IFC(IsEnabled(&bIsEnabled));
    if(!bIsEnabled)
    {
        IFC(static_cast<HRESULT>(UIA_E_ELEMENTNOTENABLED));
    }

    *pValue = FALSE;

    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    IFC(this->get_Item(&pItem));
    if(pParent != NULL)
    {
        IFC(static_cast<ItemsControlAutomationPeer*>(pParent)->get_Owner(&pSelectorASUIE));
        IFC(ctl::do_query_interface(pSelector, pSelectorASUIE));

        if(pSelector != NULL)
        {
            IFC(static_cast<Selector*>(pSelector)->AutomationPeerIsSelected(pItem, pValue));
        }
    }

Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pSelectorASUIE);
    ReleaseInterface(pSelector);
    ReleaseInterface(pItem);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItemAutomationPeer::get_SelectionContainerImpl(_Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_automation_peers::IAutomationPeer* pParentAsAP = NULL;
    xaml_automation::Provider::IIRawElementProviderSimple* pProvider = NULL;

    *ppReturnValue = NULL;
    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    if(pParent != NULL)
    {
        IFC(ctl::do_query_interface(pParentAsAP, pParent));
        IFC(ProviderFromPeer(pParentAsAP, &pProvider));
        *ppReturnValue = pProvider;
    }

    pProvider = NULL;

Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pParentAsAP);
    ReleaseInterface(pProvider);
    RRETURN(hr);
}

_Check_return_ HRESULT SelectorItemAutomationPeer::ScrollIntoViewCommon()
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParent;
    ctl::ComPtr<IInspectable> spItem;

    IFC(this->get_ItemsControlAutomationPeer(&spParent));
    IFC(this->get_Item(&spItem));
    if(spParent)
    {
        ctl::ComPtr<xaml::IUIElement> spSelectorASUIE;
        ctl::ComPtr<xaml_primitives::ISelector> spSelector;

        IFC(spParent.Cast<ItemsControlAutomationPeer>()->get_Owner(&spSelectorASUIE));
        IFC(spSelectorASUIE.As<ISelector>(&spSelector));
        IFC(spSelector.Cast<Selector>()->ScrollIntoViewInternal(spItem.Get(), /*isHeader*/FALSE, /*isFooter*/FALSE, /*isFromPublicAPI*/TRUE, xaml_controls::ScrollIntoViewAlignment::ScrollIntoViewAlignment_Default));
    }

Cleanup:
    RRETURN(hr);
}
