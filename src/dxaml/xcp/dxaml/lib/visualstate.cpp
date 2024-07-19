// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualState.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
VisualState::get_NameImpl(
    _Out_ HSTRING* phName)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IInspectable> spName;

    IFCPTR(phName);

    IFC(GetValue(
        MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DependencyObject_Name),
        &spName));

    IFC(ctl::do_get_value(*phName, spName.Get()));

Cleanup:
    RRETURN(hr);
}
