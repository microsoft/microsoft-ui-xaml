// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GeneratorPosition.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a GeneratorPosition from the specified index
//      and offset values.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
GeneratorPositionFactory::FromIndexAndOffset(
    _In_ INT index,
    _In_ INT offset,
    _Out_ xaml_primitives::GeneratorPosition* pReturnValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pReturnValue);

    pReturnValue->Index = index;
    pReturnValue->Offset = offset;

Cleanup:
    RRETURN(hr);
}

