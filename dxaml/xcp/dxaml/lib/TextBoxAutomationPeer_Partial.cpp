// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxAutomationPeer.g.h"
#include "TextBox.g.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "FrameworkElementAutomationPeer_partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT TextBoxAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::ITextBox* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::ITextBoxAutomationPeer** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::ITextBoxAutomationPeer* pInstance = NULL;
    IInspectable* pInner = NULL;
    xaml::IUIElement* ownerAsUIE = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);
    IFCPTR(owner);
    IFC(ctl::do_query_interface(ownerAsUIE, owner));

    IFC(ActivateInstance(pOuter,
            static_cast<TextBox*>(owner)->GetHandle(),
            &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));
    IFC(static_cast<TextBoxAutomationPeer*>(pInstance)->put_Owner(ownerAsUIE));

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

// Initializes a new instance of the TextBoxAutomationPeer class.
TextBoxAutomationPeer::TextBoxAutomationPeer()
{
}

// Deconstructor
TextBoxAutomationPeer::~TextBoxAutomationPeer()
{
}

IFACEMETHODIMP TextBoxAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"TextBox")).CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP TextBoxAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Edit;
    RRETURN(S_OK);
}

_Check_return_ HRESULT TextBoxAutomationPeer::GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(TextBoxPlaceholderTextHelper::SetupPlaceholderTextBlockDescribedBy(spOwner));

    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APDescribedByProperty, returnValue));
    return S_OK;
}