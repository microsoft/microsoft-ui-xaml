// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "PointHelper.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
PointHelperFactory::FromCoordinatesImpl(
    _In_ FLOAT x,
    _In_ FLOAT y,
    _Out_ wf::Point* pReturnValue)
{
    pReturnValue->X = x;
    pReturnValue->Y = y;
    return S_OK;
}
