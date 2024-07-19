// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "IconElement.g.h"
#include "SymbolIcon.g.h"
#include "SymbolIconSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
SymbolIconSource::CreateIconElementCoreImpl(_Outptr_ ABI::Microsoft::UI::Xaml::Controls::IIconElement** returnValue)
{
    xaml_controls::Symbol symbol;
    
    IFC_RETURN(get_Symbol(&symbol));
    
    ctl::ComPtr<SymbolIcon> symbolIcon;
    IFC_RETURN(ctl::make(&symbolIcon));
    
    IFC_RETURN(symbolIcon->put_Symbol(symbol));

    *returnValue = symbolIcon.Detach();
    return S_OK;
}

_Check_return_ HRESULT
SymbolIconSource::GetIconElementPropertyCoreImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyProperty* iconSourceProperty, _Outptr_ ABI::Microsoft::UI::Xaml::IDependencyProperty** returnValue)
{
    ctl::ComPtr<DependencyPropertyHandle> iconSourcePropertyHandle;
    IFC_RETURN(ctl::do_query_interface(iconSourcePropertyHandle, iconSourceProperty));
    
    switch (iconSourcePropertyHandle->GetDP()->GetIndex())
    {
    case KnownPropertyIndex::SymbolIconSource_Symbol:
        IFC_RETURN(MetadataAPI::GetIDependencyProperty(KnownPropertyIndex::SymbolIcon_Symbol, returnValue));
        break;
    default:
        IFC_RETURN(SymbolIconSourceGenerated::GetIconElementPropertyCoreImpl(iconSourceProperty, returnValue));
    }
    
    return S_OK;
}