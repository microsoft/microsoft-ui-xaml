// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlipViewItemDataAutomationPeer.g.h"
#include "FlipViewAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT FlipViewItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* item,
    _In_ xaml_automation_peers::IFlipViewAutomationPeer* parent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IFlipViewItemDataAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IFlipViewItemDataAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pParentAsItemsControl = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(parent);
    
    IFC(ctl::do_query_interface(pParentAsItemsControl, parent));
    IFC(ActivateInstance(pOuter,
            static_cast<FlipViewAutomationPeer*>(parent)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<FlipViewItemDataAutomationPeer*>(pInstance)->put_Parent(pParentAsItemsControl));
    IFC(static_cast<FlipViewItemDataAutomationPeer*>(pInstance)->put_Item(item));

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

// Initializes a new instance of the FlipViewItemDataAutomationPeer class.
FlipViewItemDataAutomationPeer::FlipViewItemDataAutomationPeer()
{
}

// Deconstructor
FlipViewItemDataAutomationPeer::~FlipViewItemDataAutomationPeer()
{
}

IFACEMETHODIMP FlipViewItemDataAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"FlipViewItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlipViewItemDataAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

_Check_return_ HRESULT FlipViewItemDataAutomationPeer::ScrollIntoViewImpl()
{
    RRETURN(ScrollIntoViewCommon());
}

IFACEMETHODIMP FlipViewItemDataAutomationPeer::Realize()
{
    HRESULT hr = S_OK;

    IFC(ScrollIntoView());

Cleanup:
    RRETURN(hr);
}
