// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ItemsControlAutomationPeer.g.h"
#include "ItemsControl.g.h"
#include "ScrollViewer.g.h"
#include "ScrollViewerAutomationPeer.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemAutomationPeer.g.h"
#include "ItemCollection.g.h"
#include "SelectorItemAutomationPeer.g.h"
#include "GroupItem.g.h"
#include "ListViewBaseHeaderItemAutomationPeer.g.h"
#include "GroupItemAutomationPeer.g.h"
#include "IRawElementProviderSimple.g.h"
#include "AutomationProperty.g.h"
#include "VisualTreeHelper.h"
#include "ListViewItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_hosting;

_Check_return_ HRESULT ItemsControlAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IItemsControl* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IItemsControlAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemsControlAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<ItemsControl*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ItemsControlAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the ItemsControlAutomationPeer class.
ItemsControlAutomationPeer::ItemsControlAutomationPeer() : m_lastIndex(-1)
{
}

// Deconstructor
ItemsControlAutomationPeer::~ItemsControlAutomationPeer()
{

    // Release the Item from ItemAutomationPeer to break the circular chain made specially in the case when
    // the item is the container itself. The chain will be like
    // ListBoxItemDataAP---->(via Item, we are breaking this)ListBoxItem----->(stores AP)ListBoxItemAP---
    // ---->(original EvetnsSource)back to ListBoxItemDataAP

    // The two storages could be in an unsafe state though; they could reference something that's GC'd but not finalized.
    // So we get an "unsafe" reference to the TrackerCollection, and call a routine that's careful not to use it in a way
    // that would access one of these zombie objects.

    auto unsafeTrackerCollection = static_cast<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>*>
        (ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(m_tpItemPeerStorage.GetAsReferenceTrackerUnsafe()));
    ClearCacheCollectionUnsafe(unsafeTrackerCollection);


    unsafeTrackerCollection = static_cast<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>*>
        (ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(m_tpItemPeerStorageForPattern.GetAsReferenceTrackerUnsafe()));
    ClearCacheCollectionUnsafe(unsafeTrackerCollection);

}



// Call ReleaseItemAndParent on each of the items in the passed-in TrackerCollection.  We have to use this
// collection and items carefully, because they may be in a GC'd but not finalized state.  So we can't call AddRef/Release,
// and can't call QI.

void ItemsControlAutomationPeer::ClearCacheCollectionUnsafe(
    _In_ TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>* unsafeTrackerCollection)
{
    IReferenceTrackerInternal *pTrackerNoRef = nullptr;
    UINT nCount = 0;

    if (unsafeTrackerCollection != nullptr)
    {
        VERIFYHR(unsafeTrackerCollection->get_Size(&nCount));
        if(nCount > 0)
        {
            for(UINT i = 0; i < nCount; i++)
            {
                VERIFYHR(unsafeTrackerCollection->GetAsReferenceTrackerUnsafe(i, &pTrackerNoRef));
                if(pTrackerNoRef != NULL)
                {
                    static_cast<ItemAutomationPeer*>(ctl::impl_cast<ctl::WeakReferenceSourceNoThreadId>(pTrackerNoRef))->ReleaseItemAndParent();
                }
            }
        }
    }
}


IFACEMETHODIMP ItemsControlAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** returnValue)
{
    HRESULT hr = S_OK;
    xaml::IUIElement* pOwner = NULL;
    xaml::IDependencyObject* pOwnerDO = NULL;
    xaml_controls::IPanel* pItemsHost = NULL;
    xaml::IDependencyObject* pCurrent = NULL;
    xaml_controls::IScrollViewer* psv = NULL;
    xaml::IDependencyObject* pDO = NULL;
    xaml_automation_peers::IAutomationPeer* pScrollPeer = NULL;
    xaml_automation::Provider::IScrollProvider* psp = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Scroll)
    {
        IFC(get_Owner(&pOwner));
        IFCPTR(pOwner);
        IFC((static_cast<ItemsControl*>(pOwner))->get_ItemsHost(&pItemsHost));
        pCurrent = ctl::query_interface<xaml::IDependencyObject>(pItemsHost);
        pOwnerDO = ctl::query_interface<xaml::IDependencyObject>(pOwner);

        // Walk the tree up, checking to see if there is a scroll viewer or not.
        // Two exit criterias
        // (1) current == owner  ==> Means we reached the ItemsControl itself.
        // (2) sv != null        ==> ScrollViewer has been found.

        // For the combobox which haven't been expanded yet ItemsHost is null
        // so skipping the tree traversing. Selector should provide this functionality.
        while (pCurrent != NULL && pCurrent != pOwnerDO)
        {
            IFC(VisualTreeHelper::GetParentStatic(pCurrent, &pDO));
            ReleaseInterface(pCurrent);
            pCurrent = pDO;

            psv = ctl::query_interface<IScrollViewer>(pCurrent);
            if (psv != NULL)
                break;
        }

        if (psv != NULL)
        {
            IFC(static_cast<ScrollViewer*>(psv)->GetOrCreateAutomationPeer(&pScrollPeer));
            if(pScrollPeer)
            {
                psp = ctl::query_interface<xaml_automation::Provider::IScrollProvider>(pScrollPeer);
            }

            if (psp != NULL)
            {
                *returnValue = ctl::as_iinspectable(static_cast<AutomationPeer*>(pScrollPeer));
                ctl::addref_interface(static_cast<AutomationPeer*>(pScrollPeer));
                IFC(static_cast<AutomationPeer*>(pScrollPeer)->put_EventsSource(this));
            }
        }
    }
    else if(patternInterface == xaml_automation_peers::PatternInterface_ItemContainer)
    {
        *returnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ItemsControlAutomationPeerGenerated::GetPatternCore(patternInterface, returnValue));
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pOwnerDO);
    ReleaseInterface(pItemsHost);
    ReleaseInterface(pCurrent);
    ReleaseInterface(psv);
    ReleaseInterface(pScrollPeer);
    ReleaseInterface(psp);
    RRETURN(hr);
}

IFACEMETHODIMP ItemsControlAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren = NULL;

    IFCPTR(ppReturnValue);
    IFC(ctl::ComObject<TrackerCollection<xaml_automation_peers::AutomationPeer*>>::CreateInstance(&pAPChildren));
    IFC(GetItemsControlChildrenChildren(pAPChildren));

    *ppReturnValue = pAPChildren;
    pAPChildren = NULL;

