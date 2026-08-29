// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FlyoutPresenterAutomationPeer.g.h"
#include "FlyoutPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT FlyoutPresenterAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IFlyoutPresenter* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IFlyoutPresenterAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IFlyoutPresenterAutomationPeer* pInstance = NULL;
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
            static_cast<FlyoutPresenter*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<FlyoutPresenterAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

IFACEMETHODIMP FlyoutPresenterAutomationPeer::GetPatternCore(_In_ xaml_automation_peers::PatternInterface patternInterface, _Outptr_ IInspectable** ppReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(ppReturnValue);
    *ppReturnValue = NULL;

    if (patternInterface == xaml_automation_peers::PatternInterface_Invoke)
    {
        *ppReturnValue = ctl::as_iinspectable(this);
        ctl::addref_interface(this);
    }
    else
    {
        IFC(FlyoutPresenterAutomationPeerGenerated::GetPatternCore(patternInterface, ppReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlyoutPresenterAutomationPeer::GetClassNameCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Flyout")).CopyTo(pReturnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlyoutPresenterAutomationPeer::GetAutomationIdCore(_Out_ HSTRING* pReturnValue)
{
    HRESULT hr = S_OK;
    INT length = 0;

    IFC(FlyoutPresenterAutomationPeerGenerated::GetAutomationIdCore(pReturnValue));

    length = WindowsGetStringLen(*pReturnValue);
    if(length == 0)
    {
        ctl::ComPtr<IFlyoutPresenter> spFlyout;
        ctl::ComPtr<IUIElement> spOwner;
        
        IFC(get_Owner(&spOwner));
        IFCPTR(spOwner.Get());

        IFC(spOwner.As<IFlyoutPresenter>(&spFlyout));
        IFC(static_cast<FlyoutPresenter*>(spFlyout.Get())->GetOwnerName(pReturnValue));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP FlyoutPresenterAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* pReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(pReturnValue);
    *pReturnValue = xaml_automation_peers::AutomationControlType_Pane;

Cleanup:
    RRETURN(hr);
}
