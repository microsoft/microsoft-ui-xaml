// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Description:
//      Definition of methods used to create intermediate brush 
//      representations from user-defined state.

class CBrushTypeUtils
{
public:

    static void GetBrushTransform(
        _In_reads_opt_(1) const CMILMatrix *pmatRelative,
        _In_reads_opt_(1) const CMILMatrix *pmatTransform,
        _In_reads_(1) const XRECTF *pBoundingBox,
        _Out_writes_(1) CMILMatrix *pResultTransform
        );

    static void ConvertRelativeTransformToAbsolute(
        _In_reads_(1) const XRECTF *pBoundingBox,
        _In_reads_(1) const CMILMatrix *pRelativeTransform,
        _Out_writes_(1) CMILMatrix* pConvertedTransform
        );
};


void 
AdjustRelativePoint(
    _In_ const XRECTF *pBoundingBox,
    _Inout_ XPOINTF *pt
    );

void 
AdjustRelativeRectangle(
    _In_ const XRECTF *prcBoundingBox,
    _Inout_ XRECTF *prcAdjustRectangle
    );