Cleanup:
    ReleaseInterface(pAPChildren);
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsControlAutomationPeer::GetItemsControlChildrenChildrenHelper(_In_ xaml_controls::IItemsControl* pOwner,
    _In_ wfc::IVector<xaml::UIElement*>* pItemsFromItemsHostPanel,
    _In_ UINT nCount,
    _In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pNewChildrenCollection)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    xaml_automation_peers::IAutomationPeer* pItemPeerAsAP = NULL;
    xaml_automation_peers::IAutomationPeer* pContainerItemPeer = NULL;
    IUIElement* pItemContainer = NULL;
    IInspectable* pItem = NULL;
    xaml::IDependencyObject* pItemContainerAsDO = NULL;

    for(UINT i = 0; i < nCount; i++)
    {
        IFC(pItemsFromItemsHostPanel->GetAt(i, &pItemContainer));
        IFC(ctl::do_query_interface(pItemContainerAsDO, pItemContainer));
        IFCEXPECT(pItemContainerAsDO);
        IFC((static_cast<ItemsControl*>(pOwner))->GetItemOrContainerFromContainer(pItemContainerAsDO, &pItem));

        if(pItem != NULL)
        {
            IFC(GetItemPeerFromChildrenCache(pItem, &pItemPeer));
            if(!pItemPeer)
            {
                BOOLEAN bFoundInCache = FALSE;
                IFC(GetItemPeerFromItemContainerCache(pItem, &pItemPeer, bFoundInCache));
            }
            if(!pItemPeer)
            {
                IFC(OnCreateItemAutomationPeerProtected(pItem, &pItemPeer));
            }

            if(pItemPeer != NULL)
            {
                IFC(static_cast<ItemAutomationPeer*>(pItemPeer)->GetContainerPeer(&pContainerItemPeer));
                if(pContainerItemPeer)
                {
                    IFC(ctl::do_query_interface(pItemPeerAsAP, pItemPeer));
                    IFC(static_cast<AutomationPeer*>(pContainerItemPeer)->put_EventsSource(pItemPeerAsAP));
                    IFC(pNewChildrenCollection->Append(pItemPeer));
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

Cleanup:
    ReleaseInterface(pContainerItemPeer);
    ReleaseInterface(pItemPeerAsAP);
    ReleaseInterface(pItem);
    ReleaseInterface(pItemContainer);
    ReleaseInterface(pItemContainerAsDO);
    ReleaseInterface(pItemPeer);

    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::GetModernItemsControlChildrenChildrenHelper(_In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pNewChildrenCollection)
{
    HRESULT hr = S_OK;
    INT firstCacheIndex = -1;
    INT lastCacheIndex = -1;
    ctl::ComPtr<IDependencyObject> spItemContainerAsDO;
    ctl::ComPtr<IInspectable> spItemAsInspectable;
    ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spItemPeer;

    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spItemPeerAsAP;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spContainerItemPeer;
    ctl::ComPtr<IModernCollectionBasePanel> spModernPanel;
    ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<xaml_controls::IItemsControl> spOwnerAsItemsControl;

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner);
    IFC(spOwner.As(&spOwnerAsItemsControl));

    IFC(spOwnerAsItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHostPanel));
    IFC(spItemsHostPanel.As(&spModernPanel));

    IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheIndexBase(&firstCacheIndex));
    IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&lastCacheIndex));

    if (firstCacheIndex >= 0 && lastCacheIndex >= 0)
    {
        for (int indexContainer = firstCacheIndex; indexContainer <= lastCacheIndex ; ++indexContainer)
        {
            IFC(spModernPanel.Cast<ModernCollectionBasePanel>()->ContainerFromIndex(indexContainer, &spItemContainerAsDO));
            if (spItemContainerAsDO)
            {
                IFC(spOwnerAsItemsControl.Cast<ItemsControl>()->GetItemOrContainerFromContainer(spItemContainerAsDO.Get(), &spItemAsInspectable));
                if (spItemAsInspectable)
                {
                    IFC(GetItemPeerFromChildrenCache(spItemAsInspectable.Get(), &spItemPeer));
                    if (!spItemPeer)
                    {
                        BOOLEAN bFoundInCache = FALSE;
                        IFC(GetItemPeerFromItemContainerCache(spItemAsInspectable.Get(), &spItemPeer, bFoundInCache));
                    }
                    if (!spItemPeer)
                    {
                        IFC(CreateItemAutomationPeer(spItemAsInspectable.Get(), &spItemPeer));
                    }
                    if (spItemPeer)
                    {
                        IFC(spItemPeer.Cast<ItemAutomationPeer>()->GetContainerPeer(&spContainerItemPeer));
                        if (spContainerItemPeer)
                        {
                            IFC(spItemPeer.As(&spItemPeerAsAP));
                            IFC(spContainerItemPeer.Cast<AutomationPeer>()->put_EventsSource(spItemPeerAsAP.Get()));
                            IFC(pNewChildrenCollection->Append(spItemPeer.Get()));
                        }
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ItemsControlAutomationPeer::GetItemsControlChildrenChildren(
    _In_ wfc::IVector<xaml_automation_peers::AutomationPeer*>* pAPChildren) noexcept
{
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;

    BOOLEAN isGrouping = FALSE;

    IFC_RETURN(get_Owner(&spOwner));
    IFC_RETURN(spOwner.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));
    IFC_RETURN(spOwner.Cast<ItemsControl>()->get_ItemsHost(&spItemsHostPanel));

    if(spItemsHostPanel)
    {
        ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spItemsFromItemsHostPanel;
        IFC_RETURN(spItemsHostPanel->get_Children(&spItemsFromItemsHostPanel));

        UINT nCount = 0;
        IFC_RETURN(spItemsFromItemsHostPanel->get_Size(&nCount));

        if(nCount > 0)
        {
            ctl::ComPtr<IModernCollectionBasePanel> spModernPanel;
            ctl::ComPtr<TrackerCollection<xaml::Automation::Peers::ItemAutomationPeer*>> spNewChildrenCollection;

            IFC_RETURN(ctl::make(&spNewChildrenCollection));

            if(isGrouping)
            {
                spModernPanel = spItemsHostPanel.AsOrNull<IModernCollectionBasePanel>();
                if (spModernPanel)
                {
                    INT firstCacheGroup = -1;
                    INT lastCacheGroup = -1;
                    IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->get_FirstCacheGroupIndexBase(&firstCacheGroup));
                    IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->get_LastCacheGroupIndexBase(&lastCacheGroup));
                    if (firstCacheGroup >= 0 && lastCacheGroup >= 0)
                    {
                        for (INT i = firstCacheGroup; i <= lastCacheGroup; ++i)
                        {
                            ctl::ComPtr<IDependencyObject> spHeaderElementAsDO;
                            ctl::ComPtr<IUIElement> spHeaderElementAsUIE;
                            ctl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spItemPeerAsAP;
                            IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->HeaderFromIndex(i, &spHeaderElementAsDO));

                            // It's possible that a header doesn't exist, for example if it's hidden (HidesIfEmpty==true).
                            // Skip over such headers.
                            if (spHeaderElementAsDO)
                            {
                                IFC_RETURN(spHeaderElementAsDO.As(&spHeaderElementAsUIE));
                                IFC_RETURN(spHeaderElementAsUIE.Cast<UIElement>()->GetOrCreateAutomationPeer(&spItemPeerAsAP));
                                if(spItemPeerAsAP)
                                {
                                    IFC_RETURN(pAPChildren->Append(spItemPeerAsAP.Get()));
                                }
                            }
                        }
                        IFC_RETURN(GetModernItemsControlChildrenChildrenHelper(spNewChildrenCollection.Get()));
                    }
                }
                else
                {
                    for(UINT i = 0; i < nCount; i++)
                    {
                        ctl::ComPtr<IUIElement> spItemContainer;
                        ctl::ComPtr<xaml_controls::IGroupItem> spIGroupItem;
                        ctl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spItemPeerAsAP;

                        IFC_RETURN(spItemsFromItemsHostPanel->GetAt(i, &spItemContainer));
                        IFC_RETURN(spItemContainer.Cast<UIElement>()->GetOrCreateAutomationPeer(&spItemPeerAsAP));
                        if(spItemPeerAsAP)
                        {
                            IFC_RETURN(pAPChildren->Append(spItemPeerAsAP.Get()));
                        }

                        // We need to add the leaf elements to the new short term cache, pNewChildrenCollection, to prevent
                        // losing track of existing DataItemAutomation peers. UIA clients depend on the peers being stable.
                        spIGroupItem = spItemContainer.AsOrNull<xaml_controls::IGroupItem>();

                        if (spIGroupItem)
                        {
                            ctl::ComPtr<xaml_controls::IItemsControl> spGroupItemIItemsControl;
                            ctl::ComPtr<xaml_controls::IPanel> spGroupItemItemsHostPanel;
                            ctl::ComPtr<wfc::IVector<xaml::UIElement*>> spGroupItemItemsFromItemsHostPanel;

                            IFC_RETURN(spIGroupItem.Cast<GroupItem>()->GetTemplatedItemsControl(&spGroupItemIItemsControl));
                            if(spGroupItemIItemsControl)
                            {
                                IFC_RETURN(spGroupItemIItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spGroupItemItemsHostPanel));
                                if(spGroupItemItemsHostPanel)
                                {
                                    IFC_RETURN(spGroupItemItemsHostPanel->get_Children(&spGroupItemItemsFromItemsHostPanel));
                                    if (spGroupItemItemsFromItemsHostPanel)
                                    {
                                        UINT nGroupItemCount = 0;

                                        IFC_RETURN(spGroupItemItemsFromItemsHostPanel.Get()->get_Size(&nGroupItemCount));

                                        IFC_RETURN(GetItemsControlChildrenChildrenHelper(spGroupItemIItemsControl.Cast<ItemsControl>(), spGroupItemItemsFromItemsHostPanel.Get(),
                                            nGroupItemCount, spNewChildrenCollection.Get()));

                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                spModernPanel = ctl::ComPtr<xaml_controls::IPanel>(spItemsHostPanel).AsOrNull<IModernCollectionBasePanel>();
                if (spModernPanel)
                {
                    UINT nItems = 0;
                    IFC_RETURN(GetModernItemsControlChildrenChildrenHelper(spNewChildrenCollection.Get()));
                    IFC_RETURN(spNewChildrenCollection->get_Size(&nItems));

                    for (UINT idx = 0; idx < nItems; idx++)
                    {
                        ctl::ComPtr<IItemAutomationPeer> spItemAP;
                        IFC_RETURN(spNewChildrenCollection->GetAt(idx, &spItemAP));
                        IFC_RETURN(pAPChildren->Append(spItemAP.AsOrNull<IAutomationPeer>().Get()));
                    }
                }
                else
                {
                    xaml::Visibility visibility;

                    for(UINT i = 0; i < nCount; i++)
                    {
                        ctl::ComPtr<IUIElement> spItemContainer;
                        ctl::ComPtr<IDependencyObject> spItemContainerAsDO;
                        ctl::ComPtr<IInspectable> spItem;

                        IFC_RETURN(spItemsFromItemsHostPanel->GetAt(i, &spItemContainer));
                        IFC_RETURN(spItemContainer.Get()->get_Visibility(&visibility));
                        IFC_RETURN(spItemContainer.As(&spItemContainerAsDO));
                        IFCEXPECT_RETURN(spItemContainerAsDO);

                        IFC_RETURN(spOwner.Cast<ItemsControl>()->GetItemOrContainerFromContainer(spItemContainerAsDO.Get(), &spItem));

                        if(spItem && visibility != xaml::Visibility_Collapsed)
                        {
                            ctl::ComPtr<xaml::Automation::Peers::IItemAutomationPeer> spItemPeer;

                            IFC_RETURN(GetItemPeerFromChildrenCache(spItem.Get(), &spItemPeer));
                            if(!spItemPeer)
                            {
                                BOOLEAN bFoundInCache = FALSE;
                                IFC_RETURN(GetItemPeerFromItemContainerCache(spItem.Get(), &spItemPeer, bFoundInCache));
                            }

                            if(!spItemPeer)
                            {
                                IFC_RETURN(OnCreateItemAutomationPeerProtected(spItem.Get(), &spItemPeer));
                            }

                            if(spItemPeer)
                            {
                                ctl::ComPtr<xaml::Automation::Peers::IAutomationPeer> pContainerItemPeer;

                                IFC_RETURN(spItemPeer.Cast<ItemAutomationPeer>()->GetContainerPeer(&pContainerItemPeer));
                                if(pContainerItemPeer)
                                {
                                    ctl::ComPtr<xaml::Automation::Peers::IAutomationPeer> spItemPeerAsAP;
                                    IFC_RETURN(spItemPeer.As(&spItemPeerAsAP));
                                    IFC_RETURN(pContainerItemPeer.Cast<AutomationPeer>()->put_EventsSource(spItemPeerAsAP.Get()));
                                    IFC_RETURN(pAPChildren->Append(spItemPeerAsAP.Get()));
                                    IFC_RETURN(spNewChildrenCollection->Append(spItemPeer.Get()));
                                }
                            }
                        }
                    }
                }
            }

            IFC_RETURN(ReleaseItemPeerStorage(spNewChildrenCollection.Get()));

            SetPtrValue(m_tpItemPeerStorage, std::move(spNewChildrenCollection));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ItemsControlAutomationPeer::OnCreateItemAutomationPeerImpl(
    _In_ IInspectable* /* item */,
    _Outptr_ xaml_automation_peers::IItemAutomationPeer** /* returnValue */)
{
    RRETURN(S_OK);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::OnCreateItemAutomationPeerProtected(
    _In_ IInspectable* pItem,
    _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppReturnValue)
{
    *ppReturnValue = nullptr;
    ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spItemPeer;
    IFC_RETURN(ItemsControlAutomationPeerGenerated::OnCreateItemAutomationPeerProtected(pItem, &spItemPeer));

    if (spItemPeer)
    {
        BOOLEAN isGrouping = false;
        ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
        IFC_RETURN(get_Owner(&spItemsControlAsUIE));
        auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();

        // ItemAutomationPeerFactory sets this ItemsControlAP as parent of the ItemAutomationPeer. This is
        // not correct in case of grouping so we need to ensure, we set the correct parent while grouping.
        // This is also done in GetChildrenCore calls but that triggers during top-down. A lot of times
        // narrator directly interacts with UI via hit-testing or focus tracking which triggers bottom-up
        // approach. Here we correct the tree heirarchy early enough for bottom-up approach.
        IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));
        if (isGrouping)
        {
            ctl::ComPtr<xaml::IUIElement> spItemsContainer;
            IFC_RETURN(spItemPeer.Cast<ItemAutomationPeer>()->GetContainer(&spItemsContainer));

            ctl::ComPtr<xaml_controls::IPanel> spPanel;
            IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_ItemsPanelRoot(&spPanel));
            auto spItemsHostPanelModernCollection = spPanel.AsOrNull<IModernCollectionBasePanel>();

            // We are scoping this change to only modern panels as they are the recommended and default panels used for grouping.
            if (spItemsHostPanelModernCollection && spItemsContainer)
            {
                auto spItemsContainerAsDO = spItemsContainer.AsOrNull<IDependencyObject>();
                INT indexContainer = -1;

                IFC_RETURN(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->IndexFromContainer(spItemsContainerAsDO.Get(), &indexContainer));

                if (indexContainer > -1)
                {
                    INT32 indexGroup = -1;
                    ctl::ComPtr<IDependencyObject> spHeaderElementAsDO;

                    IFC_RETURN(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromItemIndex(indexContainer, &indexGroup, nullptr, nullptr));
                    IFC_RETURN(spItemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->HeaderFromIndex(indexGroup, &spHeaderElementAsDO));
                    auto spHeaderElementAsFE = spHeaderElementAsDO.AsOrNull<IFrameworkElement>();

                    if (spHeaderElementAsFE)
                    {
                        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spAutomationPeerForHeader;
                        IFC_RETURN(spHeaderElementAsFE.Cast<FrameworkElement>()->GetOrCreateAutomationPeer(&spAutomationPeerForHeader));

                        if (spAutomationPeerForHeader)
                        {
                            IFC_RETURN(spItemPeer.Cast<ItemAutomationPeer>()->SetParent(spAutomationPeerForHeader.Get()));
                        }
                    }
                }
            }
        }
        IFC_RETURN(spItemPeer.CopyTo(ppReturnValue));
    }
    return S_OK;
}

_Check_return_ HRESULT ItemsControlAutomationPeer::OnCollectionChanged(
    _In_ wfc::IObservableVector<IInspectable*>* pSender,
    _In_ wfc::IVectorChangedEventArgs* e)
{
    HRESULT hr = S_OK;
    wfc::CollectionChange action;
    xaml_automation_peers::IItemAutomationPeer* pPeer = NULL;
    IInspectable* pItem = NULL;

    IFC(e->get_CollectionChange(&action));

    switch (action)
    {
        case wfc::CollectionChange_ItemRemoved:
            //IFC(e->get_Index(&nIndex));
            //IFC(OnItemRemoved(nIndex));
            break;

        case wfc::CollectionChange_ItemChanged:
            //IFC(e->get_Index(&nIndex));
            //IFC(OnItemReplaced(nIndex));
            break;

        case wfc::CollectionChange_Reset:
            IFC(ReleaseItemPeerStorage(NULL));
            break;
    }

Cleanup:
    ReleaseInterface(pPeer);
    ReleaseInterface(pItem);
    RRETURN(hr);
}

// ItemContainerProvider implementation
_Check_return_ HRESULT ItemsControlAutomationPeer::FindItemByPropertyImpl(
    _In_opt_ xaml_automation::Provider::IIRawElementProviderSimple* pStartAfterAsRaw,
    _In_opt_ xaml_automation::IAutomationProperty* pProperty,
    _In_opt_ IInspectable* pValue,
    _Outptr_ xaml_automation::Provider::IIRawElementProviderSimple** ppReturnValue) noexcept
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pStartAfter = NULL;
    xaml::IUIElement* pOwner = NULL;
    wfc::IObservableVector<IInspectable*>* pItems = NULL;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    xaml_automation_peers::ISelectorItemAutomationPeer* pItemPeerAsSelectorItem = NULL;
    xaml_automation_peers::IAutomationPeer* pItemPeerAsAP = NULL;
    xaml_automation_peers::IAutomationPeer* pContainerItemPeer = NULL;
    IInspectable* pStartAfterItem = NULL;
    IInspectable* pItem = NULL;
    IInspectable* pItemFromPeer = NULL;
    AutomationPropertiesEnum ePropertiesEnum;
    bool areEqual = false;
    BOOLEAN bFound = FALSE;
    BOOLEAN bFoundInCache = FALSE;
    wrl_wrappers::HString strValue;
    wrl_wrappers::HString strPropertyValue;
    xaml_automation_peers::AutomationControlType controlTypeValue;
    xaml_automation_peers::AutomationControlType controlTypePropertyValue = xaml_automation_peers::AutomationControlType_Button;
    INT controlTypeAsInt = -1;
    UINT index = 0;
    BOOLEAN bIsSelectedValue = FALSE;
    BOOLEAN bIsSelectedPropertyValue = FALSE;
    UINT nCount = 0;

    IFCPTR(ppReturnValue);

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    // Get list of all Items from ItemsControl(this doesn't support Data virtualization, hence virtualized data item won't be part of it.)
    IFC((static_cast<ItemsControl*>(pOwner))->get_Items(&pItems));
    IFC(static_cast<ItemCollection*>(pItems)->get_Size(&nCount));

    if (pStartAfterAsRaw)
    {
        IFC(static_cast<IRawElementProviderSimple*>(pStartAfterAsRaw)->GetAutomationPeer(&pStartAfter));
    }
    // Find the index of the item from where to begin the search.
    if (pStartAfter)
    {
        IFC(ctl::do_query_interface(pItemPeer, pStartAfter));
        IFC(pItemPeer->get_Item(&pStartAfterItem));
        if (m_lastIndex > -1 && m_lastIndex < static_cast<INT>(nCount))
        {
            IFC(static_cast<ItemCollection*>(pItems)->GetAt(m_lastIndex, &pItem));
            IFC(PropertyValue::AreEqual(pStartAfterItem, pItem, &areEqual));
            ReleaseInterface(pItem);
        }
        if (!areEqual)
        {
            for (UINT i = 0; i < nCount; i++)
            {
                IFC(static_cast<ItemCollection*>(pItems)->GetAt(i, &pItem));
                IFC(PropertyValue::AreEqual(pStartAfterItem, pItem, &areEqual));
                if (areEqual)
                {
                    m_lastIndex = i;
                    break;
                }
                ReleaseInterface(pItem);
            }
        }
        ReleaseInterface(pStartAfterItem);
        ReleaseInterface(pItemPeer);
    }
    if (!areEqual)
    {
        m_lastIndex = -1;
    }

    // Fetch the Property value based on property type, which is a search criterion.
    if (pProperty)
    {
        static_cast<AutomationProperty*>(pProperty)->GetAutomationPropertiesEnum(&ePropertiesEnum);
        IFCPTR(pValue);
        switch (ePropertiesEnum)
        {
        case AutomationPropertiesEnum::AutomationIdProperty:
        case AutomationPropertiesEnum::NameProperty:
            IFC(ctl::do_get_value(*strPropertyValue.GetAddressOf(), pValue));
            break;
        case AutomationPropertiesEnum::ControlTypeProperty:
            IFC(ctl::do_get_value(controlTypeAsInt, pValue));
            controlTypePropertyValue = (xaml_automation_peers::AutomationControlType)controlTypeAsInt;
            break;
        case AutomationPropertiesEnum::IsSelectedProperty:
            IFC(ctl::do_get_value(bIsSelectedPropertyValue, pValue));
        }
    }
    else
    {
        ePropertiesEnum = AutomationPropertiesEnum::EmptyProperty;
    }

    if (nCount > 0)
    {
        for (UINT i = m_lastIndex + 1; i < nCount; i++)
        {
            IFC(static_cast<ItemCollection*>(pItems)->GetAt(i, &pItem));
            if (pItem)
            {
                IFC(GetItemPeerFromItemContainerCache(pItem, &pItemPeer, bFoundInCache));
                if (!pItemPeer)
                {
                    IFC(GetItemPeerFromChildrenCache(pItem, &pItemPeer));
                }
                if (!pItemPeer)
                {
                    IFC(OnCreateItemAutomationPeerProtected(pItem, &pItemPeer));
                }
                switch (ePropertiesEnum)
                {
                case AutomationPropertiesEnum::EmptyProperty:
                    bFound = TRUE;
                    break;
                case AutomationPropertiesEnum::AutomationIdProperty:
                    hr = static_cast<ItemAutomationPeer*>(pItemPeer)->GetAutomationId(strValue.GetAddressOf());
                    if (hr == UIA_E_ELEMENTNOTAVAILABLE)
                    {
                        hr = S_OK;
                    }
                    IFC(hr);
                    bFound = (strValue == strPropertyValue);
                    break;
                case AutomationPropertiesEnum::NameProperty:
                    hr = static_cast<ItemAutomationPeer*>(pItemPeer)->GetName(strValue.GetAddressOf());
                    if (hr == UIA_E_ELEMENTNOTAVAILABLE)
                    {
                        hr = S_OK;
                    }
                    IFC(hr);
                    bFound = (strValue == strPropertyValue);
                    break;
                case AutomationPropertiesEnum::ControlTypeProperty:
                    hr = static_cast<ItemAutomationPeer*>(pItemPeer)->GetAutomationControlType(&controlTypeValue);
                    if (hr == UIA_E_ELEMENTNOTAVAILABLE)
                    {
                        hr = S_OK;
                    }
                    IFC(hr);
                    if (controlTypeValue == controlTypePropertyValue)
                    {
                        bFound = TRUE;
                    }
                    break;
                case AutomationPropertiesEnum::IsSelectedProperty:
                    IFC(ctl::do_query_interface(pItemPeerAsSelectorItem, pItemPeer));
                    hr = static_cast<SelectorItemAutomationPeer*>(pItemPeerAsSelectorItem)->get_IsSelected(&bIsSelectedValue);
                    if (hr == UIA_E_ELEMENTNOTAVAILABLE)
                    {
                        hr = S_OK;
                    }
                    IFC(hr);
                    if (bIsSelectedValue == bIsSelectedPropertyValue)
                    {
                        bFound = TRUE;
                    }
                    break;
                default:
                    IFC(ErrorHelper::OriginateError(AgError(UIA_OPERATION_CANNOT_BE_PERFORMED)));
                    IFC(static_cast<HRESULT>(UIA_E_INVALIDOPERATION));
                    break;
                }
                if (bFound)
                {
                    BOOLEAN foundInChildrenCache = FALSE;
                    m_lastIndex = i;
                    IFC(ctl::do_query_interface(pItemPeerAsAP, pItemPeer));
                    IFC(static_cast<ItemAutomationPeer*>(pItemPeer)->GetContainerPeer(&pContainerItemPeer));
                    if (pContainerItemPeer != NULL)
                    {
                        IFC(static_cast<AutomationPeer*>(pContainerItemPeer)->put_EventsSource(pItemPeerAsAP));
                        ReleaseInterface(pContainerItemPeer);
                    }
                    if (!bFoundInCache)
                    {
                        IFCPTR(m_tpItemPeerStorageForPattern);
                        IFC(m_tpItemPeerStorageForPattern->Append(pItemPeer));
                    }

                    // Basically when Grouping is enabled we want the virtualized Peers which do not exist in realized Item's cache to have root most ItemsControl as their parent
                    // It helps them from not being orphan while it is not known which GroupItem they would belong. It's needed specially as drag and drop allows Items parent's to
                    // be shuffled easily.
                    if (m_tpItemPeerStorage)
                    {
                        IFC(m_tpItemPeerStorage->IndexOf(pItemPeer, &index, &foundInChildrenCache));
                    }

                    if (!foundInChildrenCache)
                    {
                        IFC(CoreImports::SetAutomationPeerParent(static_cast<CAutomationPeer*>(static_cast<ItemAutomationPeer*>(pItemPeer)->GetHandle()),
                            static_cast<CAutomationPeer*>(GetHandle())));
                    }

                    IFC(ProviderFromPeer(pItemPeerAsAP, ppReturnValue));
                    break;
                }
            }
            ReleaseInterface(pItem);
            ReleaseInterface(pItemPeer);
        }
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pItems);
    ReleaseInterface(pItemPeer);
    ReleaseInterface(pItemPeerAsSelectorItem);
    ReleaseInterface(pItemPeerAsAP);
    ReleaseInterface(pContainerItemPeer);
    ReleaseInterface(pStartAfter);
    ReleaseInterface(pStartAfterItem);
    ReleaseInterface(pItem);
    ReleaseInterface(pItemFromPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::GetItemPeerFromItemContainerCache(
    _In_ IInspectable* pItem,
    _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppItemPeer,
    BOOLEAN &bFoundInCache)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    IInspectable* pItemFromPeer = NULL;
    UINT nCount = 0;

    *ppItemPeer = NULL;
    if (!m_tpItemPeerStorageForPattern)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>> spItemPeerStorageForPattern;
        IFC(ctl::make(&spItemPeerStorageForPattern));
        SetPtrValue(m_tpItemPeerStorageForPattern, std::move(spItemPeerStorageForPattern));

        bFoundInCache = FALSE;
        goto Cleanup;
    }

    IFC(m_tpItemPeerStorageForPattern->get_Size(&nCount));
    if (nCount > 0)
    {
        for (UINT i = 0; i < nCount; i++)
        {
            IFC(m_tpItemPeerStorageForPattern->GetAt(i, &pItemPeer));
            if (pItemPeer)
            {
                bool areEqual = false;
                IFC(static_cast<ItemAutomationPeer*>(pItemPeer)->get_Item(&pItemFromPeer));
                IFC(PropertyValue::AreEqual(pItemFromPeer, pItem, &areEqual));
                bFoundInCache = areEqual;
            }
            if (bFoundInCache)
            {
                *ppItemPeer = pItemPeer;
                pItemPeer = NULL;
                break;
            }
            ReleaseInterface(pItemFromPeer);
            ReleaseInterface(pItemPeer);
        }
    }

Cleanup:
    ReleaseInterface(pItemPeer);
    ReleaseInterface(pItemFromPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::GetItemPeerFromChildrenCache(
    _In_ IInspectable* pItem,
    _Outptr_ xaml_automation_peers::IItemAutomationPeer** ppItemPeer)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    IInspectable* pItemFromPeer = NULL;
    bool areEqual = false;
    UINT nCount = 0;

    *ppItemPeer = NULL;
    if (m_tpItemPeerStorage)
    {
        IFC(m_tpItemPeerStorage->get_Size(&nCount));
        if (nCount > 0)
        {
            for (UINT i = 0; i < nCount; i++)
            {
                IFC(m_tpItemPeerStorage->GetAt(i, &pItemPeer));
                if (pItemPeer)
                {
                    IFC(static_cast<ItemAutomationPeer*>(pItemPeer)->get_Item(&pItemFromPeer));
                    IFC(PropertyValue::AreEqual(pItemFromPeer, pItem, &areEqual));
                }
                if (areEqual)
                {
                    *ppItemPeer = pItemPeer;
                    pItemPeer = NULL;
                    break;
                }
                ReleaseInterface(pItemFromPeer);
                ReleaseInterface(pItemPeer);
            }
        }
    }

Cleanup:
    ReleaseInterface(pItemPeer);
    ReleaseInterface(pItemFromPeer);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::CreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    *returnValue = nullptr;

    if (item)
    {
        ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> itemPeer;
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> containerItemPeer;
        BOOLEAN foundInCache = FALSE;

        IFC_RETURN(GetItemPeerFromItemContainerCache(item, &itemPeer, foundInCache));
        if (!itemPeer)
        {
            IFC_RETURN(GetItemPeerFromChildrenCache(item, &itemPeer));
        }

        if (!itemPeer)
        {
            IFC_RETURN(OnCreateItemAutomationPeerProtected(item, &itemPeer));
        }

        IFCEXPECT_ASSERT_RETURN(itemPeer);

        auto itemPeerAsAP = itemPeer.AsOrNull<xaml_automation_peers::IAutomationPeer>();
        IFC_RETURN(itemPeer.Cast<ItemAutomationPeer>()->GetContainerPeer(&containerItemPeer));
        if (containerItemPeer)
        {
            IFC_RETURN(containerItemPeer.Cast<AutomationPeer>()->put_EventsSource(itemPeerAsAP.Get()));
        }

        if (!foundInCache)
        {
            ctl::ComPtr<xaml::IUIElement> itemsControlAsUIE;
            IFC_RETURN(get_Owner(&itemsControlAsUIE));
            auto itemsControl = itemsControlAsUIE.AsOrNull<IItemsControl>();
            BOOLEAN isGrouping = FALSE;

            IFC_RETURN(itemsControl.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));
            if (isGrouping && !containerItemPeer)
            {
                // We are in a grouped scenario but the panel is not created yet or the container
                // is not realized. In this case we could potentially have a peer with
                // incorrect parent (pointing to the itemscontrol instead of a header automation peer).
                // Dont cache if this is the case.
            }
            else
            {
                IFCPTR_RETURN(m_tpItemPeerStorageForPattern);
                IFC_RETURN(m_tpItemPeerStorageForPattern->Append(itemPeer.Get()));
            }
        }

        IFC_RETURN(itemPeer.MoveTo(returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT ItemsControlAutomationPeer::GetPositionInSetHelper(_In_ xaml::IUIElement* pItemContainer, _Out_ INT* returnValue)
{
    *returnValue = -1;
    ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
    IFC_RETURN(get_Owner(&spItemsControlAsUIE));
    auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();

    INT itemIndex = -1;
    auto spContainerAsDO = ctl::ComPtr<xaml::IUIElement>(pItemContainer).AsOrNull<IDependencyObject>();
    ASSERT(spContainerAsDO);

    // Index of the item returned here is actual position of the item in the complete list of "realized data"
    // that also includes "virtualized containers". If it's a group list we will use this index to get group
    // info that will provide the position within the group.
    IFC_RETURN(spItemsControl.Cast<ItemsControl>()->IndexFromContainer(spContainerAsDO.Get(), &itemIndex));

    if (itemIndex != -1)
    {
        BOOLEAN isGrouping = false;
        IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));

        if (isGrouping)
        {
            ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
            IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHostPanel));
            auto spModernPanel = spItemsHostPanel.AsOrNull<IModernCollectionBasePanel>();

            if (spModernPanel)
            {
                INT indexOfGroup = -1;
                INT indexInsideGroup = -1;
                INT countInGroup = -1;

                IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromItemIndex(itemIndex, &indexOfGroup, &indexInsideGroup, &countInGroup));
                itemIndex = indexInsideGroup;
            }
            else
            {
                // set to -2 so eventually it's default as we do not support old style grouping atm
                itemIndex = -2;
            }
        }

        // As index is 0 based and position is 1 based
        *returnValue = itemIndex + 1;
    }

    return S_OK;
}
_Check_return_ HRESULT ItemsControlAutomationPeer::GetSizeOfSetHelper(_In_ xaml::IUIElement* pItemContainer, _Out_ INT* returnValue)
{
    *returnValue = -1;
    ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
    IFC_RETURN(get_Owner(&spItemsControlAsUIE));
    auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();

    UINT count = -1;
    BOOLEAN isGrouping = false;
    IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_IsGrouping(&isGrouping));

    // Count of the ItemsCollection is actual size of list of "realized data".
    if (!isGrouping)
    {
        ctl::ComPtr<wfc::IObservableVector<IInspectable*>> spItems;
        IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_Items(&spItems));
        ASSERT(spItems);
        IFC_RETURN(spItems.Cast<ItemCollection>()->get_Size(&count));
        *returnValue = static_cast<INT>(count);
    }
    else
    {
        // In case of grouping we will find the index of this item and use that index to find group information.
        // group infornmation will provide us the count of items within the group this item belongs to.
        ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
        IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHostPanel));
        auto spModernPanel = spItemsHostPanel.AsOrNull<IModernCollectionBasePanel>();

        if (spModernPanel)
        {
            INT itemIndex = -1;
            INT indexOfGroup = -1;
            INT indexInsideGroup = -1;
            INT countInGroup = -1;
            auto spContainerAsDO = ctl::ComPtr<xaml::IUIElement>(pItemContainer).AsOrNull<IDependencyObject>();
            ASSERT(spContainerAsDO);

            IFC_RETURN(spItemsControl.Cast<ItemsControl>()->IndexFromContainer(spContainerAsDO.Get(), &itemIndex))
            IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromItemIndex(itemIndex, &indexOfGroup, &indexInsideGroup, &countInGroup));

            *returnValue = countInGroup;
        }
        else
        {
            // do nothing, we do not support old style grouping atm, default value is correct value so Narrator does the calculation.
        }
    }

    return S_OK;
}

