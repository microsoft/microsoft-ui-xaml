// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MenuFlyoutPresenterAutomationPeer.g.h"
#include "MenuFlyoutPresenter.g.h"
#include "ItemAutomationPeer.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT MenuFlyoutPresenterAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IMenuFlyoutPresenter* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IMenuFlyoutPresenterAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IMenuFlyoutPresenterAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    *ppInstance = NULL;
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    if (ppInner)
    {
        *ppInner = NULL;
    }
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));
    
    IFC(ActivateInstance(pOuter,
            static_cast<MenuFlyoutPresenter*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<MenuFlyoutPresenterAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

IFACEMETHODIMP MenuFlyoutPresenterAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    INT length = 0;

    IFC(MenuFlyoutPresenterAutomationPeerGenerated::GetAutomationIdCore(pReturnValue));

    length = WindowsGetStringLen(*pReturnValue);
    if(length == 0)
    {
        ctl::ComPtr<IMenuFlyoutPresenter> spMenuFlyout;
        ctl::ComPtr<IUIElement> spOwner;
        
        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner.Get());

        IFC(spOwner.As<IMenuFlyoutPresenter>(&spMenuFlyout));
        IFC(static_cast<MenuFlyoutPresenter*>(spMenuFlyout.Get())->GetOwnerName(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutPresenterAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"MenuFlyout")).CopyTo(pReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP MenuFlyoutPresenterAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Menu;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Top-Down menuflyout expansion require ItemAutomationPeer to be presented
//
//------------------------------------------------------------------------
_Check_return_ HRESULT MenuFlyoutPresenterAutomationPeer::OnCreateItemAutomationPeerImpl(_In_ IInspectable* item, _Outptr_ xaml_automation_peers::IItemAutomationPeer** returnValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spItem(item);
    ctl::ComPtr<xaml_automation_peers::IItemAutomationPeer> spItemAutomationPeer;
    ctl::ComPtr<xaml_automation_peers::IItemAutomationPeerFactory> spItemAutomationPeerFactory;
    ctl::ComPtr<IActivationFactory> spActivationFactory;
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<xaml_controls::IMenuFlyoutSeparator> spSeparatorItem;
    
    spSeparatorItem = spItem.AsOrNull<IMenuFlyoutSeparator>();
    // Not providing automation peer for separator element
    if (spSeparatorItem)
    {
        RRETURN(S_OK);
    }

    spActivationFactory.Attach(ctl::ActivationFactoryCreator<DirectUI::ItemAutomationPeerFactory>::CreateActivationFactory());
    IFC(spActivationFactory.As<xaml_automation_peers::IItemAutomationPeerFactory>(&spItemAutomationPeerFactory));

    IFC(spItemAutomationPeerFactory.Cast<ItemAutomationPeerFactory>()->CreateInstanceWithParentAndItem(item,
        this,
        NULL, 
        &spInner, 
        &spItemAutomationPeer));
    IFC(spItemAutomationPeer.CopyTo<xaml_automation_peers::IItemAutomationPeer>(returnValue));

Cleanup:
    RRETURN(hr);
}
