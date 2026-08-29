// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "AccessKeyStringBuilder.h"
#include "UIElement.g.h"
#include "Hyperlink.g.h"
#include "AutomationAnnotation.g.h"
#include "AutomationPeerAnnotation.g.h"
#include "AutomationProperties.h"
#include "FrameworkElement.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT InterleaveStringWithDelimiters(_In_ PCWSTR toInterleave, _In_ int length, _Out_ HSTRING* interleavedString)
{
    IFCEXPECT_RETURN(length <= 3);
    //We take advantage of the fact that access keys can be at most 3 characters long.
    //So, after interleaving ',' and ' ', the string can be at most 9 characters long.
    WCHAR AccessKeyInterleaved[9] = { 0 };

    for (int i = 0; i < length; i++)
    {
        AccessKeyInterleaved[3 * i] = ',';
        AccessKeyInterleaved[(3 * i) + 1] = ' ';
        AccessKeyInterleaved[(3 * i) + 2] = toInterleave[i];
    }
    IFC_RETURN(WindowsCreateString(AccessKeyInterleaved, (length * 3), interleavedString));
    return S_OK;
}

_Check_return_ HRESULT GetAccessKeyFromOwner(_In_ ctl::ComPtr<DependencyObject>& spOwner, _Out_ HSTRING *accessKey, _Outptr_result_maybenull_ xaml::IDependencyObject** scopeOwner)
{

    ctl::ComPtr<FrameworkElement> ownerAsFrameworkElement(spOwner.AsOrNull<FrameworkElement>());
    ctl::ComPtr<TextElement> ownerAsTextElement(spOwner.AsOrNull<TextElement>());

    if (ownerAsFrameworkElement)
    {
        IFC_RETURN(ownerAsFrameworkElement->GetValueByKnownIndex(KnownPropertyIndex::UIElement_AccessKey, accessKey));
        IFC_RETURN(ownerAsFrameworkElement->get_AccessKeyScopeOwnerImpl(scopeOwner));
    }
    else if (ownerAsTextElement)
    {
        IFC_RETURN(ownerAsTextElement->GetValueByKnownIndex(KnownPropertyIndex::TextElement_AccessKey, accessKey));
        IFC_RETURN(ownerAsTextElement->get_AccessKeyScopeOwnerImpl(scopeOwner));
    }
    else
    {
        IFCFAILFAST(E_INVALIDARG);
    }

    return S_OK;
}

_Check_return_ HRESULT AccessKeyStringBuilder::GetAccessKeyMessageFromElement(_In_ ctl::ComPtr<DependencyObject>& spOwner, _Out_ HSTRING *returnValue)
{
    wrl_wrappers::HString prefix;
    wrl_wrappers::HString accessKey;
    wrl_wrappers::HString messageWithoutPrefix;

    UINT32 length;

    ctl::ComPtr<DirectUISynonyms::IDependencyObject> akScopeOwner;
    IFC_RETURN(GetAccessKeyFromOwner(spOwner, accessKey.GetAddressOf(), &akScopeOwner));


    PCWSTR AccessKeyRawBuffer = WindowsGetStringRawBuffer(accessKey.Get(), &length);
    ASSERT(length <= 3);

    //If no AccessKey was set, return an empty String
    if (length == 0)
    {
        IFC_RETURN(WindowsCreateString(STR_LEN_PAIR(L""), returnValue));
        return S_OK;
    }

    //If the scope owner exists, grab its sequence and add access keys.
    ctl::ComPtr<xaml_automation_peers::IAutomationPeer> automationPeer;
    if (akScopeOwner)
    {
        ctl::ComPtr<IFrameworkElement> akScopeOwnerAsFrameworkElement(akScopeOwner.AsOrNull<IFrameworkElement>());
        ctl::ComPtr<ITextElement> akScopeOwnerAsTextElement(akScopeOwner.AsOrNull<ITextElement>());

        if (akScopeOwnerAsFrameworkElement)
        {
            IFC_RETURN(akScopeOwnerAsFrameworkElement.Cast<FrameworkElement>()->GetOrCreateAutomationPeer(&automationPeer));
        }
        else if (akScopeOwnerAsTextElement)
        {
            IFC_RETURN(akScopeOwnerAsTextElement.Cast<TextElement>()->GetOrCreateAutomationPeer(&automationPeer));
        }
        else
        {
            IFCFAILFAST(E_INVALIDARG);
        }
    }

    if (automationPeer)
    {
        IFC_RETURN(automationPeer.Cast<AutomationPeer>()->GetAccessKey(prefix.GetAddressOf()));
    }
    else
    {
        IFC_RETURN(WindowsCreateString(STR_LEN_PAIR(L"Alt"), prefix.GetAddressOf()));
    }

    IFC_RETURN(InterleaveStringWithDelimiters(AccessKeyRawBuffer, length, messageWithoutPrefix.GetAddressOf()));
    IFC_RETURN(WindowsConcatString(prefix.Get(), messageWithoutPrefix.Get(), returnValue));

    return S_OK;
}



