// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CTransform::TransformPoints(
    _In_reads_(cPoints) XPOINTF *pptOriginal,
    _Inout_updates_(cPoints) XPOINTF *pptTransformed,
    XUINT32 cPoints
    )
{
    CMILMatrix mat;
    GetTransform(&mat);
    mat.Transform(pptOriginal, pptTransformed, cPoints);
    RRETURN(S_OK);
}
