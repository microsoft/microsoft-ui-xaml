// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Contains math helper macros/inlines

#pragma once

#undef min
#undef max
#define NOMINMAX

#include <matrix.h>
#include <algorithm>
#include <gsl/span>

#define _USE_MATH_DEFINES
#include <math.h>

// Min/Max macros
#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif

// Limit macros
#define XINT32_MIN     (-2147483647 - 1) /* minimum (signed) int value */
#define XINT32_MAX       2147483647      /* maximum (signed) int value */
#define XUINT32_MAX      0xFFFFFFFF

#define XUINT8_MAX      0xFF
#define XUINT16_MAX     0xFFFF

#define XINT64_MIN     (-9223372036854775807i64 - 1)

#define XFLOAT_MIN     -3.402823466e+38F        /* min value */
#define XFLOAT_MAX     3.402823466e+38F         /* max value */

#define XDOUBLE_MAX         1.7976931348623158e+308 /* max value */

#define XDOUBLE_INF (XDOUBLE_MAX * 2)
#define XDOUBLE_NAN (XDOUBLE_INF/XDOUBLE_INF)

const XFLOAT XFLOAT_INF = static_cast<XFLOAT>(XDOUBLE_INF);


//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents a NaN.
//
//------------------------------------------------------------------------------
bool
IsNanF(
    XFLOAT value
    );

//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents +INF or -INF.
//
//------------------------------------------------------------------------------
bool
IsInfiniteF(
    XFLOAT value
    );

//------------------------------------------------------------------------------
//
//  A helper function that checks if a given XFLOAT represents finite number only.
//
//------------------------------------------------------------------------------
bool
IsFiniteF(
    XFLOAT value
    );
//
// Math constants
//

#define SQRT_2          1.4142135623730950488016887242097f
#define REAL_EPSILON    1.192092896e-07F /* FLT_EPSILON */

//
// Fixed point 16 helpers
//

enum
{
    FIX16_SHIFT = 16,
    FIX16_ONE = 1 << FIX16_SHIFT,
    FIX16_HALF = 1 << (FIX16_SHIFT - 1),
    FIX16_MASK = FIX16_ONE - 1
};

//
// Fixed point 8 helpers
//
enum {
    FIX8_SHIFT = 8,
    FIX8_ONE = 1 << FIX8_SHIFT,
    FIX8_HALF = 1 << (FIX8_SHIFT - 1),
    FIX8_MASK = FIX8_ONE - 1
};

//
// The x86 C compiler understands assembler. Therefore, functions
// that employ assembler are used for shifts of 0..31.  The multiplies
// rely on the compiler recognizing the cast of the multiplicand to int64 to
// generate the optimal code inline.
//

#define INT32x32To64(a, b)  ((XINT64)(((XINT64)((XINT32)(a))) * ((XINT32)(b))))

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XINT32
XcpAbs(_In_ XINT32 nValue);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XFLOAT
XcpAbsF(_In_ XFLOAT rValue);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the absolute value
//
//------------------------------------------------------------------------
XDOUBLE
XcpAbsD(_In_ XDOUBLE rValue);

//+------------------------------------------------------------------------
//
//  Synopsis:  Template method that clamps a value to a range
//
//  Notes:     This method is written in manner such that NaNs
//             are clamped to the minValue.
//
//-------------------------------------------------------------------------
template <class T>
T
ClampValue(_In_ T value, _In_ T minValue, _In_ T maxValue)
{
    ASSERT(minValue <= maxValue);

    if (value > maxValue)
    {
        return maxValue;
    }
    else if (value >= minValue)
    {
        return value;
    }
    else
    {
        return minValue;
    }
}

//+------------------------------------------------------------------------
//
//  Synopsis:  Clamps an integer to the specified range.
//
//-------------------------------------------------------------------------
int
ClampInteger(_In_ XINT32 nValue, _In_ XINT32 nMin, _In_ XINT32 nMax);

//+------------------------------------------------------------------------
//
//  Synopsis:  Clamps a float to the specified range.
//
//-------------------------------------------------------------------------
XFLOAT
ClampReal(_In_ XFLOAT rValue, _In_ XFLOAT rMin, _In_ XFLOAT rMax);

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the integral component and returns just the fractional
//      component of the argument float.
//
//-------------------------------------------------------------------------
XFLOAT
FractionReal(XFLOAT val);

