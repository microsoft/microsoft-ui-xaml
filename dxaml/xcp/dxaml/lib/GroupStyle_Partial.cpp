// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      StyleSelector allows the app writer to provide custom style selection logic.

#include "precomp.h"
#include "GroupStyle.g.h"
#include "PropertyChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

GroupStyle::GroupStyle()
{
}

GroupStyle::~GroupStyle()
{
}

// Handle the custom property changed event and call the
// OnPropertyChanged2 methods. 
_Check_return_ 
HRESULT 
GroupStyle::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    IFC(GroupStyleGenerated::OnPropertyChanged2(args));
    
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::GroupStyle_Panel:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"Panel")));
        break;

    case KnownPropertyIndex::GroupStyle_ContainerStyle:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"ContainerStyle")));
        break;

    case KnownPropertyIndex::GroupStyle_ContainerStyleSelector:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"ContainerStyleSelector")));
        break;

    case KnownPropertyIndex::GroupStyle_HeaderTemplate:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"HeaderTemplate")));
        break;

    case KnownPropertyIndex::GroupStyle_HeaderTemplateSelector:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"HeaderTemplateSelector")));
        break;

    case KnownPropertyIndex::GroupStyle_HidesIfEmpty:
        IFC(OnPropertyChanged(STR_LEN_PAIR(L"HidesIfEmpty")));
        break;
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ 
HRESULT 
GroupStyle::OnPropertyChanged(
    _In_reads_(nLength) const WCHAR* name,
    _In_ const XUINT32 nLength)
{
    HRESULT hr = S_OK;
    PropertyChangedEventSourceType* pEventSource = nullptr;
    ctl::ComPtr<PropertyChangedEventArgs> spArgs;
    wrl_wrappers::HString strPropertyName;

    // Create the args
    IFC(ctl::make(&spArgs));

    IFC(strPropertyName.Set(name, nLength));
    IFC(spArgs->put_PropertyName(strPropertyName));

    // Raise the event
    IFC(GetPropertyChangedEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spArgs.Get()));

Cleanup:
    RRETURN(hr);
}
