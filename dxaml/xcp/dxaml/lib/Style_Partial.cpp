// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Style.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
StyleFactory::CreateInstanceImpl(
    _In_ wxaml_interop::TypeName targetType,
    _Outptr_ xaml::IStyle** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<Style> spStyle;

    IFC(ctl::make(&spStyle));
    IFC(spStyle->put_TargetType(targetType));

    *ppInstance = spStyle.Detach();

Cleanup:
    RRETURN(hr);
}