//-------------------------------------------------------------------------
//
//  Synopsis:  Round num to the closest power of 2 that is equal or greater
//             than num.  Input must be greater than 0 and less than equal
//             to (1 <<31)
//
//-------------------------------------------------------------------------
XUINT32
RoundToPow2(_In_ XUINT32 uNum);

//-------------------------------------------------------------------------
//
//  Synopsis:  Returns the distance between 2 MILPointF points
//
//-------------------------------------------------------------------------
XFLOAT
Distance(_In_ XPOINTF pt1, _In_ XPOINTF pt2);

//-------------------------------------------------------------------------
//
//   Return TRUE if two points are close. Close is defined as near enough
//   that the rounding to 32bit float precision could have resulted in the
//   difference. We define an arbitrary number of allowed rounding errors (10).
//   We divide by b to normalize the difference. It doesn't matter which point
//   we divide by - if they're significantly different, we'll return true, and
//   if they're really close, then a==b (almost).
//
// Arguments:
//
//   a, b - input numbers to compare.
//
// Return Value:
//
//   TRUE if the numbers are close enough.
//
//-------------------------------------------------------------------------
bool
IsCloseReal(_In_ const XFLOAT a, _In_ const XFLOAT b);

bool
IsLessThanReal(_In_ const XFLOAT a, _In_ const XFLOAT b);

