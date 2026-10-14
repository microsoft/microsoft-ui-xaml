// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxAutomationPeer.g.h"
#include "TextBox.g.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "FrameworkElementAutomationPeer_partial.h"
#include "AutoSuggestBox.g.h"
#include "AutoSuggestBox_Partial.h"

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

IFACEMETHODIMP TextBoxAutomationPeer::GetAccessKeyCore(_Out_ HSTRING* returnValue)
{
    // If the owning AutoSuggestBox's AccessKeyScopeOwner chain leads back to this
    // TextBox, asking the parent peer for its access key re-enters this method and
    // would recurse indefinitely. When already resolving, break the cycle by falling
    // back to the default implementation instead of delegating to the parent again.
    if (!m_isResolvingAccessKey)
    {
        ctl::ComPtr<IUIElement> owner;
        IFC_RETURN(get_Owner(owner.GetAddressOf()));

        auto textBox = owner.Cast<TextBox>();
        const auto automationAccessKeyProperty =
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::AutomationProperties_AccessKey);
        const auto accessKeyProperty =
            MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::UIElement_AccessKey);

        if (textBox->GetHandle()->IsPropertyDefault(automationAccessKeyProperty) &&
            textBox->GetHandle()->IsPropertyDefault(accessKeyProperty))
        {
            ctl::ComPtr<DependencyObject> templatedParent;
            IFC_RETURN(textBox->get_TemplatedParent(&templatedParent));

            auto autoSuggestBox = templatedParent.AsOrNull<xaml_controls::IAutoSuggestBox>();
            if (autoSuggestBox)
            {
                auto autoSuggestBoxImpl = autoSuggestBox.Cast<AutoSuggestBox>();
                if (autoSuggestBoxImpl->IsTextBoxPart(textBox))
                {
                    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> autoSuggestBoxPeer;
                    IFC_RETURN(autoSuggestBoxImpl->GetOrCreateAutomationPeer(&autoSuggestBoxPeer));
                    if (autoSuggestBoxPeer)
                    {
                        m_isResolvingAccessKey = true;
                        auto resetGuard = wil::scope_exit([this]() { m_isResolvingAccessKey = false; });

                        return autoSuggestBoxPeer.Cast<AutomationPeer>()->GetAccessKey(returnValue);
                    }
                }
            }
        }
    }

    return FrameworkElementAutomationPeer::GetAccessKeyCore(returnValue);
}

_Check_return_ HRESULT TextBoxAutomationPeer::GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(TextBoxPlaceholderTextHelper::SetupPlaceholderTextBlockDescribedBy(spOwner));

    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APDescribedByProperty, returnValue));
    return S_OK;
}