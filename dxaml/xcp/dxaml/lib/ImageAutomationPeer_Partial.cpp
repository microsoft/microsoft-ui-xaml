// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "AutomationProperties.h"
#include "ImageAutomationPeer.g.h"
#include "Image.g.h"
#include "Image.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ImageAutomationPeerFactory::CreateInstanceWithOwnerImpl(
    _In_ xaml_controls::IImage* owner,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_automation_peers::IImageAutomationPeer** ppInstance)
{
    IFCEXPECT_RETURN(pOuter == NULL || ppInner != NULL);

    ctl::ComPtr<IInspectable> inner;
    IFC_RETURN(ActivateInstance(pOuter, static_cast<Image*>(owner)->GetHandle(), &inner));

    ctl::ComPtr<xaml_automation_peers::IImageAutomationPeer> instance;
    IFC_RETURN(inner.As(&instance));

    ctl::ComPtr<xaml::IUIElement> ownerAsUIE;
    IFC_RETURN(ctl::do_query_interface(ownerAsUIE, owner));

    IFC_RETURN(static_cast<ImageAutomationPeer*>(instance.Get())->put_Owner(ownerAsUIE.Get()));

    if (ppInner)
    {
        *ppInner = inner.Detach();
    }

    *ppInstance = instance.Detach();

    return S_OK;
}

IFACEMETHODIMP ImageAutomationPeer::GetClassNameCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(wrl_wrappers::HStringReference(STR_LEN_PAIR(L"Image")).CopyTo(returnValue));

    return S_OK;
}

IFACEMETHODIMP ImageAutomationPeer::GetAutomationControlTypeCore(_Out_ xaml_automation_peers::AutomationControlType* returnValue)
{
    *returnValue = xaml_automation_peers::AutomationControlType_Image;
    
    return S_OK;
}

IFACEMETHODIMP ImageAutomationPeer::GetNameCore(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(__super::GetNameCore(returnValue));

    auto length = (*returnValue != nullptr) ? WindowsGetStringLen(*returnValue) : 0;

    if (length == 0)
    {
        ctl::ComPtr<xaml::IUIElement> ownerAsUIE;
        IFC_RETURN(get_Owner(&ownerAsUIE));

        ctl::ComPtr<xaml_controls::IImage> owner;
        IFC_RETURN(ownerAsUIE.As(&owner));

        auto image = static_cast<CImage*>(owner.Cast<Image>()->GetHandle());
        IFCPTR_RETURN(image);
        IFC_RETURN(image->GetTitle(returnValue));
    }

    return S_OK;
}

_Check_return_ HRESULT ImageAutomationPeer::GetFullDescriptionCoreImpl(_Out_ HSTRING* returnValue)
{
    IFC_RETURN(__super::GetFullDescriptionCoreImpl(returnValue));

    auto length = (*returnValue != nullptr) ? WindowsGetStringLen(*returnValue) : 0;

    if (length == 0)
    {
        ctl::ComPtr<xaml::IUIElement> ownerAsUIE;
        IFC_RETURN(get_Owner(&ownerAsUIE));

        ctl::ComPtr<xaml_controls::IImage> owner;
        IFC_RETURN(ownerAsUIE.As(&owner));

        auto image = static_cast<CImage*>(owner.Cast<Image>()->GetHandle());
        IFCPTR_RETURN(image);
        IFC_RETURN(image->GetDescription(returnValue));
    }

    return S_OK;
}
