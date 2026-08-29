// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Geometry.g.h"
#include "PathGeometry.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT GeometryFactory::get_EmptyImpl(
    _Outptr_result_maybenull_ IGeometry **pValue)
{
    HRESULT hr = S_OK;
    IGeometry* pGeometry = NULL;

    IFCPTR(pValue);

    IFC(ctl::ComObject<PathGeometry>::CreateInstance(&pGeometry));

    *pValue = pGeometry;
    pGeometry = NULL;

Cleanup:
    ReleaseInterface(pGeometry);
    RRETURN(hr);
}

_Check_return_ HRESULT GeometryFactory::get_StandardFlatteningToleranceImpl(
    _Out_ DOUBLE* pValue)
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = 0.25;

Cleanup:
    RRETURN(hr);
}