// This is to keep the Items peers in the storage updated and cleans any references which is no longer used by UIA client.
_Check_return_ HRESULT ItemsControlAutomationPeer::RemoveItemAutomationPeerFromStorage(
    _In_opt_ ItemAutomationPeer* pItemPeer,
    bool forceRemoveItemPeer)
{
    HRESULT hr = S_OK;
    UINT index = 0;
    BOOLEAN found = FALSE;
    bool removeItemAP = true;

    // Releasing our peer may cause this ItemsControlAutomationPeer to be destroyed. Make sure that
    // we don't get destroyed until we're done with the following work, to avoid accessing members
    // on a deleted object.
    ctl::addref_interface(this);

    // Do not remove the ItemAutomationPeer if the Item's container is a non-control and
    // has a focusable child element. ItemAutomationPeer's UIA wrapper can be unreferenced
    // from the UIA client but Item's focusable child AutomationPeer still can be accessible
    // with the item navigation.
    if (pItemPeer && !forceRemoveItemPeer)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> containerAP;

        IFC(pItemPeer->GetContainerPeer(&containerAP));

        if (containerAP)
        {
            ctl::ComPtr<xaml_automation_peers::IListViewItemAutomationPeer> listViewItemAutomationPeer = containerAP.AsOrNull<xaml_automation_peers::IListViewItemAutomationPeer>();

            if (listViewItemAutomationPeer)
            {
                BOOLEAN isControlElement = true;

                IFC(containerAP.AsOrNull<AutomationPeer>()->IsControlElement(&isControlElement));

                if (!isControlElement)
                {
                    ctl::ComPtr<xaml::IUIElement> ownerElement;

                    IFC(containerAP.AsOrNull<FrameworkElementAutomationPeer>()->get_Owner(&ownerElement));

                    if (ownerElement)
                    {
                        xref_ptr<CDependencyObject> focusableItemChildStop;
                        IFC(CoreImports::FocusManager_GetFirstFocusableElement((ownerElement.Cast<UIElement>())->GetHandle(), focusableItemChildStop.ReleaseAndGetAddressOf()));
                        if (focusableItemChildStop != nullptr)
                        {
                            IFC(listViewItemAutomationPeer.AsOrNull<ListViewItemAutomationPeer>()->SetRemovableItemAutomationPeer(pItemPeer));
                            removeItemAP = false;
                        }
                    }
                }
            }
        }
    }

    if (m_tpItemPeerStorageForPattern && removeItemAP)
    {
        IFC(m_tpItemPeerStorageForPattern->IndexOf(pItemPeer, &index, &found));
        if (found)
        {
            found = FALSE;
            if (m_tpItemPeerStorage)
            {
                UINT childIndex = 0;
                IFC(m_tpItemPeerStorage->IndexOf(pItemPeer, &childIndex, &found));
            }

            // if found is TRUE, that means ItemsControlAP (who manages life of ItemAP) still refering to the Peer.
            // so don't release the Item yet. The below code mainly excutes, if selection happens before GetChildrenCore
            // or FindItemByProperty finds something which is not part of Automation Tree yet, like virtualized Item.
            if (!found && pItemPeer)
            {
                pItemPeer->ReleaseEventsSourceLink();
                pItemPeer->ReleaseItemAndParent();
            }
            IFC(m_tpItemPeerStorageForPattern->RemoveAt(index));
        }
    }

