// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SymbolIcon.g.h"

using namespace DirectUI;

// Initializes a new instance of the SymbolIcon class.
SymbolIcon::SymbolIcon()
{
}

// Deconstructor
SymbolIcon::~SymbolIcon()
{
}

// Apply a template to the icon.
IFACEMETHODIMP SymbolIcon::OnApplyTemplate()
{
    HRESULT hr = S_OK;
    
    IFC(SymbolIconGenerated::OnApplyTemplate());
    
Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP DirectUI::SymbolIcon::put_Symbol(_In_ xaml_controls::Symbol value)
{
    RRETURN(DependencyObject::SetValueByKnownIndex(KnownPropertyIndex::SymbolIcon_Symbol, value));
}

void DirectUI::SymbolIcon::SetFontSize(_In_ float fontSize)
{
    static_cast<CSymbolIcon *>(GetHandle())->SetFontSize(fontSize);
}

_Check_return_ HRESULT SymbolIconFactory::CreateInstanceWithSymbolImpl(
    xaml_controls::Symbol symbol,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_controls::ISymbolIcon** ppInstance)
{
    ctl::ComPtr<SymbolIcon> spInstance;
    ctl::ComPtr<IInspectable> spInner;

    IFCEXPECT_RETURN(pOuter == nullptr || ppInner != nullptr);
    IFC_RETURN(CheckActivationAllowed());

    IFC_RETURN(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(pOuter, &spInner));
    IFC_RETURN(spInner.As(&spInstance));
    IFC_RETURN(spInstance->put_Symbol(symbol));

    if (ppInner)
    {
        *ppInner = spInner.Detach();
    }

    *ppInstance = spInstance.Detach();
    return S_OK;
}
