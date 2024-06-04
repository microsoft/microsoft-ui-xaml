// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListBoxItemDataAutomationPeer.g.h"
#include "ListBoxAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListBoxItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* item,
    _In_ xaml_automation_peers::IListBoxAutomationPeer* parent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListBoxItemDataAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IListBoxItemDataAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pParentAsItemsControl = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(parent);
    
    IFC(ctl::do_query_interface(pParentAsItemsControl, parent));
    IFC(ActivateInstance(pOuter,
            static_cast<ListBoxAutomationPeer*>(parent)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ListBoxItemDataAutomationPeer*>(pInstance)->put_Parent(pParentAsItemsControl));
    IFC(static_cast<ListBoxItemDataAutomationPeer*>(pInstance)->put_Item(item));

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

// Initializes a new instance of the ListBoxItemDataAutomationPeer class.
ListBoxItemDataAutomationPeer::ListBoxItemDataAutomationPeer()
{
}

// Deconstructor
ListBoxItemDataAutomationPeer::~ListBoxItemDataAutomationPeer()
{
}

IFACEMETHODIMP ListBoxItemDataAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListBoxItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListBoxItemDataAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ListBoxItemDataAutomationPeer::ScrollIntoViewImpl()
{
    RRETURN(ScrollIntoViewCommon());
}

IFACEMETHODIMP ListBoxItemDataAutomationPeer::Realize()
{
    HRESULT hr = S_OK;

    IFC(ScrollIntoView());

Cleanup:
    RRETURN(hr);
}