Cleanup:
    ctl::release_interface_nonull(this);
    RRETURN(hr);
}

// We have to release the Item from ItemAPs if only m_pItemPeerStorage was referring to ItemAPs in this object.
// so as we break the circular chain, but we don't want to release those items if we are referring to them otherwise
// within this object. So we basically call this methode when we want to release m_pItemPeerStorage. pItemPeerStorage
// is the current storage in case we are updating m_pItemPeerStorage and there are overlaps.
_Check_return_ HRESULT ItemsControlAutomationPeer::ReleaseItemPeerStorage(
        _In_ wfc::IVector<xaml_automation_peers::ItemAutomationPeer*>* pItemPeerStorage)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    UINT index = 0;
    BOOLEAN found = FALSE;
    UINT nCount = 0;

    // Releasing our peer may cause this ItemsControlAutomationPeer to be destroyed. Make sure that
    // we don't get destroyed until we're done with the following work, to avoid accessing members
    // on a deleted object.
    ctl::addref_interface(this);

    if(m_tpItemPeerStorage)
    {
        IFC(m_tpItemPeerStorage->get_Size(&nCount));
        if(nCount > 0)
        {
            for(UINT i = 0; i < nCount; i++)
            {
                IFC(m_tpItemPeerStorage->GetAt(i, &pItemPeer));
                if(pItemPeerStorage)
                {
                    IFC(pItemPeerStorage->IndexOf(pItemPeer, &index, &found));
                }
                if(!found && m_tpItemPeerStorageForPattern)
                {
                    IFC(m_tpItemPeerStorageForPattern->IndexOf(pItemPeer, &index, &found));
                }
                if(!found)
                {
                    static_cast<ItemAutomationPeer*>(pItemPeer)->ReleaseEventsSourceLink();
                    static_cast<ItemAutomationPeer*>(pItemPeer)->ReleaseItemAndParent();
                }
                found = FALSE;
                ReleaseInterface(pItemPeer);
            }
        }
    }

