// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewItemDataAutomationPeer.g.h"
#include "GridViewAutomationPeer.g.h"
#include "ListViewBaseitem.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GridViewItemDataAutomationPeerFactory::CreateInstanceWithParentAndItemImpl(
    _In_ IInspectable* pItem,
    _In_ xaml_automation_peers::IGridViewAutomationPeer* pParent,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IGridViewItemDataAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IGridViewItemDataAutomationPeer> spInstance = NULL;
    ctl::ComPtr<IInspectable> spInner = NULL;
    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> spParentAsItemsControl = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pParent);
    
    IFC(ctl::do_query_interface(spParentAsItemsControl, pParent));
    
    IFC(ActivateInstance(pOuter,
            static_cast<GridViewAutomationPeer*>(pParent)->GetHandle(),
            &spInner));
    IFC(spInner.As<xaml_automation_peers::IGridViewItemDataAutomationPeer>(&spInstance))
    IFC(spInstance.Cast<GridViewItemDataAutomationPeer>()->put_Parent(spParentAsItemsControl.Get()));
    IFC(spInstance.Cast<GridViewItemDataAutomationPeer>()->put_Item(pItem));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the GridViewItemDataAutomationPeer class.
GridViewItemDataAutomationPeer::GridViewItemDataAutomationPeer()
{
}

// Deconstructor
GridViewItemDataAutomationPeer::~GridViewItemDataAutomationPeer()
{
}

IFACEMETHODIMP GridViewItemDataAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"GridViewItem")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT GridViewItemDataAutomationPeer::ScrollIntoViewImpl()
{
    ctl::ComPtr<xaml_automation_peers::IItemsControlAutomationPeer> itemsControlPeer;
    ctl::ComPtr<IUIElement> gridView;
 
    IFC_RETURN(get_ItemsControlAutomationPeer(&itemsControlPeer));
    IFC_RETURN(itemsControlPeer.Cast<ItemsControlAutomationPeer>()->get_Owner(&gridView));
    if (gridView)
    {
        bool isScrollable = false;
        IFC_RETURN(gridView.Cast<ListViewBase>()->IsScrollable(&isScrollable));

        // If the GridView's ScrollViewer can scroll, a scroll into view is sufficient to bring it into view. If 
        // it is not (nested scroll viewer case), then a bring into view which bubbles up the tree 
        // is required to bring the item into view.
        if (isScrollable)
        {
            IFC_RETURN(ScrollIntoViewCommon());
        }
        else
        {
            // bring into view
            ctl::ComPtr<IUIElement> itemContainer;
            IFC_RETURN(GetContainer(&itemContainer));
            if (itemContainer)
            {
                wf::Size size = {};
                IFC_RETURN(itemContainer.Cast<UIElement>()->get_RenderSize(&size));

                wf::Rect rect = { 0.0f, 0.0f, size.Width, size.Height };
                itemContainer.Cast<UIElement>()->BringIntoView(rect, true /*forceIntoView*/, false /*useAnimation*/, false /*skipDuringManipulation*/);
            }
        }
    }

    return S_OK;
}
