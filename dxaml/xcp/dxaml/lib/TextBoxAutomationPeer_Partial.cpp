// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxAutomationPeer.g.h"
#include "TextBox.g.h"
#include "TextBoxPlaceholderTextHelper.h"
#include "FrameworkElementAutomationPeer_partial.h"
#include "AutoSuggestBox_Partial.h"
#include "AccessKeyStringBuilder.h"

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
    ctl::ComPtr<IInspectable> localValue;
    ctl::ComPtr<IUIElement> owner;
    xaml::IDependencyProperty* accessKeyProperty = nullptr;
    BOOLEAN isUnset = FALSE;

    IFC_RETURN(get_Owner(owner.GetAddressOf()));

    MetadataAPI::GetIDependencyProperty(
        KnownPropertyIndex::AutomationProperties_AccessKey,
        &accessKeyProperty);
    IFC_RETURN(owner.Cast<UIElement>()->ReadLocalValue(accessKeyProperty, &localValue));
    IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(localValue.Get(), isUnset));

    // An explicitly set value, including an empty string, always belongs to the TextBox.
    if (!isUnset)
    {
        IFC_RETURN(FrameworkElementAutomationPeer::GetAccessKeyCore(returnValue));
        return S_OK;
    }

    wrl_wrappers::HString textBoxAccessKey;
    IFC_RETURN(FrameworkElementAutomationPeer::GetAccessKeyCore(textBoxAccessKey.GetAddressOf()));
    if (WindowsGetStringLen(textBoxAccessKey.Get()) > 0)
    {
        IFC_RETURN(textBoxAccessKey.CopyTo(returnValue));
        return S_OK;
    }

    ctl::ComPtr<IFrameworkElement> ownerAsFrameworkElement;
    IFC_RETURN(owner.As(&ownerAsFrameworkElement));

    ctl::ComPtr<DependencyObject> templatedParent;
    IFC_RETURN(ownerAsFrameworkElement.Cast<FrameworkElement>()->get_TemplatedParent(&templatedParent));

    auto autoSuggestBox = templatedParent.AsOrNull<xaml_controls::IAutoSuggestBox>();
    auto ownerAsTextBox = owner.AsOrNull<xaml_controls::ITextBox>();
    if (autoSuggestBox &&
        ownerAsTextBox &&
        autoSuggestBox.Cast<AutoSuggestBox>()->IsEditableTextBoxPart(ownerAsTextBox.Get()))
    {
        IFC_RETURN(AccessKeyStringBuilder::GetEffectiveAccessKeyFromElement(templatedParent, returnValue));
        return S_OK;
    }

    IFC_RETURN(textBoxAccessKey.CopyTo(returnValue));
    return S_OK;
}

_Check_return_ HRESULT TextBoxAutomationPeer::GetDescribedByCoreImpl(_Outptr_ wfc::IIterable<xaml_automation_peers::AutomationPeer*>** returnValue)
{
    ctl::ComPtr<IUIElement> spOwner;
    IFC_RETURN(get_Owner(spOwner.GetAddressOf()));

    IFC_RETURN(TextBoxPlaceholderTextHelper::SetupPlaceholderTextBlockDescribedBy(spOwner));

    IFC_RETURN(GetAutomationPeerCollection(UIAXcp::APDescribedByProperty, returnValue));
    return S_OK;
}