Cleanup:
    ReleaseInterface(pItemPeer);
    m_tpItemPeerStorage.Clear();
    ctl::release_interface_nonull(this);
    RRETURN(hr);
}

_Check_return_ HRESULT ItemsControlAutomationPeer::AddItemAutomationPeerToItemPeerStorage(_In_ ItemAutomationPeer* pItemPeer)
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    UINT index = 0;
    BOOLEAN found = FALSE;

    IFCPTR(pItemPeer);

    if(!m_tpItemPeerStorage)
    {
        ctl::ComPtr<TrackerCollection<xaml_automation_peers::ItemAutomationPeer*>> spItemPeerStorage;
        IFC(ctl::make(&spItemPeerStorage));
        SetPtrValue( m_tpItemPeerStorage, std::move(spItemPeerStorage));
    }

    IFCEXPECT_ASSERT(m_tpItemPeerStorage);
    IFC(m_tpItemPeerStorage->IndexOf(pItemPeer, &index, &found));

    if (!found)
    {
        IFC(m_tpItemPeerStorage->Append(pItemPeer));
    }

Cleanup:
    RRETURN(S_OK);
}

// Finally generating and setting the EventsSource for pItemContainerAP.
_Check_return_ HRESULT ItemsControlAutomationPeer::GenerateEventsSourceForContainerItemPeer(_In_ xaml_automation_peers::IFrameworkElementAutomationPeer* pItemContainerAP)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IAutomationPeer* pOldEventsSource = NULL;
    xaml_automation_peers::IItemAutomationPeer* pOldItemPeer = NULL;
    xaml_automation_peers::IItemAutomationPeer* pItemPeer = NULL;
    xaml_automation_peers::IAutomationPeer* pItemPeerAsAP = NULL;
    xaml::IUIElement* pItemContainer = NULL;
    xaml::IUIElement* pOwner = NULL;
    IDependencyObject* pItemContainerAsDO = NULL;
    IInspectable* pItem = NULL;
    IInspectable* pOldItem = NULL;
    BOOLEAN foundInCache = FALSE;

    IFC(get_Owner(&pOwner));
    IFCPTR(pOwner);

    IFCPTR(pItemContainerAP);
    IFC(pItemContainerAP->get_Owner(&pItemContainer));
    IFC(ctl::do_query_interface(pItemContainerAsDO, pItemContainer));
    IFCEXPECT(pItemContainerAsDO);
    IFC((static_cast<ItemsControl*>(pOwner))->GetItemOrContainerFromContainer(pItemContainerAsDO, &pItem));

    IFC(static_cast<FrameworkElementAutomationPeer*>(pItemContainerAP)->get_EventsSource(&pOldEventsSource));
    if(pOldEventsSource)
    {
        pOldItemPeer = ctl::query_interface<xaml_automation_peers::IItemAutomationPeer>(pOldEventsSource);
        if(pOldItemPeer)
        {
            IFC(pOldItemPeer->get_Item(&pOldItem));
            if(pOldItem == pItem)
            {
                goto Cleanup;
            }
        }
    }

    if(pItem != NULL)
    {
        IFC(GetItemPeerFromItemContainerCache(pItem, &pItemPeer, foundInCache));
        if(!pItemPeer)
        {
            IFC(GetItemPeerFromChildrenCache(pItem, &pItemPeer));
        }
        if(!pItemPeer)
        {
            IFC(OnCreateItemAutomationPeerProtected(pItem, &pItemPeer));
        }

        IFCEXPECT_ASSERT(pItemPeer);

        IFC(ctl::do_query_interface(pItemPeerAsAP, pItemPeer));
        IFC(static_cast<FrameworkElementAutomationPeer*>(pItemContainerAP)->put_EventsSource(pItemPeerAsAP));
        if(!foundInCache)
        {
            IFCPTR(m_tpItemPeerStorageForPattern);
            IFC(m_tpItemPeerStorageForPattern->Append(pItemPeer));
        }
    }

