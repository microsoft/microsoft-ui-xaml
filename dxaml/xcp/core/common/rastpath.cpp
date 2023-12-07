// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#define SWAP(temp, a, b) { temp = a; a = b; b = temp; }

extern void
GenerateBezierFromQuadratic(
    _In_reads_(3) const XPOINTF *pptQuadratic,
    _Out_writes_(4) XPOINTF *pptCubic
    );

extern void
UpdateBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_ XINT32 cPoints                              // Count of points
    );

extern XINT32                          // Return the number of relevant roots
SolveSpecialQuadratic(
    _In_  XFLOAT a,             // In: Coefficient of x^2
    _In_  XFLOAT b,             // In: Coefficient of 2*x
    _In_  XFLOAT c,             // In: Constant term
    _Out_writes_(2) XFLOAT * r);

extern XINT32                          // Return the number of relevant zeros
GetDerivativeZeros(
    _In_ XFLOAT a,              // In: Bezier coefficient of (1-t)^3
    _In_ XFLOAT b,              // In: Bezier coefficient of 3t(1-t)^2
    _In_ XFLOAT c,              // In: Bezier coefficient of 3(1-t)t^2
    _In_ XFLOAT d,              // In: Bezier coefficient of t^3
    _Out_writes_(2) XFLOAT * r); // Out: An array of size 2 to receive the zeros

extern XFLOAT          // Return a(1-t)^3 + 3bt(1-t)^2 + 3c(1-t)t^2 + dt^3
GetBezierPolynomValue(
    XFLOAT a,   // In: Coefficient of (1-t)^3
    XFLOAT b,   // In: Coefficient of 3t(1-t)^2
    XFLOAT c,   // In: Coefficient of 3(1-t)t^2
    XFLOAT d,   // In: Coefficient of t^3
    XFLOAT t);   // In: Parameter value t

extern void
UpdateBezierBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_range_(4,4) XINT32 cPoints                   // Count of points
    );
