// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComboBoxItemDataAutomationPeer.g.h"
#include "ComboBoxAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ComboBoxItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* item,
    _In_ xaml_automation_peers::IComboBoxAutomationPeer* parent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IComboBoxItemDataAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IComboBoxItemDataAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pParentAsItemsControl = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(parent);
    
    IFC(ctl::do_query_interface(pParentAsItemsControl, parent));
    IFC(ActivateInstance(pOuter,
            static_cast<ComboBoxAutomationPeer*>(parent)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ComboBoxItemDataAutomationPeer*>(pInstance)->put_Parent(pParentAsItemsControl));
    IFC(static_cast<ComboBoxItemDataAutomationPeer*>(pInstance)->put_Item(item));

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

// Initializes a new instance of the ComboBoxItemDataAutomationPeer class.
ComboBoxItemDataAutomationPeer::ComboBoxItemDataAutomationPeer()
{
}

// Deconstructor
ComboBoxItemDataAutomationPeer::~ComboBoxItemDataAutomationPeer()
{
}

IFACEMETHODIMP ComboBoxItemDataAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ComboBoxItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ComboBoxItemDataAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_ListItem;
    RRETURN(S_OK);
}

_Check_return_ HRESULT ComboBoxItemDataAutomationPeer::ScrollIntoViewImpl()
{
    RRETURN(ScrollIntoViewCommon());
}

// ISelectionItemProvider implementation for ComboBox, needed to override default to Collapse ComboBox after the selection
// to work with Touch + Narrator behavior.
_Check_return_ HRESULT ComboBoxItemDataAutomationPeer::SelectImpl()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_automation::Provider::IExpandCollapseProvider* pProvider = NULL;
    
    IFC(ComboBoxItemDataAutomationPeerGenerated::SelectImpl());
    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    if(pParent != NULL)
    {
        IFC(ctl::do_query_interface(pProvider, pParent));
        if(pProvider)
        {
            IFC(pProvider->Collapse());
        }
    }
Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pProvider);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxItemDataAutomationPeer::AddToSelectionImpl()
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pParent = NULL;
    xaml_automation::Provider::IExpandCollapseProvider* pProvider = NULL;

    IFC(ComboBoxItemDataAutomationPeerGenerated::AddToSelectionImpl());
    IFC(this->get_ItemsControlAutomationPeer(&pParent));
    if (pParent != NULL)
    {
        IFC(ctl::do_query_interface(pProvider, pParent));
        if (pProvider)
        {
            IFC(pProvider->Collapse());
        }
    }
Cleanup:
    ReleaseInterface(pParent);
    ReleaseInterface(pProvider);
    RRETURN(hr);
}

_Check_return_ HRESULT ComboBoxItemDataAutomationPeer::RemoveFromSelectionImpl()
{
    // NOP - Deselection of current selection is not allowed
    RRETURN(S_OK);
}

IFACEMETHODIMP ComboBoxItemDataAutomationPeer::Realize()
{
    HRESULT hr = S_OK;

    IFC(ScrollIntoView());

Cleanup:
    RRETURN(hr);
}
