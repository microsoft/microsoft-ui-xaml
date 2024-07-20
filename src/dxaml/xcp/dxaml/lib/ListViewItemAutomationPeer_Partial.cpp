// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewItemAutomationPeer.g.h"
#include "ListViewItem.g.h"
#include "ListView_Partial.h"
#include "ItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListViewItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListViewItem* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListViewItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewItemAutomationPeer> spInstance = NULL;
    ctl::ComPtr<IInspectable> spInner = NULL;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<ListViewItem*>(pOwner)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::IListViewItemAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<ListViewItemAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the ListViewItemAutomationPeer class.
ListViewItemAutomationPeer::ListViewItemAutomationPeer()
{
}

// Deconstructor
ListViewItemAutomationPeer::~ListViewItemAutomationPeer()
{
    if (m_wpRemovableItemAutomationPeer)
    {
        // Remove the ItemAutomationPeer from the ItemsControl storage when the UIAWrapper isn't available
        auto automationPeer = m_wpRemovableItemAutomationPeer.AsOrNull<IAutomationPeer>();
        if (automationPeer)
        {
            CAutomationPeer* nativeAPNoRef = static_cast<CAutomationPeer*>(automationPeer.Cast<AutomationPeer>()->GetHandle());
            if (nativeAPNoRef && nativeAPNoRef->GetUIAWrapper() == nullptr)
            {
                IGNOREHR(automationPeer.Cast<ItemAutomationPeer>()->RemoveItemAutomationPeerFromItemsControlStorage());
            }
        }
        m_wpRemovableItemAutomationPeer.Reset();
    }
}

IFACEMETHODIMP ListViewItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListViewItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewItemAutomationPeer::SetFocusCore()
{
    ctl::ComPtr<xaml::IUIElement> owner;
    ctl::ComPtr<ListViewBase> parentListViewBase;
    ctl::ComPtr<ListView> parentListView;

    IFC_RETURN(get_Owner(&owner));
    parentListViewBase = owner.Cast<ListViewBaseItem>()->GetParentListView();
    if (parentListViewBase)
    {
        parentListView = parentListViewBase.AsOrNull<ListView>();
        if (parentListView)
        {
            if (!parentListView->ItemAllowFocusFromUIA())
            {
                // does not allow setting focus by UIA, just return S_OK
                return S_OK;
            }
        }
    }

    return (__super::SetFocusCore());
}

_Check_return_ HRESULT
ListViewItemAutomationPeer::SetRemovableItemAutomationPeer(
    _In_ xaml_automation_peers::IItemAutomationPeer* itemAP)
{
    m_wpRemovableItemAutomationPeer.Reset();
    IFC_RETURN(ctl::AsWeak(itemAP, &m_wpRemovableItemAutomationPeer));
    return S_OK;
}