// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseHeaderItemAutomationPeer.g.h"
#include "ListViewBase.g.h"
#include "ModernCollectionBasePanel.g.h"
#include "ItemsControlAutomationPeer.g.h"
#include "ItemAutomationPeer.g.h"
#include <XamlOneCoreTransforms.h>
#include "RootScale.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListViewBaseHeaderItem* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListViewBaseHeaderItemAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IListViewBaseHeaderItemAutomationPeer* pInstance = nullptr;
    IInspectable* pInner = nullptr;
    xaml::IUIElement* ownerAsUIE = nullptr;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == nullptr || ppInner != nullptr);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<ListViewBaseHeaderItem*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<ListViewBaseHeaderItemAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = nullptr;
    }

    *ppInstance = pInstance;
    pInstance = nullptr;

Cleanup:
    ReleaseInterface(ownerAsUIE);
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseHeaderItemAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    IFCPTR_RETURN(returnValue);

    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> aPChildren;
    IFC_RETURN(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&aPChildren));

    ctl::ComPtr<xaml::IUIElement> owner;
    ctl::ComPtr<IDependencyObject> ownerAsDO;
    ctl::ComPtr<xaml_controls::IListViewBase> parentListViewBase;
    IFC_RETURN(get_Owner(&owner));
    IFCPTR_RETURN(owner.Get());
    IFC_RETURN(owner.As(&ownerAsDO));
    IFC_RETURN(owner.Cast<ListViewBaseHeaderItem>()->GetParent(&parentListViewBase));

    if (parentListViewBase)
    {
        ctl::ComPtr<xaml_automation_peers::IAutomationPeer> itemsControlAutomationPeerAsAP;
        ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> itemsControlAutomationPeer;
        ctl::ComPtr<xaml_controls::IPanel> itemsHostPanel;
        ctl::ComPtr<IModernCollectionBasePanel> itemsHostPanelModernCollection;

        IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->GetOrCreateAutomationPeer(&itemsControlAutomationPeerAsAP));
        IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->get_ItemsHost(&itemsHostPanel));
        if (itemsControlAutomationPeerAsAP && itemsHostPanel)
        {
            // Add the children of the header to the automation tree
            IFC_RETURN(FrameworkElementAutomationPeer::GetAutomationPeerChildren(owner.Get(), aPChildren.Get()));

            // This is a group, add the group items to the automation tree
            IFC_RETURN(itemsControlAutomationPeerAsAP.As(&itemsControlAutomationPeer));
            IFC_RETURN(itemsHostPanel.As(&itemsHostPanelModernCollection));

            INT indexHeader = 0;
            INT32 indexCacheStart = 0;
            INT32 indexCacheEnd = 0;
            INT32 indexStartItem = 0;
            INT32 countItemInGroup = 0;
            IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->IndexFromHeader(ownerAsDO.Get(), FALSE /*excludeHiddenEmptyGroups*/, &indexHeader));
            IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->GetGroupInformationFromGroupIndex(indexHeader, &indexStartItem, &countItemInGroup));
            IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->get_FirstCacheIndexBase(&indexCacheStart));
            IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&indexCacheEnd));
            IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->get_LastCacheIndexBase(&indexCacheEnd));

            INT32 indexRealizedStart = (indexCacheStart > indexStartItem) ? indexCacheStart : indexStartItem;
            INT32 indexRealizedEnd = (indexCacheEnd < (indexStartItem + countItemInGroup - 1) ) ? indexCacheEnd : (indexStartItem + countItemInGroup - 1);

            ctl::ComPtr<IDependencyObject> itemContainerAsDO;

            for (int indexContainer = indexRealizedStart; indexContainer <= indexRealizedEnd ; ++indexContainer)
            {
                IFC_RETURN(itemsHostPanelModernCollection.Cast<ModernCollectionBasePanel>()->ContainerFromIndex(indexContainer, &itemContainerAsDO));
                if (itemContainerAsDO)
                {
                    ctl::ComPtr<IInspectable> itemAsInspectable;
                    IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->GetItemOrContainerFromContainer(itemContainerAsDO.Get(), &itemAsInspectable));
                    if (itemAsInspectable)
                    {
                        ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> itemPeer;
                        BOOLEAN foundInChildrenCache = FALSE;
                        IFC_RETURN(itemsControlAutomationPeerAsAP.Cast<ItemsControlAutomationPeer>()->GetItemPeerFromChildrenCache(itemAsInspectable.Get(), &itemPeer));
                        if (!itemPeer)
                        {
                            BOOLEAN foundInCache = FALSE;
                            IFC_RETURN(itemsControlAutomationPeerAsAP.Cast<ItemsControlAutomationPeer>()->GetItemPeerFromItemContainerCache(itemAsInspectable.Get(), &itemPeer, foundInCache));
                        }
                        else
                        {
                            foundInChildrenCache = TRUE;
                        }
                        if (!itemPeer)
                        {
                            IFC_RETURN(itemsControlAutomationPeerAsAP.Cast<ItemsControlAutomationPeer>()->OnCreateItemAutomationPeerProtected(itemAsInspectable.Get(), &itemPeer));
                        }
                        if (itemPeer)
                        {
                            ctl::ComPtr<xaml_automation_peers::IAutomationPeer> containerItemPeer;
                            IFC_RETURN(itemPeer.Cast<ItemAutomationPeer>()->GetContainerPeer(&containerItemPeer));
                            if (containerItemPeer)
                            {
                                ctl::ComPtr<xaml_automation_peers::IAutomationPeer> itemPeerAsAP;
                                IFC_RETURN(itemPeer.As(&itemPeerAsAP));
                                IFC_RETURN(containerItemPeer.Cast<AutomationPeer>()->put_EventsSource(itemPeerAsAP.Get()));
                                IFC_RETURN(aPChildren->Append(itemPeerAsAP.Get()));
                                if (!foundInChildrenCache)
                                {
                                    IFC_RETURN(itemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->AddItemAutomationPeerToItemPeerStorage(itemPeer.Cast<ItemAutomationPeer>()));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    IFC_RETURN(aPChildren.CopyTo(returnValue));
    return S_OK;
}

IFACEMETHODIMP ListViewBaseHeaderItemAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"ListViewBaseHeaderItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP ListViewBaseHeaderItemAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    *pReturnValue = xaml_automation_peers::AutomationControlType_Group;
    RRETURN(S_OK);
}

IFACEMETHODIMP ListViewBaseHeaderItemAutomationPeer::GetBoundingRectangleCore(_Out_ wf::Rect* returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwner;
    IFCPTR(returnValue);
    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    wf::Rect groupBounds;
    IFC(spOwner.Cast<ListViewBaseHeaderItem>()->GetGroupBounds(&groupBounds));

    XRECTF rect = ConvertRectToXRECTF(groupBounds);
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // In OneCoreTransforms mode, GetGroupBounds returns logical pixels so we must convert to RasterizedClient
        const float scale = RootScale::GetRasterizationScaleForElement(static_cast<CAutomationPeer*>(GetHandle())->GetRootNoRef());
        const auto logicalRect = rect;
        const auto physicalRect = logicalRect * scale;
        rect = physicalRect;
    }
    *returnValue = ConvertXRECTFToRect(rect);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeer::GetPositionInSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(ListViewBaseHeaderItemAutomationPeerGenerated::GetPositionInSetCoreImpl(pReturnValue));

    // if it still is default value, calculate it ourselves.
    if (*pReturnValue == -1)
    {
        ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer>  spItemsControlAutomationPeer;
        IFC_RETURN(get_ParentItemsControlAutomationPeer(&spItemsControlAutomationPeer));

        if (spItemsControlAutomationPeer)
        {
            ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
            IFC_RETURN(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->get_Owner(&spItemsControlAsUIE));
            auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();

            ctl::ComPtr<xaml::IUIElement> spHeaderItemAsUIE;
            IFC_RETURN(get_Owner(&spHeaderItemAsUIE));

            INT headerIndex = -1;
            auto spHeaderItemAsDO = spHeaderItemAsUIE.AsOrNull<IDependencyObject>();
            ASSERT(spHeaderItemAsDO);

            // Index of the header itself is position of the header within the list
            // Headers of empty groups are not counted when ModernCollectionBasePanel::m_hidesIfEmpty is True.
            IFC_RETURN(spItemsControl.Cast<ItemsControl>()->IndexFromHeader(spHeaderItemAsDO.Get(), TRUE /*excludeHiddenEmptyGroups*/, &headerIndex));
            *pReturnValue = headerIndex + 1;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeer::GetSizeOfSetCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    *pReturnValue = -1;

    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(ListViewBaseHeaderItemAutomationPeerGenerated::GetSizeOfSetCoreImpl(pReturnValue));

    // if it still is default value, calculate it ourselves.
    if (*pReturnValue == -1)
    {
        ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer>  spItemsControlAutomationPeer;
        IFC_RETURN(get_ParentItemsControlAutomationPeer(&spItemsControlAutomationPeer));

        if (spItemsControlAutomationPeer)
        {
            ctl::ComPtr<xaml::IUIElement> spItemsControlAsUIE;
            IFC_RETURN(spItemsControlAutomationPeer.Cast<ItemsControlAutomationPeer>()->get_Owner(&spItemsControlAsUIE));
            auto spItemsControl = spItemsControlAsUIE.AsOrNull<IItemsControl>();

            ctl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
            IFC_RETURN(spItemsControl.Cast<ItemsControl>()->get_ItemsHost(&spItemsHostPanel));
            auto spModernPanel = spItemsHostPanel.AsOrNull<IModernCollectionBasePanel>();

            if (spModernPanel)
            {
                IFC_RETURN(spModernPanel.Cast<ModernCollectionBasePanel>()->GetTotalGroupCountImpl(pReturnValue));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeer::GetLevelCoreImpl(_Out_ INT* pReturnValue)
{
    IFCPTR_RETURN(pReturnValue);
    // First retrieve any valid value being directly set on the container, that value will get precedence.
    IFC_RETURN(ListViewBaseHeaderItemAutomationPeerGenerated::GetLevelCoreImpl(pReturnValue));

    // if it still is default value, calculate it ourselves.
    if (*pReturnValue == -1)
    {
        // Header are always first level elements within list.
        *pReturnValue = 1;
    }
    return S_OK;
}

_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeer::ShouldSupportInvokePattern(
    _In_ ListViewBaseHeaderItem* listViewbaseHeaderItem,
    _Out_ bool* supportInvokePatternOut)
{
    *supportInvokePatternOut = false;

    ctl::ComPtr<IListViewBase> parentListViewBase;
    IFC_RETURN(listViewbaseHeaderItem->GetParent(&parentListViewBase));

    if (parentListViewBase)
    {
        // Support invoke pattern if ListViewBaseHeaderItem is in a Semantic Zoom's zoomed-in view.
        // Invoke will switch to zoomed-out view.
        ctl::ComPtr<ISemanticZoom> semanticZoomOwner;
        IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->get_SemanticZoomOwner(&semanticZoomOwner));

        if (semanticZoomOwner)
        {
            BOOLEAN canChangeViews = FALSE;
            IFC_RETURN(semanticZoomOwner->get_CanChangeViews(&canChangeViews));

            if (canChangeViews)
            {
                BOOLEAN isZoomedInView = FALSE;
                IFC_RETURN(parentListViewBase.Cast<ListViewBase>()->get_IsZoomedInView(&isZoomedInView));

                if (isZoomedInView)
                {
                    *supportInvokePatternOut = true;
                }
            }
        }
    }

    return S_OK;
}

IFACEMETHODIMP ListViewBaseHeaderItemAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** patternOut)
{
    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        ctl::ComPtr<xaml::IUIElement> owner;
        IFC_RETURN(get_Owner(&owner));

        bool supportInvokePattern = false;
        IFC_RETURN(ShouldSupportInvokePattern(owner.Cast<ListViewBaseHeaderItem>(), &supportInvokePattern));

        if (supportInvokePattern)
        {
            // Create adapter to perform Invoke action, if needed
            if (!m_itemInvokeAdapter)
            {
                ctl::ComPtr<ItemInvokeAdapter> itemInvokeAdapter;

                IFC_RETURN(ctl::make<ItemInvokeAdapter>(&itemInvokeAdapter));
                IFCPTR_RETURN(itemInvokeAdapter.Get());

                m_itemInvokeAdapter = itemInvokeAdapter;
                IFC_RETURN(m_itemInvokeAdapter->put_Owner(this));
            }
            IFC_RETURN(m_itemInvokeAdapter.CopyTo(patternOut));
        }
    }
    else
    {
        IFC_RETURN(ListViewBaseHeaderItemAutomationPeerGenerated::GetPatternCore(patternInterface, patternOut));
    }

    return S_OK;
}

// Gets the AP for Parent ItemsControl for this HeaderItem if there is one.
_Check_return_ HRESULT ListViewBaseHeaderItemAutomationPeer::get_ParentItemsControlAutomationPeer(_Out_ xaml_automation_peers::IItemsControlAutomationPeer** ppParentItemsControl)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<xaml_controls::IItemsControl> spItemsControl;
    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spItemsControlAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> spItemsControlAutomationPeerAsAP;

    IFCPTR(ppParentItemsControl);
    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner);

    IFC(ItemsControl::ItemsControlFromItemContainer(spOwner.Cast<UIElement>(), &spItemsControl));
    if (spItemsControl)
    {
        IFC(spItemsControl.Cast<ItemsControl>()->GetOrCreateAutomationPeer(&spItemsControlAutomationPeerAsAP));
        IFC(spItemsControlAutomationPeerAsAP.As(&spItemsControlAutomationPeer));
        *ppParentItemsControl = spItemsControlAutomationPeer.Detach();
    }

Cleanup:
    RRETURN(hr);
}
