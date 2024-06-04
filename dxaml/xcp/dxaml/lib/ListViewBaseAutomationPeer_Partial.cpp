// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewBaseAutomationPeer.g.h"
#include "ListViewBase.g.h"
#include "ItemsControl.g.h"
#include "ItemsPresenter.g.h"
#include "FrameworkElementAutomationPeer.g.h"
#include "ContentControl.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ListViewBaseAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IListViewBase* pOwner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IListViewBaseAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<xaml_automation_peers::IListViewBaseAutomationPeer> spInstance;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml::IUIElement> spOwnerAsUIE;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(pOwner);
    IFC(ctl::do_query_interface(spOwnerAsUIE, pOwner))

    IFC(ActivateInstance(pOuter,
            static_cast<ListViewBase*>(pOwner)->GetHandle(),
            &spInner));

    IFC(spInner.As<xaml_automation_peers::IListViewBaseAutomationPeer>(&spInstance));
    IFC(spInstance.Cast<ListViewBaseAutomationPeer>()->put_Owner(spOwnerAsUIE.Get()));

    if (ppInner)
    {
        IFC(spInner.CopyTo(ppInner));
    }

    IFC(spInstance.CopyTo(ppInstance));

Cleanup:
    RRETURN(hr);
}

// Initializes a new instance of the ListViewBaseAutomationPeer class.
ListViewBaseAutomationPeer::ListViewBaseAutomationPeer()
{
}

// Deconstructor
ListViewBaseAutomationPeer::~ListViewBaseAutomationPeer()
{
}

IFACEMETHODIMP ListViewBaseAutomationPeer::GetChildrenCore(_Outptr_ wfc::IVector<xaml_automation_peers::AutomationPeer*>** ppReturnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<TrackerCollection<xaml_automation_peers::AutomationPeer*>> spAPChildren;
    ctl::ComPtr<xaml::IUIElement> spOwner;
    ctl::ComPtr<IItemsPresenter> spItemsPresenter;
    ctl::ComPtr<ContentControl> spHeaderContainer;
    ctl::ComPtr<ContentControl> spFooterContainer;

    IFCPTR(ppReturnValue);
    IFC(ctl::make<TrackerCollection<xaml_automation_peers::AutomationPeer*>>(&spAPChildren));

    IFC(get_Owner(&spOwner));
    IFCPTR(spOwner.Get());

    IFC(spOwner.Cast<ItemsControl>()->get_ItemsPresenter(&spItemsPresenter));
    if (spItemsPresenter)
    {
        IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_HeaderContainer(&spHeaderContainer));
        IFC(spItemsPresenter.Cast<ItemsPresenter>()->get_FooterContainer(&spFooterContainer));
    }

    if (spHeaderContainer)
    {
        IFC(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spHeaderContainer.Get(), spAPChildren.Get()));
    }
    IFC(ItemsControlAutomationPeer::GetItemsControlChildrenChildren(spAPChildren.Get()));
    if (spFooterContainer)
    {
        IFC(FrameworkElementAutomationPeer::GetAutomationPeerChildren(spFooterContainer.Get(), spAPChildren.Get()));
    }

    IFC(spAPChildren.CopyTo(ppReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP ListViewBaseAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;

    if (patternInterface == xaml_automation_peers::PatternInterface_DropTarget)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(ListViewBaseAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}


IFACEMETHODIMP ListViewBaseAutomationPeer::IsOffscreenCore(_Out_ BOOLEAN* returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwnerUie;
    ctl::ComPtr<IListViewBase> spOwnerLvb;
    ctl::ComPtr<ISemanticZoom> spSemanticZoom;
    BOOLEAN bIsActiveView = TRUE;

    IFCPTR(returnValue);
    *returnValue = FALSE;

    IFC(get_Owner(&spOwnerUie));
    IFCPTR(spOwnerUie.Get());

    IFC(spOwnerUie.As(&spOwnerLvb));
    IFC(spOwnerLvb.Cast<ListViewBase>()->get_SemanticZoomOwner(&spSemanticZoom));
    // Control is hosted inside SZ
    if (spSemanticZoom)
    {
        IFC(spOwnerLvb.Cast<ListViewBase>()->get_IsActiveView(&bIsActiveView));
        *returnValue = !bIsActiveView;
    }
    else
    {
        IFC(SelectorAutomationPeer::IsOffscreenCore(returnValue));
    }

Cleanup:
    RRETURN(hr);
}

// IDropTargetProvider -----------------------------------------------------------------------------------------------------------

_Check_return_ HRESULT ListViewBaseAutomationPeer::get_DropEffectImpl(_Out_ HSTRING* phsValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwnerUie;
    ctl::ComPtr<IListViewBase> spOwnerLvb;

    IFC(get_Owner(&spOwnerUie));
    IFC(spOwnerUie ? S_OK : E_FAIL);
    IFC(spOwnerUie.As(&spOwnerLvb));
    IFC(spOwnerLvb.Cast<ListViewBase>()->GetDropTargetDropEffect(phsValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT ListViewBaseAutomationPeer::get_DropEffectsImpl(_Out_ UINT* returnValueCount, _Out_ HSTRING** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IUIElement> spOwnerUie;
    ctl::ComPtr<IListViewBase> spOwnerLvb;

    IFC(get_Owner(&spOwnerUie));
    IFC(spOwnerUie ? S_OK : E_FAIL);
    IFC(spOwnerUie.As(&spOwnerLvb));
    IFC(spOwnerLvb.Cast<ListViewBase>()->GetDropTargetDropEffects(returnValueCount, returnValue));

Cleanup:
    RRETURN(hr);
}
