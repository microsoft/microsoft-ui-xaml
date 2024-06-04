// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GroupItemAutomationPeer.g.h"
#include "GroupItem.g.h"
#include "ItemAutomationPeer.g.h"
#include "ItemsControlAutomationPeer.g.h"
#include "ItemsControl.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GroupItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IGroupItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IGroupItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IGroupItemAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<GroupItem*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<GroupItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the GroupAutomationPeer class.
GroupItemAutomationPeer::GroupItemAutomationPeer()
{
}

// Deconstructor
GroupItemAutomationPeer::~GroupItemAutomationPeer()
{
}

IFACEMETHODIMP GroupItemAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    wfc::IVector<xaml::UIElement*>* pItemsFromItemsHostPanel = NULL;
    wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren = NULL;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pItemsControlAutomationPeer = NULL;
    xaml_controls::IItemsControl* pTemplatedItemsControl = NULL;
    xaml_automation_peers::IAutomationPeer* pItemPeerAsAP = NULL;
    xaml_automation_peers::IAutomationPeer* pContainerItemPeer = NULL;
    xaml_controls::IPanel* pItemsHostPanel = NULL;
    xaml_controls::IControl* pHeaderContent = NULL;
    xaml::IUIElement* pHeaderContentAsUIE = NULL;
    IUIElement* pItemContainer = NULL;
    IInspectable* pItem = NULL;
    IDependencyObject* pItemContainerAsDO = NULL;
    BOOLEAN bFoundInChildrenCache = FALSE;

    IFCPTR(ppReturnValue);

    IFC(ctl::ComObject<TrackerCollection<xaml_automation_peers::AutomationPeer*>>::CreateInstance(&pAPChildren));

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    // Append Header content of the Groups before the Items in UIA tree.
    IFC((static_cast<GroupItem*>(pOwner))->GetHeaderContent(&pHeaderContent));
    if(pHeaderContent)
    {
        IFC(ctl::do_query_interface(pHeaderContentAsUIE, pHeaderContent));
        IFC(FrameworkElementAutomationPeer::GetAutomationPeerChildren(pHeaderContentAsUIE, pAPChildren));
    }

    IFC(get_ParentItemsControlAutomationPeer(&pItemsControlAutomationPeer));
    IFC((static_cast<GroupItem*>(pOwner))->GetTemplatedItemsControl(&pTemplatedItemsControl));
    if(pTemplatedItemsControl && pItemsControlAutomationPeer)
    {
        IFC((static_cast<ItemsControl*>(pTemplatedItemsControl))->get_ItemsHost(&pItemsHostPanel));
        if(pItemsHostPanel)
        {
            IFC(pItemsHostPanel->get_Children(&pItemsFromItemsHostPanel));
            UINT nCount = 0;
            IFC(pItemsFromItemsHostPanel->get_Size(&nCount));
            if(nCount > 0)
            {
                for(UINT i = 0; i < nCount; i++)
                {
                    IFC(pItemsFromItemsHostPanel->GetAt(i, &pItemContainer));
                    IFC(ctl::do_query_interface(pItemContainerAsDO, pItemContainer));
                    IFCEXPECT(pItemContainerAsDO);
                    IFC((static_cast<ItemsControl*>(pTemplatedItemsControl))->GetItemOrContainerFromContainer(pItemContainerAsDO, &pItem));

                    if(pItem != NULL)
                    {
                        IFC(static_cast<ItemsControlAutomationPeer*>(pItemsControlAutomationPeer)->GetItemPeerFromChildrenCache(pItem, &pItemPeer));
                        if(!pItemPeer)
                        {
                            BOOLEAN bFoundInCache = FALSE;
                            IFC(static_cast<ItemsControlAutomationPeer*>(pItemsControlAutomationPeer)->GetItemPeerFromItemContainerCache(pItem, &pItemPeer, bFoundInCache));
                        }
                        else
                        {
                            bFoundInChildrenCache = TRUE;
                        }
                        if(!pItemPeer)
                        {
                            IFC(static_cast<ItemsControlAutomationPeer*>(pItemsControlAutomationPeer)->OnCreateItemAutomationPeerProtected(pItem, &pItemPeer));
                        }

                        if(pItemPeer != NULL)
                        {
                            IFC(static_cast<ItemAutomationPeer*>(pItemPeer)->GetContainerPeer(&pContainerItemPeer));
                            if(pContainerItemPeer)
                            {
                                IFC(ctl::do_query_interface(pItemPeerAsAP, pItemPeer));
                                IFC(static_cast<AutomationPeer*>(pContainerItemPeer)->put_EventsSource(pItemPeerAsAP));
                                IFC(pAPChildren->Append(pItemPeerAsAP));
                                if(!bFoundInChildrenCache)
                                {
                                    IFC(static_cast<ItemsControlAutomationPeer*>(pItemsControlAutomationPeer)->AddItemAutomationPeerToItemPeerStorage(static_cast<ItemAutomationPeer*>(pItemPeer)));
                                }
                            }
                        }
                    }
                    ReleaseInterface(pContainerItemPeer);
                    ReleaseInterface(pItemPeerAsAP);
                    ReleaseInterface(pItem);
                    ReleaseInterface(pItemContainer);
                    ReleaseInterface(pItemContainerAsDO);
                    ReleaseInterface(pItemPeer);
                }
            }
        }
    }
    *ppReturnValue = pAPChildren;
    pAPChildren = NULL;

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pItemsFromItemsHostPanel);
    ReleaseInterface(pItemsHostPanel);
    ReleaseInterface(pItem);
    ReleaseInterface(pItemPeer);
    ReleaseInterface(pItemPeerAsAP);
    ReleaseInterface(pContainerItemPeer);
    ReleaseInterface(pItemContainer);
    ReleaseInterface(pItemContainerAsDO);
    ReleaseInterface(pItemsControlAutomationPeer);
    ReleaseInterface(pTemplatedItemsControl);
    ReleaseInterface(pHeaderContent);
    ReleaseInterface(pHeaderContentAsUIE);
    ReleaseInterface(pAPChildren);
    RRETURN(hr);
}

IFACEMETHODIMP GroupItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GroupItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP GroupItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;
    RRETURN(S_OK);
}

// Gets the AP for Parent ItemsControl for this GroupItem if there is one.
_Check_return_ HRESULT GroupItemAutomationPeer::get_ParentItemsControlAutomationPeer(_Out_ xaml_automation_peers::IItemsControlAutomationPeer** ppParentItemsControl)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml_controls::IItemsControl* pItemsControl = NULL;
    xaml_automation_peers::IItemsControlAutomationPeer* pItemsControlAutomationPeer = NULL;
    xaml_automation_peers::IAutomationPeer* pItemsControlAutomationPeerAsAP = NULL;
    IFCPTR(ppParentItemsControl);
    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFC(ItemsControl::ItemsControlFromItemContainer(static_cast<UIElement*>(pOwner), &pItemsControl));
    if(pItemsControl)
    {
        IFC(static_cast<ItemsControl*>(pItemsControl)->GetOrCreateAutomationPeer(&pItemsControlAutomationPeerAsAP));
        IFC(ctl::do_query_interface(pItemsControlAutomationPeer, pItemsControlAutomationPeerAsAP));
        *ppParentItemsControl = pItemsControlAutomationPeer;
        pItemsControlAutomationPeer = NULL;
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pItemsControl);
    ReleaseInterface(pItemsControlAutomationPeerAsAP);
    ReleaseInterface(pItemsControlAutomationPeer);
    RRETURN(hr);
}
