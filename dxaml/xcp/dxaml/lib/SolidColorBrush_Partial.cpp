// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SolidColorBrush.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SolidColorBrushFactory::CreateInstanceWithColorImpl(
    _In_ wu::Color color,
    _Outptr_ ISolidColorBrush** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<SolidColorBrush> spInstance;

    IFCPTR(ppInstance);

    IFC(ctl::make(&spInstance));
    IFC(spInstance->put_Color(color));

    *ppInstance = spInstance.Detach();

Cleanup:
    RRETURN(hr);
}