//-------------------------------------------------------------------------
//
//  Synopsis:  Returns the determinant
//
//-------------------------------------------------------------------------
XFLOAT
Determinant(
    _In_ const XPOINTF &a,
    _In_ const XPOINTF &b);

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Right turn in a left handed coordinate system (left otherwise)
//
//-------------------------------------------------------------------------
XPOINTF
TurnRight(_In_ const XPOINTF &pt);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point?
//
//------------------------------------------------------------------------
bool
DoesRectContainPoint(
    _In_ const XRECTF &rect,
    _In_ const XPOINTF &point
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point?
//
//------------------------------------------------------------------------
bool
DoesRectContainPoint(
    _In_ const XRECTF_RB &rect,
    _In_ const XPOINTF &point
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does the rectangle contain the point inclusively?
//
//------------------------------------------------------------------------
bool
DoesRectContainPointInclusive(
    _In_ const XRECTF_RB &rect,
    _In_ const XPOINTF &point
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECT &rc1,
    _In_ const XRECT &rc2
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECTF_RB &a,
    _In_ const XRECTF_RB &b
    );


//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect
//
//------------------------------------------------------------------------
bool
DoRectsIntersect(
    _In_ const XRECTF &rc1,
    _In_ const XRECTF &rc2
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect or exactly touch
//
//------------------------------------------------------------------------
bool
DoRectsIntersectInclusive(
    _In_ const XRECTF &rc1,
    _In_ const XRECTF &rc2
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if two rectangles intersect inclusively
//
//------------------------------------------------------------------------
bool
DoRectsIntersectInclusive(
    _In_ const XRECTF_RB &a,
    _In_ const XRECTF_RB &b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does this polygon intersect this other polygon?
//
//------------------------------------------------------------------------
bool DoPolygonsIntersect(
    _In_ XUINT32 cPolyA,
    _In_reads_(cPolyA) const XPOINTF *pPtPolyA,
    _In_ XUINT32 cPolyB,
    _In_reads_(cPolyB) const XPOINTF *pPtPolyB);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is polygon A contained entirely inside polygon B?
//      PointType is XPOINTF for 2D and XPOINTF4 for projection clipping.
//      Ignores the z values in polygon A.
//
//------------------------------------------------------------------------
template <typename PointType>
bool IsEntirelyContained(
        _In_ XUINT32 cPolyA,
        _In_reads_(cPolyA) const PointType *pPtPolyA,
        _In_ XUINT32 cPolyB,
        _In_reads_(cPolyB) XPOINTF *pPtPolyB);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is the polygon concave?
//
//------------------------------------------------------------------------
bool IsPolygonConcave(const gsl::span<const XPOINTF>& points);
bool IsPolygonConcave(const gsl::span<const XPOINTF4>& points);
template <typename PointType>
bool IsPolygonConcave(
    XUINT32 cPoints,
    _In_reads_(cPoints) const PointType *pPoints);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes a convex hull of a set of points.
//
//------------------------------------------------------------------------
template <typename PointType>
void ComputeConvexHull(
    _In_ const XUINT32 numPoints,
    _In_reads_(numPoints) const PointType *pPoints,
    _Out_ XUINT32 *pNumHullPoints,
    _Out_writes_to_(numPoints, *pNumHullPoints) PointType *pHullPoints);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
ZeroSize(
    _Out_ XSIZE *pSize
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsZeroSize(_In_ const XSIZE &size);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRect(
    _Out_ XRECT *pRect
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRect(_In_ const XRECT &rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRect(
    _Out_ XRECT_RB *pRect
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRect(_In_ const XRECT_RB &rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRectF(
    _Out_ XRECTF *pRect
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRectF(_In_ const XRECTF &rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set empty rect
//
//------------------------------------------------------------------------
void
EmptyRectF(
    _Out_ XRECTF_RB *pRect
    );

void
InvalidRectF( _Out_ XRECTF_RB *pRect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is empty.
//
//------------------------------------------------------------------------
bool
IsEmptyRectF(_In_ const XRECTF_RB& Rect);

bool IsInvalidRectF(_In_ const XRECTF_RB& rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets rect to 'empty' clip.  This clip uses extreme values to
//      ensure by default that nothing we can render is clipped.
//
//------------------------------------------------------------------------
void
SetInfiniteClip(_Out_ XRECTF *pClipRect);

XRECTF
GetInfiniteClip();

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if a rectangle is infinite.
//
//------------------------------------------------------------------------
bool
IsInfiniteRectF(_In_ const XRECTF& Rect);


//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECTF_RB to an XRECTF
//
//------------------------------------------------------------------------
XRECTF
ToXRectF(_In_ const XRECTF_RB& Rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECTF to an XRECTF_RB
//
//------------------------------------------------------------------------
XRECTF_RB
ToXRectFRB(_In_ const XRECTF& Rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECT_RB to an XRECT
//
//------------------------------------------------------------------------
XRECT
ToXRect(_In_ const XRECT_RB& Rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Convert an XRECT to an XRECT_RB
//
//------------------------------------------------------------------------
XRECT_RB
ToXRectRB(_In_ const XRECT& Rect);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRect(
    _Inout_ XRECT *a,
    _In_ const XRECT *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRect(
    _Inout_ XRECT_RB *a,
    _In_ const XRECT_RB *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRectF(
    _Inout_ XRECTF *a,
    _In_ const XRECTF *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Union 2 rectangles
//
//------------------------------------------------------------------------
void
UnionRectF(
    _Inout_ XRECTF_RB *a,
    _In_ const XRECTF_RB *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Expand rectangle bounds - empty rectangles still have bounds.
//
//------------------------------------------------------------------------
void
UpdateRectBoundsF(
    _Inout_ XRECTF_RB *a,
    _In_ const XRECTF_RB *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(
    _Inout_ XRECTF *a,
    _In_ const XRECTF *b
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECT* a, _In_ const XRECT* b);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECT_RB* a, _In_ const XRECT_RB* b);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects 2 rectangles
//
//------------------------------------------------------------------------
bool
IntersectRect(_Inout_ XRECTF_RB* a, _In_ const XRECTF_RB* b);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if rect "contained" is contained inside of rect "container"
//      This is an inclusive comparison (a rect is contained inside itself)
//
//------------------------------------------------------------------------
bool
DoesRectContainRect(_In_ const XRECTF_RB* container, _In_ const XRECTF_RB* contained);

bool
DoesRectContainRect(_In_ const XRECT_RB* container, _In_ const XRECT_RB* contained);

bool
DoesRectContainRect(_In_ const XRECTF_WH *pContainer, _In_ const XRECTF_WH *pContained);

bool
DoesRectContainRect(_In_ const XRECT_WH *pContainer, _In_ const XRECT_WH *pContained);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inflates a rectangle to be integer aligned.
//
//------------------------------------------------------------------------
void
InflateRectF(_Inout_ XRECTF_RB* a);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares to rectangles for inequality
//
//------------------------------------------------------------------------
bool
operator !=(const XRECT_WH& lhs, const XRECT_WH& rhs);

bool
operator !=(const XRECT_RB& lhs, const XRECT_RB& rhs);

// NOTE: The floating-point rect comparisons do not account for rounding error.
bool
operator !=(const XRECTF_WH& lhs, const XRECTF_WH& rhs);

bool
operator !=(const XRECTF_RB& lhs, const XRECTF_RB& rhs);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Compares to rectangles for equality
//
//------------------------------------------------------------------------
bool
operator ==(const XRECT_WH& lhs, const XRECT_WH& rhs);

bool
operator ==(const XRECT_RB& lhs, const XRECT_RB& rhs);

bool
operator ==(const XRECTF_WH& lhs, const XRECTF_WH& rhs);

bool
operator ==(const XRECTF_RB& lhs, const XRECTF_RB& rhs);

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Returns a floating point mod.
//
//     This definition assumes y > 0.
//     GpModF(x, Inf) = x, as long as x != Inf.
//
//-------------------------------------------------------------------------
XDOUBLE XcpMod(XDOUBLE x, XDOUBLE y);
XFLOAT XcpModF(XFLOAT x, XFLOAT y);

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) XPOINTF* pPoints,
    _In_ const XRECTF& rcRect
    );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) XPOINTF4* pPoints,
    _In_ const XRECTF& rcRect
    );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a point with the AA offset or shift depending on the mask
//      00 - no AA, no shifting
//      11 - AA, shifts interior points inward and exterior points outward
//      01 - Shift inwards
//      10 - Shift outwards
//
//-------------------------------------------------------------------------

void FillPointWithAAOffset(
    XUINT32 aaMask,
    _Out_ XINT32* pAAMaskInterior,
    _Out_ XINT32* pAAMaskExterior
    );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a 4-point array in counter-clockwise order with the 4
//      corner points form a rect.
//
//-------------------------------------------------------------------------
void FillPointsFromRectCCW(
    _Out_writes_(4) PointWithAAMasks* pPoints,
    _In_ const XRECTF& rcRect,
    XUINT32 aaLeft,
    XUINT32 aaTop,
    XUINT32 aaRight,
    XUINT32 aaBottom
    );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a rect from a 4-point array in counter-clockwise order and
//  returns true if the points form a rect.  Otherwise, returns false.
//
//-------------------------------------------------------------------------
bool FillRectFromPointsCCW(const gsl::span<const XPOINTF>& points, XRECTF& rect);

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the Dot Product of two vectors ( point structs )
//
//-------------------------------------------------------------------------
XFLOAT DotProduct( _In_ XPOINTF& vecA, _In_ XPOINTF& vecB );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Extends the 2D vectors ( x,y ) to 3D ( x,y,z=0) and takes the cross product.
//      Returns the Z component of the resultant vector.
//
//-------------------------------------------------------------------------
XFLOAT CrossProductZ( _In_ XPOINTF& vecA, _In_ XPOINTF& vecB );

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Rounds the number up to the next power of 2
//
//-------------------------------------------------------------------------
XUINT32 RoundUpToPow2(_In_ XUINT32 a);

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Scans from MSB to LSB for the first set bit in the mask and returns
//      it through the first param. Returns 0 iff the mask is 0.
//
//      The function has a leading Xcp to not conflict with a VC++ intrinsic
//
//-------------------------------------------------------------------------
_Success_(return != 0) unsigned char XcpBitScanReverse(_Out_ unsigned long *index, _In_ unsigned long mask);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a surface to a rectangular clip region
//      Note: Input surface is assumed to be a triangle strip configuration
//            [1]-------[2]
//             |         |
//            [3]-------[4]
//
//------------------------------------------------------------------------
_Check_return_ HRESULT ClipToRectangle(
    _In_reads_(4) const XPOINTF *pSurfaceIn,
    _In_ const XRECTF *pClipRect,
    _Deref_out_range_(4,8) XUINT32 *pSurfaceOutCount,
    _Inout_updates_(8) XPOINTF *pSurfaceOut
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Bounds the array of points
//
//------------------------------------------------------------------------
void BoundPoints(
    _In_reads_(cPoints) XPOINTF *pPoints,
    XUINT32 cPoints,
    _Out_ XRECTF *pBoundingRect
    );

XRECTF_RB BoundPoints_RB(const gsl::span<const XPOINTF>& points);
XRECTF_RB BoundPoints_RB(const gsl::span<const XPOINTF4>& points);

//------------------------------------------------------------------------
//  Method: PullTo01
//  Synopsis:
//        Clips a parameter to the interval [0,1]
//-----------------------------------------------------------------------
bool PullTo01(// Return true if r is in [0,1] within tolerance
    XDOUBLE fuzz,    // In: Computational error tolerance
    _Inout_ XDOUBLE & t);     // In/out: Pulled into [0,1], if close enough

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generates the AA masks for a point created by a clip.
//
//------------------------------------------------------------------------
void GenerateAAMasksForClipPoint(
    _In_ const PointWithAAMasks& ptA,
    _In_ const PointWithAAMasks& ptB,
    bool isPointBInsideClip,
    _Out_ PointWithAAMasks& clipPoint
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generates the AA masks for a point created by a clip. No-op
//      overload for XPOINTF4.
//
//------------------------------------------------------------------------
void GenerateAAMasksForClipPoint(
    _In_ const XPOINTF4& ptA,
    _In_ const XPOINTF4& ptB,
    bool isPointBInsideClip,
    _In_ XPOINTF4& clipPoint
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Lines AB and CD intersect at X. Find the scale AX/AB. This scale
//      can then be multiplied into AB and added to A to get the point
//      of intersection X.
//
//------------------------------------------------------------------------
bool GetScaleFactor(
    const XPOINTF& vecBA,
    const XPOINTF& vecCD,
    const XPOINTF& vecCA,
    XDOUBLE fuzz,
    _Inout_ XDOUBLE& scale
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if a line segment intersects an infinite line and returns
//      the point of intersection.  Algorithm same as IntersectSegments, but
//      without clamping the second line to a segment.
//
//------------------------------------------------------------------------
_Success_(return != 0) bool IntersectSegmentWithLine( // Return true if there is an intersection
    const XPOINTF & A,    // In: Segment's start
    const XPOINTF & B,    // In: Segment's end
    const XPOINTF & C,    // In: A point on the infinite line
    const XPOINTF & D,    // In: A second point on the infinite line
    bool isPointBInside, // In: Whether point B (segment's end) is inside the clipped polygon
    XDOUBLE fuzz,         // In: Computational error tolerance
    _Out_ XPOINTF & result);   // Out: Point of intersection

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines if a line segment intersects an infinite line and returns
//      the point of intersection. Algorithm same as IntersectSegments, but
//      without clamping the second line to a segment. Used for projection.
//      Interpolates the z value and the w value as well.
//
//      Unlike Lerp3DPoint, this method needs to account for w when
//      interpolating. For the following points on the XZ plane:
//
//    Z ^
//      |
//   10 +                   B = (10, 0, 10, 10)
//    9 +                   :
//    8 +                   :
//    7 +                   :
//    6 +                   :
//    5 +                   :
//    4 +                   :
//    3 +                   :
//    2 +                   :
//    1 +                   A = (10, 0, 1, 1)
//    0 +-+-+-+-+-+-+-+-+-+-+---->
//      0 1 2 3 4 5 6 7 8 9 10   X
//
//      Point A is (x, y, z, w) = (10, 0, 1, 1), and point B is (10, 0, 10, 10).
//      Divide out their w values to get the world space points. A remains at
//      (10, 0, 1, 1), and B becomes point b at (1, 0, 1, 1):
//
//    Z ^
//      |
//   10 +                   B
//    9 +                 .'
//    8 +               .'
//    7 +             .'
//    6 +           .'
//    5 +         .'
//    4 +       .'
//    3 +     .'
//    2 +   .'
//    1 + b-----------------a
//    0 +-+-+-+-+-+-+-+-+-+-+---->
//      0 1 2 3 4 5 6 7 8 9 10   X
//
//      The edge AB in 3D space will be drawn as the edge ab on the screen, with
//      a at (x, y) = (10, 0) and b at (1, 0).
//
//      Now clip off the world space points to the left of the line X = 2:
//
//    Z ^
//      |   |
//   10 +   |               B
//    9 +   |             .'
//    8 +   |           .'
//    7 +   |         .'
//    6 +   |       .'
//    5 +   |     .'
//    4 +   |   .'
//    3 +   | .'
//    2 +   |'
//    1 + b c---------------a
//    0 +-+-+-+-+-+-+-+-+-+-+---->
//      0 1 2 3 4 5 6 7 8 9 10   X
//          |
//
//      Point b is outside the clip. The edge ab has been clipped to become edge
//      ac, with c at (2, 0, 1, ?). We need to find the point C on AB that will
//      become point c after its perspective has been divided out:
//
//    Z ^
//      |
//   10 +                   B
//    9 +                   :
//    8 +                   :
//    7 +                   :
//    6 +                   :
//    5 +                  .C = (10, 0, 5, 5)
//    4 +              . '  :
//    3 +          . '      :
//    2 +      . '          :
//    1 +   c---------------A
//    0 +-+-+-+-+-+-+-+-+-+-+---->
//      0 1 2 3 4 5 6 7 8 9 10   X
//
//      In this case, C is (10, 0, 5, 5).
//
//      Lerp3DPoint clips the untransformed points AB, so it can do
//      linear interpolation. Here we're clipping the world space points
//      ab and need to find C, so we do the extra math.
//
//------------------------------------------------------------------------
template <typename PointType>
_Success_(return != 0) bool IntersectSegmentWithLine( // Return true if there is an intersection
    const PointType& ptA,    // In: Segment's start
    const PointType& ptB,    // In: Segment's end
    const XPOINTF& ptC,     // In: A point on the infinite line
    const XPOINTF& ptD,     // In: A second point on the infinite line
    bool isPointBInside,   // In: Whether point B (segment's end) is inside the clipped polygon
    XDOUBLE fuzz,           // In: Computational error tolerance
    _Out_ PointType& result) // Out: Point of intersection
{
    //
    // In order to keep the perspective texturing correct, we can't just linearly interpolate
    // x, y, z, and w. We'll have to calculate x/w, y/w, z/w, and 1/w, and interpolate those.
    //
    XPOINTF4 ptA_OverW = {
        ptA.x / ptA.w,
        ptA.y / ptA.w,
        ptA.z / ptA.w,
        1 / ptA.w
        };
    XPOINTF4 vecBA_OverW = {
        ptB.x/ptB.w - ptA_OverW.x,
        ptB.y/ptB.w - ptA_OverW.y,
        ptB.z/ptB.w - ptA_OverW.z,
        1/ptB.w - ptA_OverW.w
        };

    XPOINTF vecBA2 = {vecBA_OverW.x, vecBA_OverW.y};
    XPOINTF vecCD = {ptC.x - ptD.x, ptC.y - ptD.y};
    XPOINTF vecCA = {ptC.x - ptA_OverW.x, ptC.y - ptA_OverW.y};
    XDOUBLE s = 0.0f;

    if (!GetScaleFactor(vecBA2, vecCD, vecCA, fuzz, s))
    {
        return false;
    }

    XPOINTF4 combined_OverW = ptA_OverW + vecBA_OverW * s;

    // Convert back to x/y/w
    result.x = combined_OverW.x / combined_OverW.w;
    result.y = combined_OverW.y / combined_OverW.w;
    result.z = combined_OverW.z / combined_OverW.w;
    result.w = 1 / combined_OverW.w;

    GenerateAAMasksForClipPoint(ptA, ptB, isPointBInside, result);

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Takes a collection of points and rearranges them such that they
//      are in counterclockwise winding order.
//
//------------------------------------------------------------------------
bool EnsureCounterClockwiseWindingOrder(const gsl::span<XPOINTF>& points);
bool EnsureCounterClockwiseWindingOrder(const gsl::span<XPOINTF4>& points);
template <typename PointType>
bool
EnsureCounterClockwiseWindingOrder(
    _In_ XUINT32 cPoints,
    _Inout_ PointType *pPoints
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not the given point is inside the given polygon.
//      The polygon is assumed to be convex and in counter-clockwise winding
//      order.
//
//------------------------------------------------------------------------
bool
IsPointInsidePolygon(
    _In_ const XPOINTF& testPoint,
    XUINT32 numPoints,
    _In_reads_(numPoints) const XPOINTF *pPolygonPoints
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a polygon with a polygon clip.  Both polygons are assumed
//      to be convex and the clip points are in clockwise winding order.
//
//      PointType will be XPOINTF for 2D clipping. This method is also
//      used for geometry generation with projections, which will use
//      XPOINTF4 for PointType. The clips for projections will all be
//      2D, but the w value needs to be preserved for correct perspective
//      texturing.
//
//------------------------------------------------------------------------
template <typename PointType>
_Check_return_ HRESULT
ClipToPolygon(
    XUINT32 cSubjectPolygon,
    _In_reads_(cSubjectPolygon) const PointType *ptSubjectPolygon,
    XUINT32 cClipPolygon,
    _In_reads_(cClipPolygon) const XPOINTF *ptClipPolygon,
    _Out_ XUINT32 *pcOutputPolygon,
    _Inout_updates_(*pcOutputPolygon) PointType *ptOutputPolygon
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the length of a vector.
//
//------------------------------------------------------------------------
XFLOAT
Length(_In_ const XPOINTF& vector);

//------------------------------------------------------------------------
//
//  Synopsis:
//     Produces the normalized vector of the vector provided
//
//------------------------------------------------------------------------
XPOINTF
Normalize(_In_ const XPOINTF& vector);

//------------------------------------------------------------------------
//
//  Synopsis:
//     Compares two matricies to determine if the both matricies have common axes.  The axes are described
// by the set of row vectors of each matrix.  If both matrices have sets of vectors that are aligned with the vectors of
// the opposing matrix then they are axis aligned.  Even matrices with axes that overlap can pass this test because the clip
// is degenerate and will entirely clip containing content.

//
//------------------------------------------------------------------------
bool
AreMatricesAxisAligned(_In_ const CMILMatrix* pMatrixA, _In_ const CMILMatrix* pMatrixB);

// Efficient way of divding two integers and rounding up the result
// T must be any integer type.  This does not avoid overflows.
template<typename T>
T DivideIntAndRoundUp(T dividend, T divisor)
{
    return (dividend + divisor - 1) / divisor;
}

XCORNERRADIUS CornerRadius(float uniform);

XTHICKNESS Thickness(float uniform);

XTHICKNESS SubtractThickness(_In_ const XTHICKNESS& first, _In_ const XTHICKNESS& second);

XPOINTF ConvertPOINTToXPOINTF(_In_ const POINT& point);
POINT ConvertXPOINTFToPOINT(_In_ const XPOINTF& pointf);

XRECTF EnlargeRectByThickness(_In_ const XRECTF& bounds, _In_ const XTHICKNESS& margin);
XRECTF ShrinkRectByThickness(_In_ const XRECTF& bounds, _In_ const XTHICKNESS& margin);

bool IsRectContainedBy(_In_ const XRECTF& r, _In_ const XRECTF& container);

//Converts an XRECTF into an array of 4 points (in counter-clockwise order)
void RectToPoint(_In_ const XRECTF_RB& rect, _Inout_updates_(4) XPOINTF* points);

float ConvertRadianToDegrees(_In_ float radians);
float ConvertDegreesToRadian(_In_ float degrees);

void ScaleRect(_Inout_ XRECTF_RB& rect, float factor);

XRECTF ConvertRectToXRectF(_In_ const wf::Rect& rect);

XRECTF_RB ConvertRectToXRectFRB(
    _In_ wf::Rect& rect);

wf::Rect ConvertXRectFRBToRect(
    _In_ XRECTF_RB& rect);

XRECTF ConvertRectToXRECTF(const wf::Rect& rect);
wf::Rect ConvertXRECTFToRect(const XRECTF& rect);
