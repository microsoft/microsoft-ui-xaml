// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PasswordBoxAutomationPeer.g.h"
#include "PasswordBox.g.h"
#include "TextBoxPlaceholderTextHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT PasswordBoxAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IPasswordBox* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IPasswordBoxAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IPasswordBoxAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<PasswordBox*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<PasswordBoxAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the PasswordBoxAutomationPeer class.
PasswordBoxAutomationPeer::PasswordBoxAutomationPeer()
{
}

// Deconstructor
PasswordBoxAutomationPeer::~PasswordBoxAutomationPeer()
{
}

IFACEMETHODIMP PasswordBoxAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"PasswordBox")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP PasswordBoxAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Edit;
    RRETURN(S_OK);
}

IFACEMETHODIMP PasswordBoxAutomationPeer::IsPasswordCore(_Out_ BOOLEAN* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);
    *pReturnValue = TRUE;

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT PasswordBoxAutomationPeer::GetDescribedByCoreImpl (_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(TextBoxPlaceholderTextHelper::SetupPlaceholderTextBlockDescribedBy(spOwner));

    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APDescribedByProperty, returnValue));
    return S_OK;
}