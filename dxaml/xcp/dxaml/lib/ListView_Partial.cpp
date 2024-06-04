// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListView.g.h"
#include "ListViewAutomationPeer.g.h"
#include "ListViewItem.g.h"
#include "ListViewHeaderItem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ListView::ListView()
    :m_itemsAcceptFocusFromUIA(true)
{
}

// Create ListViewAutomationPeer to represent the ListView.
IFACEMETHODIMP ListView::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewAutomationPeer> spListViewAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IListViewAutomationPeerFactory> spListViewAPFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    
    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ListViewAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IListViewAutomationPeerFactory>(&spListViewAPFactory));

    IFC(spListViewAPFactory->CreateInstanceWithOwner(this, 
        NULL, 
        &spInner, 
        &spListViewAutomationPeer));
    IFC(spListViewAutomationPeer.CopyTo(ppAutomationPeer));

Cleanup:
    RRETURN(hr);
}

// Determines if the specified item is (or is eligible to be) its own container.
IFACEMETHODIMP ListView::IsItemItsOwnContainerOverride(
    _In_ IInspectable* item, 
    _Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    
    IFCPTR(returnValue);
    
    *returnValue = !!ctl::value_is<IListViewItem>(item);
    
Cleanup:
    RRETURN(hr);
}

// Creates or identifies the element that is used to display the given item.
IFACEMETHODIMP ListView::GetContainerForItemOverride(
    _Outptr_ IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewItem> spItem;
    
    IFCPTR(returnValue);
    *returnValue = NULL;
    
    IFC(ctl::make<ListViewItem>(&spItem));
    
    IFC(spItem.CopyTo(returnValue));
    
Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT ListView::GetHeaderForGroupOverrideImpl(_Outptr_ xaml::IDependencyObject** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ListViewHeaderItem> spContainer;

    IFCEXPECT(returnValue);
    IFC(ctl::make(&spContainer));

    IFC(spContainer.MoveTo(returnValue));

Cleanup:
    RRETURN(hr);
}
