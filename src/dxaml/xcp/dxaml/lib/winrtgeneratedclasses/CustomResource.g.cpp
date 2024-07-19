// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#include "CustomResource.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::CustomResource::CustomResource()
{
}

DirectUI::CustomResource::~CustomResource()
{
}

HRESULT DirectUI::CustomResource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::CustomResource)))
    {
        *ppObject = static_cast<DirectUI::CustomResource*>(this);
    }
    else
    {
        RRETURN(DirectUI::MarkupExtensionBase::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.
_Check_return_ HRESULT DirectUI::CustomResource::get_ResourceKey(_Out_ HSTRING* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::CustomResource_ResourceKey, pValue));
}
_Check_return_ HRESULT DirectUI::CustomResource::put_ResourceKey(_In_opt_ HSTRING value)
{
    IFC_RETURN(DefaultStrictApiCheck(this));
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::CustomResource_ResourceKey, value));
}

// Events.

// Methods.


