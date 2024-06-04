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
    _In_ xaml_controls::Symbol symbol,
    _Outptr_ xaml_controls::ISymbolIcon** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<SymbolIcon> spInstance;

    IFC(ctl::make(&spInstance));
    IFC(spInstance->put_Symbol(symbol));

    *ppInstance = spInstance.Detach();

Cleanup:
    RRETURN(hr);
}
