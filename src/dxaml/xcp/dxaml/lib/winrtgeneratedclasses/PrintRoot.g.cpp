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

#include "PrintRoot.g.h"
#include "XamlTelemetry.h"

// Constructors/destructors.
DirectUI::PrintRoot::PrintRoot()
{
}

DirectUI::PrintRoot::~PrintRoot()
{
}

HRESULT DirectUI::PrintRoot::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::PrintRoot)))
    {
        *ppObject = static_cast<DirectUI::PrintRoot*>(this);
    }
    else
    {
        RRETURN(DirectUI::Canvas::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

// Properties.

// Events.

// Methods.


namespace DirectUI
{
}