Cleanup:
    ReleaseInterface(pOwner);
    ReleaseInterface(pOldEventsSource);
    ReleaseInterface(pItemPeer);
    ReleaseInterface(pOldItemPeer);
    ReleaseInterface(pItemPeerAsAP);
    ReleaseInterface(pItemContainer);
    ReleaseInterface(pItemContainerAsDO);
    ReleaseInterface(pItem);
    ReleaseInterface(pOldItem);
    RRETURN(hr);
}

// Static function to generate EventsSource for container APs of List Items, we can basically call this from AutomationPeer itself instead of calling from each different ItemPeers.
_Check_return_ HRESULT ItemsControlAutomationPeer::GenerateAutomationPeerEventsSourceStatic(_In_ xaml_automation_peers::IFrameworkElementAutomationPeer* pItemContainerAP, _In_ xaml_automation_peers::IAutomationPeer* pAPParent)
{
    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParentItemsControl;

    spParentItemsControl.Attach(ctl::query_interface<xaml_automation_peers::IItemsControlAutomationPeer>(pAPParent));
    if (!spParentItemsControl)
    {
        // walking up the chain finds SV as the parent of the item that has an AP but we hide SV from the UIA tree when its used
        // within template of ItemsControl. One way is not create AP for SV at all when its used within template, but we do use
        // SVAP as an object that provides scroll pattern for ItemsControl's AP. While generating EventsSource for the ITemContainer
        // We need the actual ItemsControlAP hence we get SV's templated parent if it was SV that was originally passed here.
        ctl::ComPtr<xaml_automation_peers::IScrollViewerAutomationPeer> spScrollViewerAP;
        spScrollViewerAP.Attach(ctl::query_interface<xaml_automation_peers::IScrollViewerAutomationPeer>(pAPParent));
        if (spScrollViewerAP)
        {
            // First check if SV already has its EventsSource set as during top down approach we end up setting SV's EventsSource
            // to be ItemsControl AP and that's all we need. If not, then we generate ItemsControlAP and set that as SVAP's
            // EventsSource.
            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spEventsSourceAP;
            IFC_RETURN(spScrollViewerAP.Cast<ScrollViewerAutomationPeer>()->get_EventsSource(&spEventsSourceAP));
            spParentItemsControl = spEventsSourceAP.AsOrNull<IItemsControlAutomationPeer>();
            if (!spParentItemsControl)
            {
                ctl::ComPtr<xaml::IUIElement> spScrollViewerAsUie;
                ctl::ComPtr<IFrameworkElement> spScrollViewerAsIFE;
                ctl::ComPtr<DependencyObject> spTemplatedParent;
                ctl::ComPtr<IItemsControl> spTemplatedParentAsIC;

                IFC_RETURN(spScrollViewerAP.Cast<ScrollViewerAutomationPeer>()->get_Owner(&spScrollViewerAsUie));
                spScrollViewerAsIFE = spScrollViewerAsUie.AsOrNull<IFrameworkElement>();
                IFC_RETURN(spScrollViewerAsIFE.Cast<FrameworkElement>()->get_TemplatedParent(&spTemplatedParent));
                spTemplatedParentAsIC = spTemplatedParent.AsOrNull<IItemsControl>();
                if (spTemplatedParentAsIC)
                {
                    IFC_RETURN(spTemplatedParentAsIC.Cast<ItemsControl>()->GetOrCreateAutomationPeer(&spEventsSourceAP));
                    if (spEventsSourceAP)
                    {
                        IFC_RETURN(spScrollViewerAP.Cast<ScrollViewerAutomationPeer>()->put_EventsSource(spEventsSourceAP.Get()));
                        spParentItemsControl = spEventsSourceAP.AsOrNull<IItemsControlAutomationPeer>();
                    }
                }
            }
        }
        else
        {
            ctl::ComPtr<xaml_automation_peers::IListViewBaseHeaderItemAutomationPeer> spHeaderItemParent;
            spHeaderItemParent.Attach(ctl::query_interface<xaml_automation_peers::IListViewBaseHeaderItemAutomationPeer>(pAPParent));
            if (spHeaderItemParent)
            {
                IFC_RETURN(spHeaderItemParent.Cast<ListViewBaseHeaderItemAutomationPeer>()->get_ParentItemsControlAutomationPeer(&spParentItemsControl));
            }
            else
            {
                ctl::ComPtr<xaml_automation_peers::IGroupItemAutomationPeer> spGroupItemParent;
                spGroupItemParent.Attach(ctl::query_interface<xaml_automation_peers::IGroupItemAutomationPeer>(pAPParent));
                if (spGroupItemParent)
                {
                    IFC_RETURN(spGroupItemParent.Cast<GroupItemAutomationPeer>()->get_ParentItemsControlAutomationPeer(&spParentItemsControl));
                }
            }
        }
    }

    if (spParentItemsControl)
    {
        IFC_RETURN(spParentItemsControl.Cast<ItemsControlAutomationPeer>()->GenerateEventsSourceForContainerItemPeer(pItemContainerAP));
    }

    return S_OK;
}
