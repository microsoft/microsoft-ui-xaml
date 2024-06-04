// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorDisplayNameHelper.g.h"
#include <ColorHelper.h>

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT ColorDisplayNameHelperFactory::ToDisplayNameImpl(
    _In_ wu::Color color,
    _Out_ HSTRING* returnValue)
{
    IFCPTR_RETURN(returnValue);

    int colorNameResourceId = 0;
    IFC_RETURN(ColorHelper_ToDisplayNameId(color, &colorNameResourceId));
    IFC_RETURN(DXamlCore::GetCurrent()->GetLocalizedResourceString(colorNameResourceId, returnValue));

    return S_OK;
}

