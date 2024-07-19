// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FontFamily.g.h"

using namespace DirectUI;

_Check_return_ HRESULT FontFamilyFactory::CreateInstanceWithNameImpl(
    _In_ HSTRING familyName, 
    _In_opt_ IInspectable* pOuter, 
    _Outptr_ IInspectable** ppInner, 
    _Outptr_ xaml_media::IFontFamily** ppInstance)
{
    ctl::ComPtr<IInspectable> spInner;
    ctl::ComPtr<FontFamily> spInstance;

    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(pOuter, &spInner));
    IFC_RETURN(spInner.As(&spInstance));
    IFC_RETURN(spInstance->put_Source(familyName));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();

    return S_OK;
}

_Check_return_ HRESULT FontFamilyFactory::get_XamlAutoFontFamilyImpl(_Outptr_result_maybenull_ xaml_media::IFontFamily** ppValue)
{
    ctl::ComPtr<IInspectable> spInner;

    HSTRING xamlAutoFontFamily = wrl_wrappers::HStringReference(STR_LEN_PAIR(L"XamlAutoFontFamily")).Get();

    IFC_RETURN(CreateInstanceWithNameImpl(xamlAutoFontFamily, nullptr, &spInner, ppValue));

    return S_OK;
}

