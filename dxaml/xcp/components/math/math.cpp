// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "real.h"

#define PI_F 3.1415926f
#define SWAP(temp, a, b) { temp = a; a = b; b = temp; }

//-------------------------------------------------------------------------
//
//  Synopsis:  Round num to the closest power of 2 that is equal or greater
//             than num.  Input must be greater than 0 and less than equal
//             to (1 <<31)
//
//-------------------------------------------------------------------------
XUINT32
RoundToPow2(_In_ XUINT32 num)
{

    ASSERT(num != 0, L"Zero passed to RoundToPow2");
    ASSERT(num <= (1U << 31), L"Num passed to RoundToPow2 is too high");

#if defined(_X86_) || defined(_X86_MATH_)

    __asm {
        bsr ecx, num    // Scan for high bit
        mov eax, 1      // Prepare eax to receive 1 << power
        shl eax, cl     // Get less than or equal power of 2
        cmp eax, num    // Check if num is greater
        setne cl        // Set to 1 if greater, 0 if equal
        shl eax, cl     // x2 if needed
    }

#else

// This function is currently used to round up surface
// sizes.  They are normally not more than 2^10 so start
// the high bit scan there if so.  Otherwise start at
// bit 31.
#define INITIAL_HIGH_BIT    10

    XUINT32    Pow2 = (1 << INITIAL_HIGH_BIT);

#if (INITIAL_HIGH_BIT != 31)
    {
        if (num > Pow2)
        {
            Pow2 = ((XUINT32) 1 << 31);
        }
    }
#endif //(INITIAL_HIGH_BIT != 31)

// Scan for highest set bit
    while (!(num & Pow2))
    {
        Pow2 >>= 1;
    }

// If num isn't a power of two round up
    if (num != Pow2)
    {
        Pow2 <<= 1;
    }

    return Pow2;

#endif // defined(_X86_) || defined(_X86_MATH_)
}

//-------------------------------------------------------------------------
//
//  Synopsis:  Returns the distance between 2 MILPointF points
//
//-------------------------------------------------------------------------
XFLOAT
Distance(_In_ XPOINTF pt1, _In_ XPOINTF pt2)
{
    return sqrtf( (pt1.x - pt2.x)*(pt1.x - pt2.x) + (pt1.y - pt2.y)*(pt1.y - pt2.y) );
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Returns a floating point mod.
//
//     This definition assumes y > 0.
//     GpModF(x, Inf) = x, as long as x != Inf.
//
//-------------------------------------------------------------------------
XDOUBLE
XcpMod(XDOUBLE x, XDOUBLE y)
{
    ASSERT(y > 0);

    XDOUBLE rMod = 0;

    if (x >= 0)
    {
        if (x < y)
        {
            rMod = x;
        }
        else
        {
            rMod = x - (XcpFloor(x/y))*y;
        }
    }
    else
    {
        // x is negative or NaN
        x *= -1;

        if (x < y)
        {
            rMod = x;
        }
        else
        {
            rMod = x - (XcpFloor(x/y))*y;
        }

        rMod = y - rMod;
    }

    return MIN(MAX(rMod,0.0), y);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Returns a floating point mod.
//
//-------------------------------------------------------------------------
XFLOAT
XcpModF(XFLOAT x, XFLOAT y)
{
    return static_cast<XFLOAT>(XcpMod(x, y));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a vector between two points.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE XPOINTF PointSubtract(
    _In_ const XPOINTF& from,
    _In_ const XPOINTF& to
    )
{
    return to - from;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a vector between two points. The z value of the destination
//      point is ignored. Used for projection geometry generation.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE XPOINTF PointSubtract(
    _In_ const XPOINTF& from,
    _In_ const XPOINTF4& to
    )
{
    XPOINTF vector;
    vector.x = to.x / to.w - from.x;
    vector.y = to.y / to.w - from.y;
    return vector;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a vector between two points. The z value of the destination
//      point is ignored. Used for projection geometry generation.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE XPOINTF PointSubtract(
    _In_ const XPOINTF& from,
    _In_ const PointWithAAMasks& to
    )
{
    XPOINTF vector;
    vector.x = to.x / to.w - from.x;
    vector.y = to.y / to.w - from.y;
    return vector;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a vector between two points. The z value of both points are
//      ignored. Used for projection geometry generation.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE XPOINTF PointSubtract(
    _In_ const XPOINTF4& from,
    _In_ const XPOINTF4& to
    )
{
    XPOINTF vector;
    vector.x = to.x / to.w - from.x / from.w;
    vector.y = to.y / to.w - from.y / from.w;
    return vector;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a vector between two points. The z value of both points are
//      ignored. Used for projection geometry generation.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE XPOINTF PointSubtract(
    _In_ const PointWithAAMasks& from,
    _In_ const PointWithAAMasks& to
    )
{
    XPOINTF vector;
    vector.x = to.x / to.w - from.x / from.w;
    vector.y = to.y / to.w - from.y / from.w;
    return vector;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Internal helper to determine which side of a given edge a set
//      of points is.
//      Returns -1 if all points are on the outside,
//               1 if all are on the inside,
//               0 if there are points on both sides.
//
//------------------------------------------------------------------------
template <typename PointType>
XINT32 WhichSide(
    _In_ XUINT32 cPoly,
    _In_reads_(cPoly) const PointType *pPtPoly,
    _In_ const XPOINTF& ptCurrent,
    _In_ const XPOINTF& vecEdge )
{
    XUINT32 nPositive = 0;
    XUINT32 nNegative = 0;
    XUINT32 nZero = 0;

    //
    // The idea here is to use the sign dot product of the outward facing normal
    // of the edge on one polygon with a vector from the start of the edge to each
    // point in the other polygon.  We choose the winding order of the polygons so
    // that all positive dot products mean on the inside and all negative ones mean
    // outside - if all the points are on the same side of this edge, then they are
    // either all out or all in. If we have hits on both sides then the polygon
    // crosses the edge.
    //

    // Get the outward facing normal to the edge by transposing it
    XPOINTF vecEdgeNormal;
    vecEdgeNormal.x = vecEdge.y;
    vecEdgeNormal.y = -vecEdge.x;

    for ( XUINT32 i=0; i<cPoly; i++ )
    {
        // Get a vector from the current point in the other polygon
        // to each of the points in this polygon
        XPOINTF vecCurrent = PointSubtract(ptCurrent, pPtPoly[i]);

        // Use a DOT product to determine side
        XFLOAT rDot = DotProduct( vecCurrent, vecEdgeNormal );
        if ( rDot > 0.0f )
        {
            nPositive++;
        }
        else if ( rDot < 0.0f )
        {
            nNegative++;
        }
        else
        {
            nZero++;
        }

        // We can early out if we have points on both side of the line
        // ( meaning we crossed an edge ) or if we have a zero ( meaning
        // the edges are overlapping )
        if ( ((nPositive > 0) && (nNegative > 0))
             || (nZero > 0) )
        {
            return 0;
        }
    }

    // We went through the entire polygon - we either had all in or all out
    // or we would have returned sooner
    return ( nPositive > 0 ) ? 1 : -1;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Does this polygon intersect the other?
//      NOTE: for this method to work, the points in both polygons
//      MUST BE wound counter-clockwise.
//
//------------------------------------------------------------------------
bool DoPolygonsIntersect(
    _In_ XUINT32 cPolyA,
    _In_reads_(cPolyA) const XPOINTF *pPtPolyA,
    _In_ XUINT32 cPolyB,
    _In_reads_(cPolyB) const XPOINTF *pPtPolyB )
{
    // Test B in A
    for ( XUINT32 i=0; i<cPolyA; i++ )
    {
        XPOINTF vecEdge = pPtPolyA[ (i+1) % cPolyA ] - pPtPolyA[i];
        if ( WhichSide( cPolyB, &pPtPolyB[0], pPtPolyA[i], vecEdge ) < 0 )
        {
            // The whole of the polygon is entirely on the outside of the edge,
            // so we can never intersect
            return false;
        }
    }

    // Test A in B
    for ( XUINT32 i=0; i<cPolyB; i++ )
    {
        XPOINTF vecEdge = pPtPolyB[ (i+1) % cPolyB ] - pPtPolyB[i];
        if ( WhichSide( cPolyA, &pPtPolyA[0], pPtPolyB[i], vecEdge ) < 0 )
        {
            // The whole of the polygon is entirely on the outside of the edge,
            // so we can never intersect
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is polygon A wholly contained inside polygon B?
//      PointType is XPOINTF for 2D and XPOINTF4 for projection clipping.
//      Ignores the z values in polygon A.
//
//      NOTE: for this method to work, the points in both polygons
//      MUST BE wound counter-clockwise.
//
//------------------------------------------------------------------------
template <typename PointType>
bool IsEntirelyContained(
        _In_ XUINT32 cPolyA,
        _In_reads_(cPolyA) const PointType *pPtPolyA,
        _In_ XUINT32 cPolyB,
        _In_reads_(cPolyB) XPOINTF *pPtPolyB )
{
    for (XUINT32 i = 0; i < cPolyB; i++)
    {
        XPOINTF vecEdge = pPtPolyB[ (i+1) % cPolyB ] - pPtPolyB[i];
        if ( WhichSide( cPolyA, &pPtPolyA[0], pPtPolyB[i], vecEdge ) <= 0 )
        {
            // The whole of the polygon is entirely on the outside of the edge,
            // so we can never intersect
            return false;
        }
    }

    return true;
}

template bool IsEntirelyContained(
        _In_ XUINT32 cPolyA,
        _In_reads_(cPolyA) const XPOINTF *pPtPolyA,
        _In_ XUINT32 cPolyB,
        _In_reads_(cPolyB) XPOINTF *pPtPolyB);

template bool IsEntirelyContained(
        _In_ XUINT32 cPolyA,
        _In_reads_(cPolyA) const XPOINTF4 *pPtPolyA,
        _In_ XUINT32 cPolyB,
        _In_reads_(cPolyB) XPOINTF *pPtPolyB);

template bool IsEntirelyContained(
        _In_ XUINT32 cPolyA,
        _In_reads_(cPolyA) const PointWithAAMasks *pPtPolyA,
        _In_ XUINT32 cPolyB,
        _In_reads_(cPolyB) XPOINTF *pPtPolyB);

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is the polygon concave?
//      NOTE: for this method to work, the points MUST BE wound
//      counter-clockwise.
//
//------------------------------------------------------------------------
template <typename PointType> bool IsPolygonConcave_TemplatedHelper(const gsl::span<const PointType>& points)
{
    if (points.size() <= 3)
    {
        return false;
    }

    for (int i = 0; i < points.size(); i++)
    {
        int j = (i + 1) % points.size();
        int k = (i + 2) % points.size();

        XPOINTF vec1 = PointSubtract(points[i], points[j]);
        XPOINTF vec2 = PointSubtract(points[k], points[j]);

        if (CrossProductZ(vec1, vec2) < 0.0f)
        {
            return true;
        }
    }

    return false;
}

bool IsPolygonConcave(const gsl::span<const XPOINTF>& points)
{
    return IsPolygonConcave_TemplatedHelper(points);
}

bool IsPolygonConcave(const gsl::span<const XPOINTF4>& points)
{
    return IsPolygonConcave_TemplatedHelper(points);
}

template <typename PointType> bool IsPolygonConcave(XUINT32 cPoints, _In_reads_(cPoints) const PointType *pPoints)
{
    const gsl::span<const PointType> points(pPoints, cPoints);
    return IsPolygonConcave_TemplatedHelper(points);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a surface to a rectangular clip region
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ClipToRectangle(
    _In_reads_(4) const XPOINTF *pSurfaceIn,
    _In_ const XRECTF *pClipRect,
    _Deref_out_range_(4,8) XUINT32 *pSurfaceOutCount,
    _Inout_updates_(8) XPOINTF *pSurfaceOut
    )
{
    const XUINT32 cMaxPointsArraySize = 8;
    XPOINTF aSurface1[cMaxPointsArraySize];
    XPOINTF aSurface2[cMaxPointsArraySize];
    XUINT32 cSurface1 = 0;
    XUINT32 cSurface2 = 0;

    aSurface1[cSurface1++] = pSurfaceIn[0];
    aSurface1[cSurface1++] = pSurfaceIn[1];
    aSurface1[cSurface1++] = pSurfaceIn[2];
    aSurface1[cSurface1++] = pSurfaceIn[3];

#pragma warning (push)
#pragma warning (disable : 26014)
    //
    // aSurface1 and aSurface2 are scratch surfaces which are used for generating the points of intersection
    // given a clipping rectangle edge and the surface. The surface can generate an additional point of
    // intersection given a rectangle edge and hence the expectation is that in each stage of intersection
    // there are N+1 points generated given N points as input.
    //
    // The worst case scenario detected by Prefast is incorrect due to the above input/output relationship.
    //

    // Compare with left edge
    cSurface2 = 0;
    for (XUINT32 i = 0; i < cSurface1; i++)
    {
        XINT32 iPlus1 = (i+1) % cSurface1;
        if (aSurface1[iPlus1].x > pClipRect->X)
        {
            // Both points inside, keep both
            if (aSurface1[i].x > pClipRect->X)
            {
                ASSERT(cSurface2 < cMaxPointsArraySize);
                aSurface2[cSurface2++] = aSurface1[iPlus1];
            }

            // Starting point outside, ending point inside
            else
            {
                ASSERT((cSurface2 + 1) < cMaxPointsArraySize);
                aSurface2[cSurface2].x = pClipRect->X;
                aSurface2[cSurface2++].y = aSurface1[i].y + (pClipRect->X - aSurface1[i].x) * (aSurface1[iPlus1].y - aSurface1[i].y) / (aSurface1[iPlus1].x - aSurface1[i].x);
                aSurface2[cSurface2++] = aSurface1[iPlus1];
            }
        }
        else
        {
            // Starting point inside, ending point outside
            // If it's on the line, we exclude it here
            if (aSurface1[i].x > pClipRect->X)
            {
                ASSERT(cSurface2 < cMaxPointsArraySize);
                aSurface2[cSurface2].x = pClipRect->X;
                aSurface2[cSurface2++].y = aSurface1[iPlus1].y + (pClipRect->X - aSurface1[iPlus1].x) * (aSurface1[i].y - aSurface1[iPlus1].y) / (aSurface1[i].x - aSurface1[iPlus1].x);
            }
        }
    }

    ASSERT(cSurface2 <= (cSurface1 + 1));

    // Compare with top edge
    cSurface1 = 0;
    for (XUINT32 i = 0; i < cSurface2; i++)
    {
        XINT32 iPlus1 = (i+1) % cSurface2;
        if (aSurface2[iPlus1].y > pClipRect->Y)
        {
            // Both points inside, keep both
            if (aSurface2[i].y > pClipRect->Y)
            {
                ASSERT(cSurface1 < cMaxPointsArraySize);
                aSurface1[cSurface1++] = aSurface2[iPlus1];
            }

            // Starting point outside, ending point inside
            else
            {
                ASSERT((cSurface1 + 1) < cMaxPointsArraySize);
                aSurface1[cSurface1].y = pClipRect->Y;
                aSurface1[cSurface1++].x = aSurface2[i].x + (pClipRect->Y - aSurface2[i].y) * (aSurface2[iPlus1].x - aSurface2[i].x) / (aSurface2[iPlus1].y - aSurface2[i].y);
                aSurface1[cSurface1++] = aSurface2[iPlus1];
            }
        }
        else
        {
            // Starting point inside, ending point outside
            if (aSurface2[i].y > pClipRect->Y)
            {
                ASSERT(cSurface1 < cMaxPointsArraySize);
                aSurface1[cSurface1].y = pClipRect->Y;
                aSurface1[cSurface1++].x = aSurface2[iPlus1].x + (pClipRect->Y - aSurface2[iPlus1].y) * (aSurface2[i].x - aSurface2[iPlus1].x) / (aSurface2[i].y - aSurface2[iPlus1].y);
            }
        }
    }

    ASSERT(cSurface1 <= (cSurface2 + 1));

    // Compare with right edge
    cSurface2 = 0;
    for (XUINT32 i = 0; i < cSurface1; i++)
    {
        XINT32 iPlus1 = (i+1) % cSurface1;
        if (aSurface1[iPlus1].x < pClipRect->X + pClipRect->Width)
        {
            // Both points inside, keep both
            if (aSurface1[i].x < pClipRect->X + pClipRect->Width)
            {
                ASSERT(cSurface2 < cMaxPointsArraySize);
                aSurface2[cSurface2++] = aSurface1[iPlus1];
            }

            // Starting point outside, ending point inside
            else
            {
                ASSERT((cSurface2 + 1) < cMaxPointsArraySize);
                aSurface2[cSurface2].x = pClipRect->X + pClipRect->Width;
                aSurface2[cSurface2++].y = aSurface1[iPlus1].y + (pClipRect->X + pClipRect->Width - aSurface1[iPlus1].x) * (aSurface1[i].y - aSurface1[iPlus1].y) / (aSurface1[i].x - aSurface1[iPlus1].x);
                aSurface2[cSurface2++] = aSurface1[iPlus1];
            }
        }
        else
        {
            // Starting point inside, ending point outside
            if ( aSurface1[i].x < pClipRect->X + pClipRect->Width )
            {
                ASSERT( cSurface2 < cMaxPointsArraySize );
                aSurface2[cSurface2].x = pClipRect->X + pClipRect->Width;
                aSurface2[cSurface2++].y = aSurface1[i].y + (pClipRect->X + pClipRect->Width - aSurface1[i].x) * (aSurface1[iPlus1].y - aSurface1[i].y) / (aSurface1[iPlus1].x - aSurface1[i].x);
            }
        }
    }

    ASSERT(cSurface2 <= (cSurface1 + 1));

    // Compare with bottom edge
    cSurface1 = 0;
    for (XUINT32 i = 0; i < cSurface2; i++)
    {
        XINT32 iPlus1 = (i+1) % cSurface2;
        if (aSurface2[iPlus1].y < pClipRect->Y + pClipRect->Height)
        {
            // Both points inside, keep both
            if (aSurface2[i].y < pClipRect->Y + pClipRect->Height)
            {
                ASSERT(cSurface1 < cMaxPointsArraySize);
                aSurface1[cSurface1++] = aSurface2[iPlus1];
            }

            // Starting point outside, ending point inside
            else
            {
                ASSERT((cSurface1 + 1) < cMaxPointsArraySize);
                aSurface1[cSurface1].y = pClipRect->Y + pClipRect->Height;
                aSurface1[cSurface1++].x = aSurface2[iPlus1].x + (pClipRect->Y + pClipRect->Height - aSurface2[iPlus1].y) * (aSurface2[i].x - aSurface2[iPlus1].x) / (aSurface2[i].y - aSurface2[iPlus1].y);
                aSurface1[cSurface1++] = aSurface2[iPlus1];
            }
        }
        else
        {
            // Starting point inside, ending point outside
            if (aSurface2[i].y < pClipRect->Y + pClipRect->Height)
            {
                ASSERT( cSurface1 < cMaxPointsArraySize );
                aSurface1[cSurface1].y = pClipRect->Y + pClipRect->Height;
                aSurface1[cSurface1++].x = aSurface2[i].x + (pClipRect->Y + pClipRect->Height - aSurface2[i].y) * (aSurface2[iPlus1].x - aSurface2[i].x) / (aSurface2[iPlus1].y - aSurface2[i].y);
            }
        }
    }

    ASSERT(cSurface1 <= (cSurface2 + 1));

    // Reducing the duplicate entries that are produced by the algorithm
    XUINT32 cShift = 0;
    XUINT32 cSurface1Total = cSurface1;
    for (XUINT32 i = 0; i < cSurface1; i++)
    {
        if (aSurface1[i].x == aSurface1[(i + 1) % cSurface1].x && aSurface1[i].y == aSurface1[(i + 1) % cSurface1].y)
        {
            cShift++;
            cSurface1--;
        }
        if (i+cShift < cSurface1Total)
        {
            aSurface1[i] = aSurface1[i + cShift];
        }
    }

    IFCEXPECT_ASSERT_RETURN(cSurface1 <= cMaxPointsArraySize);

    // Returning points
    *pSurfaceOutCount = cSurface1;
    for (XUINT32 i = 0; i < cSurface1; i++)
    {
        pSurfaceOut[i] = aSurface1[i];
    }

#pragma warning (pop)

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Inflates a rectangle to be integer aligned.
//
//------------------------------------------------------------------------
void
InflateRectF(_Inout_ XRECTF_RB* a)
{
    a->left = static_cast<XFLOAT>(floorf(a->left));
    a->right = static_cast<XFLOAT>(ceilf(a->right));

    a->top = static_cast<XFLOAT>(floorf(a->top));
    a->bottom = static_cast<XFLOAT>(ceilf(a->bottom));
}

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
    )
{
    const gsl::span<const XPOINTF> points(pPoints, cPoints);
    XRECTF_RB bounds = BoundPoints_RB(points);
    *pBoundingRect = ToXRectF(bounds);
}

XRECTF_RB BoundPoints_RB(const gsl::span<const XPOINTF>& points)
{
    XRECTF_RB bounds = {};

    if (points.size() > 0)
    {
        // Initialize bounds to the first point.
        bounds.left = bounds.right = points[0].x;
        bounds.top = bounds.bottom = points[0].y;

        // Increase bounds to include each successive point.
        for (int i = 1; i < points.size(); i++)
        {
            bounds.left = MIN(bounds.left, points[i].x);
            bounds.right = MAX(bounds.right, points[i].x);

            bounds.top = MIN(bounds.top, points[i].y);
            bounds.bottom = MAX(bounds.bottom, points[i].y);
        }
    }

    return bounds;
}

XRECTF_RB BoundPoints_RB(const gsl::span<const XPOINTF4>& points)
{
    XRECTF_RB bounds = {};

    if (points.size() > 0)
    {
        // Initialize bounds to the first point.
        bounds.left = bounds.right = points[0].x / points[0].w;
        bounds.top = bounds.bottom = points[0].y / points[0].w;

        // Increase bounds to include each successive point.
        for (int i = 1; i < points.size(); i++)
        {
            bounds.left = MIN(bounds.left, points[i].x / points[i].w);
            bounds.right = MAX(bounds.right, points[i].x / points[i].w);

            bounds.top = MIN(bounds.top, points[i].y / points[i].w);
            bounds.bottom = MAX(bounds.bottom, points[i].y / points[i].w);
        }
    }

    return bounds;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Takes a collection of points and rearranges them such that they
//      are in counterclockwise winding order. Returns whether the order
//      had to be flipped.
//
//      Note: Assumes that the points had a consistent winding order to
//      begin with.
//
//------------------------------------------------------------------------
template <typename PointType> XCP_FORCEINLINE bool EnsureCounterClockwiseWindingOrder_TemplatedHelper(const gsl::span<PointType>& points)
{
    // Ensure that the vertices have clockwise winding order
    if (points.size() > 2)
    {
        XPOINTF vec1 = PointSubtract(points[0], points[1]);
        XPOINTF vec2 = PointSubtract(points[1], points[2]);
        auto lastIndex = points.size() - 1;

        // If the cross-product is positive (i.e. the winding is CW)
        if ( vec1.x * vec2.y - vec2.x * vec1.y > 0.0f )
        {
            // Reverse the vertex order
            for ( int i = 0; i < points.size()/2; i++ )
            {
                // Swap
                PointType ptTemp = points[i];
                points[i] = points[lastIndex-i];
                points[lastIndex-i] = ptTemp;
            }
            return true;
        }
    }

    return false;
}

bool EnsureCounterClockwiseWindingOrder(const gsl::span<XPOINTF>& points)
{
    return EnsureCounterClockwiseWindingOrder_TemplatedHelper(points);
}

bool EnsureCounterClockwiseWindingOrder(const gsl::span<XPOINTF4>& points)
{
    return EnsureCounterClockwiseWindingOrder_TemplatedHelper(points);
}

template <typename PointType>
XCP_FORCEINLINE bool
EnsureCounterClockwiseWindingOrder(
    _In_ XUINT32 cPoints,
    _Inout_ PointType *pPoints)
{
    const gsl::span<PointType> points(pPoints, cPoints);
    return EnsureCounterClockwiseWindingOrder_TemplatedHelper(points);
}

template bool EnsureCounterClockwiseWindingOrder(
    _In_ XUINT32 cPoints,
    _Inout_ XPOINTF *pPoints);

template bool EnsureCounterClockwiseWindingOrder(
    _In_ XUINT32 cPoints,
    _Inout_ XPOINTF4 *pPoints);

template bool EnsureCounterClockwiseWindingOrder(
    _In_ XUINT32 cPoints,
    _Inout_ PointWithAAMasks *pPoints);


//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the polar angle of the given vector, counterclockwise,
//      relative to (-1,0). Used by ComputeConvexHull. Returns a value in
//      the interval [0,2pi].
//
//------------------------------------------------------------------------
XFLOAT GetCounterClockwisePolarAngle(_In_ const XPOINTF& vector)
{
    ASSERT(vector.x != 0.0f || vector.y != 0.0f);

    XFLOAT polarAngle = atan2f(vector.y, -vector.x);
    if (polarAngle < 0.0f)
    {
        //
        // ArcTan2 returns values within the interval (-pi,pi]. Map negative values to (pi,2pi].
        //
        // Note: We're adding 2pi to the negative values in the range (-pi,0). Therefore we should
        // get angles in the range (pi,2pi). However, a very small negative angle + 2pi can give
        // exactly 2pi due to floating point error, so in practice the negative values get mapped
        // to (pi,2pi].
        //
        // An alternative is to clamp polarAngle to 0.0f using IsCloseReal. But this won't work
        // because ComputeConvexHull is expecting the angles associated with each new edge to
        // increase. For a vector, the angle we use is the counterclockwise angle between (-1,0)
        // and the vector:
        //
        //                     -y
        //                      |
        //                      |
        //                      |
        //                      |
        //                      |
        //                      |
        //                      |
        //    -x ---------------+---------------- +x
        //               angle /|
        //                    / |
        //                   /  |
        //                  /   |
        //                 /    |
        //             vector   |
        //                      |
        //                     +y
        //
        // So for this case:
        //
        //   +------------1------------------------------ x
        //   |                               5
        //   |
        //   |                                   4
        //   |
        //   |
        //   |      2
        //   |
        //   |
        //   |                             3
        //   |
        //   |
        //   y
        //
        // The points were added in the order {1,2,3,4,5}, with the counterclockwise angle between
        // each pair increasing. The final edge {5,1} has a very small negative angle. The previous
        // angle is from the edge {4,5}, which has an angle of about 7pi/4. If we clamp the angle
        // of {5,1} to 0, then we won't find any edge with an angle greater than 7pi/4 to close the
        // hull. We have to return 2pi as the angle for {5,1}.
        //
        // Allowing 2pi could cause points to skipped. In the case above, suppose that we've added
        // points {1,2,3,4}, and we're looking for the next point. Suppose that the negative angles
        // for {4,5} and {4,1} were both small enough to be adjusted to 2pi. ComputeConvexHull will
        // then take the point that's farther away from 4, which is 1. We then skipped point 5, even
        // though it's on the convex hull. This case is acceptable because the edges {4,5} and {4,1}
        // would be effectively collinear anyway.
        //
        // Allowing 2pi should never cause a concave polygon to be accepted as convex. We only
        // adjust small _negative_ values, so angles that should have been (2pi-epsilon) become
        // 2pi instead. We only ever make the angle bigger.
        //
        polarAngle += 2.0f * PI_F;
    }

    ASSERT(0.0f <= polarAngle && polarAngle <= 2.0f * PI_F);
    return polarAngle;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the convex hull of an arbitrary polygon.
//
//  Notes:
//      - Uses a simple gift wrapping algorithm, which runs in O(nh) time,
//        where n is the number of points and h is the number of points on
//        the convex hull. If performance becomes a concern, we can consider
//        more complex algorithms that run in O(n log n) or O(n log h) time.
//      - No assumptions are made about the input points.
//      - Input and output buffers pointers can be the same, in which case
//        extra copying does not take place; however, partially overlapping
//        buffers are not supported.
//
//------------------------------------------------------------------------
template <typename PointType>
void ComputeConvexHull(
    _In_ const XUINT32 numPoints,
    _In_reads_(numPoints) const PointType *pPoints,
    _Out_ XUINT32 *pNumHullPoints,
    _Out_writes_to_(numPoints, *pNumHullPoints) PointType *pHullPoints)
{
    if (pPoints != pHullPoints && numPoints > 0)
    {
        // If we are given different buffers, we copy everything over to the output buffer first.
        memcpy(pHullPoints, pPoints, numPoints * sizeof(PointType));
    }

    if (numPoints <= 3)
    {
        *pNumHullPoints = numPoints;
        return;
    }

    //
    // Find the point with the minimum y-coordinate and put it in pPoints[0]. This point is guaranteed
    // to be on the convex hull. In case of a tie, we pick the rightmost point. This is important to
    // ensure that we get progressively larger polar angles as we move counterclockwise as required
    // by the algorithm below. If we didn't pick the rightmost point in case of a tie, we could
    // potentially get a polar angle of 0 along the top edge of the input polygon, and we'd get to it
    // last.
    //

    for (XUINT32 i = 1; i < numPoints; i++)
    {
        XPOINTF delta = PointSubtract(pHullPoints[0], pHullPoints[i]); // Take the delta to also handle the XPOINTF4 case.

        if (delta.y < 0.0f || (delta.y == 0.0f && delta.x > 0.0f))
        {
            PointType temp;
            SWAP(temp, pHullPoints[0], pHullPoints[i]);
        }
    }

    //
    // Loop through the points that lie on the convex hull until we get back around to the first
    // point again.
    //

    XUINT32 numHullPoints = 1;
    XFLOAT lastPolarAngle = XFLOAT_MIN;

    while (TRUE)
    {
        XFLOAT minPolarAngle = XFLOAT_MAX;
        XFLOAT maxDistance = 0.0f;

        XUINT32 lastHullPointIndex = numHullPoints - 1;
        XUINT32 nextHullPointIndex = ~0;

        //
        // Find the point with the smallest polar angle relative to the last point on the convex
        // hull, where the polar angle is larger than the polar angle for the last hull edge. Such
        // a point must lie on the convex hull.
        //
        for (XUINT32 i = 0; i < numPoints; i++)
        {
            // The next edge of the convex hull can't connect to any point along the convex hull
            // except the one we started at. The points of the convex hull that we've computed
            // so far are in pHullPoints[0..lastHullPointIndex]. Out of those points, the only one
            // we need to check is pHullPoints[0], since such an edge will terminate the outer loop.
            if ((i == 0 && lastHullPointIndex != 0) || i > lastHullPointIndex)
            {
                XPOINTF candidateHullEdge = PointSubtract(pHullPoints[lastHullPointIndex], pHullPoints[i]);
                XFLOAT distance = Length(candidateHullEdge);

                if (distance > 0.0f) // Skip duplicates.
                {
                    XFLOAT polarAngle = GetCounterClockwisePolarAngle(candidateHullEdge);

                    // In case of a tie, we pick the point with the largest distance. This ensures that
                    // we don't put redundant collinear points on the hull.
                    if (polarAngle > lastPolarAngle && (polarAngle < minPolarAngle || (polarAngle == minPolarAngle && distance > maxDistance)))
                    {
                        minPolarAngle = polarAngle;
                        maxDistance = distance;
                        nextHullPointIndex = i;
                    }
                }
            }
        }

        if (nextHullPointIndex == ~0)
        {
            //
            // A floating point error made us unable to find a next point in this iteration, because
            // the next largest polar angle was not larger than the current largest polar angle. If
            // we continue, we'll swap something outside the buffer into the buffer. We can't continue
            // anymore. If the points that were already added form a convex hull, then stop and return
            // them. If the current points don't form a convex hull (due to floating point errors),
            // then return no points. We'll skip rendering or hit testing this polygon.
            //
            // TODO: Switch to fixed-point math to avoid floating point errors.
            //
            if (IsPolygonConcave(numHullPoints, pHullPoints))
            {
                numHullPoints = 0;
            }
            break;
        }

        ASSERT(minPolarAngle > lastPolarAngle);
        lastPolarAngle = minPolarAngle;

        if (nextHullPointIndex == 0)
        {
            // We've wrapped around to the first hull point, so we're done.
            break;
        }

        // Add the next hull point to the hull.
        PointType temp;
        SWAP(temp, pHullPoints[numHullPoints], pHullPoints[nextHullPointIndex]);
        numHullPoints++;
    }

    *pNumHullPoints = numHullPoints;
    ASSERT(!IsPolygonConcave(numHullPoints, pHullPoints));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Used by ClipToPolygon to tell if a point is inside a clip edge or not.
//      Inside means that if the edge was part of a shape that was wound CCW,
//      the point would be inside that shape.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE bool IsInside(
    _In_ const XPOINTF& vecPoint,
    _In_ const XPOINTF& vecEdge,
    _In_ bool fExcludeZero
    )
{
    XFLOAT diff = vecPoint.x * vecEdge.y - vecEdge.x * vecPoint.y;

    return ((diff < 0 || (diff == 0 && fExcludeZero)) ? FALSE : TRUE);
}

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
    )
{
    for (XUINT32 i = 0; i < numPoints; i++)
    {
        XPOINTF vecPoint = testPoint - pPolygonPoints[i];
        XPOINTF vecEdge = pPolygonPoints[(i + 1) % numPoints] - pPolygonPoints[i];

        if (!IsInside(vecPoint, vecEdge, FALSE /*fExcludeZero*/))
        {
            return false;
        }
    }

    return true;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a polygon with a polygon clip.  Both polygons are assumed
//      to be convex and the points must be in counter-clockwise winding
//      order.
//
//      PointType will be XPOINTF for 2D clipping and PointWithAAMasks or
//      XPOINTF4 for 3D clipping. The clip polygon will always be 2D.
//
//      In the case of 3D points, we'll divide out perspective to turn
//      (x, y, z, w) into (x/w, y/w, z/w, 1), drop z and w to get the 2D
//      points (x/w, y/w), and clip the 2D points.
//
//      Note that for the 3D clipping case, we have to account for w when
//      interpolating. This is because we're clipping a projected 3D shape
//      using a 2D clip. A 3D point with perspective (x, y, z, w) gets
//      drawn on screen as the 2D point (x/w, y/w). We'll clip that point
//      to (x/w, y/w) = (ClipX, ClipY) by changing the original 3D point,
//      which means we have to take the w value into account (see the
//      IntersectSegmentWithLine overloads).
//
//      Compare this with view frustum clipping for projections, which
//      interpolates everything linearly without taking w into account
//      (see Lerp3DPoint). It doesn't need to account for w because
//      it's doing the clipping before the w is divided out.
//
//------------------------------------------------------------------------
template <typename PointType>
_Check_return_ HRESULT
ClipToPolygon(
    XUINT32 cSubjectPolygon,
    _In_reads_(cSubjectPolygon) const PointType *ptSubjectPolygon,
    XUINT32 cClipPolygon,
    _In_reads_(cClipPolygon) const XPOINTF *ptClipPolygon,
    _Inout_ XUINT32 *pcResultPolygon,
    _Out_writes_(*pcResultPolygon) PointType *ptResultPolygon
    )
{
    HRESULT hr = S_OK;
    const XUINT32 cBufferSize = *pcResultPolygon;
    PointType *ptScratch1 = NULL;
    PointType *ptScratch2 = NULL;
    PointType *ptInputPolygon = NULL;
    PointType *ptOutputPolygon = NULL;
    XUINT32 cInputPolygon = 0;
    XUINT32 cOutputPolygon = 0;

    ASSERT(!IsPolygonConcave(cSubjectPolygon, ptSubjectPolygon));
    ASSERT(!IsPolygonConcave(cClipPolygon, ptClipPolygon));

    *pcResultPolygon = 0;

    // Fail if the provided output buffer is smaller than the input buffer
    if (cSubjectPolygon + cClipPolygon > cBufferSize)
    {
        IFC(E_INVALIDARG);
    }

    // Allocate memory for scratch surfaces
    ptScratch1 = new PointType[cBufferSize];
    ptScratch2 = new PointType[cBufferSize];

    // Initialize buffer pointers
    ptInputPolygon = ptScratch1;
    ptOutputPolygon = ptScratch2;

    // Initialize the output polygon to the input polygon
    memcpy(ptOutputPolygon, ptSubjectPolygon, cSubjectPolygon * sizeof(PointType));
    cOutputPolygon = cSubjectPolygon;

    // Iterate over clip edges
    for (XUINT32 uClipPoint = 0; uClipPoint < cClipPolygon; uClipPoint++)
    {
        const XUINT32 nextClipPoint = (uClipPoint + 1 < cClipPolygon) ? uClipPoint + 1 : 0;
        XPOINTF vecClipEdge = ptClipPolygon[nextClipPoint] - ptClipPolygon[uClipPoint];

        // swap input and output buffers
        // i.e. use output from the last clip edge as the input for this clip edge
        {
            PointType *ptTemp;
            SWAP(ptTemp, ptInputPolygon, ptOutputPolygon);
        }
        cInputPolygon = cOutputPolygon;

        // reset output polygon
        cOutputPolygon = 0;

        for (XUINT32 uSubjectPoint = 0; uSubjectPoint < cInputPolygon; uSubjectPoint++)
        {
            const XPOINTF vecClipPointToCurrentPoint = PointSubtract(ptClipPolygon[uClipPoint], ptInputPolygon[uSubjectPoint]);
            const XPOINTF vecClipPointToPreviousPoint = PointSubtract(ptClipPolygon[uClipPoint], ptInputPolygon[uSubjectPoint ? uSubjectPoint - 1 : cInputPolygon - 1]);

            const bool fCurrentPointIsInsideClipEdge = IsInside(vecClipPointToCurrentPoint, vecClipEdge, TRUE /*fExcludeZero*/);
            const bool fPreviousPointIsInsideClipEdge = IsInside(vecClipPointToPreviousPoint, vecClipEdge, TRUE /*fExcludeZero*/);

            if (fCurrentPointIsInsideClipEdge ^ fPreviousPointIsInsideClipEdge)
            {
                // one point inside, one point outside

                // Find the intersection...
                PointType ptIntersection;
                if (IntersectSegmentWithLine(ptInputPolygon[uSubjectPoint ? uSubjectPoint - 1 : cInputPolygon - 1],
                                             ptInputPolygon[uSubjectPoint],
                                             ptClipPolygon[uClipPoint],
                                             ptClipPolygon[nextClipPoint],
                                             fCurrentPointIsInsideClipEdge,
                                             1e-6, // fuzz
                                             ptIntersection))
                {
                    // ...and add it to the output list
                    ptOutputPolygon[cOutputPolygon++] = ptIntersection;
                }
            }

            if (fCurrentPointIsInsideClipEdge)
            {
                // The current point is inside the clip edge

                // Add it to the output list
                ptOutputPolygon[cOutputPolygon++] = ptInputPolygon[uSubjectPoint];
            }
        }
    }

    // reset result polygon
    *pcResultPolygon = 0;

    // Remove the duplicate entries that are produced by the algorithm
    for (XUINT32 i = 0; i < cOutputPolygon; i++)
    {
        if (!(IsCloseReal(ptOutputPolygon[i].x, ptOutputPolygon[(i + 1) % cOutputPolygon].x) &&
              IsCloseReal(ptOutputPolygon[i].y, ptOutputPolygon[(i + 1) % cOutputPolygon].y)))
        {
            ptResultPolygon[(*pcResultPolygon)++] = ptOutputPolygon[i];
        }
    }

    if (IsPolygonConcave(*pcResultPolygon, ptResultPolygon))
    {
        // Due to floating point rounding error, we can sometimes get a resulting polygon
        // that is slightly concave. In those cases, we return the convex hull.
        ComputeConvexHull(*pcResultPolygon, ptResultPolygon, pcResultPolygon, ptResultPolygon);
    }

Cleanup:
    delete[] ptScratch1;
    delete[] ptScratch2;
    RRETURN(hr);
}

template _Check_return_ HRESULT ClipToPolygon(
    _In_ XUINT32 cSubjectPolygon,
    _In_reads_(cSubjectPolygon) const XPOINTF *ptSubjectPolygon,
    _In_ XUINT32 cClipPolygon,
    _In_reads_(cClipPolygon) const XPOINTF *ptClipPolygon,
    _Out_ XUINT32 *pcOutputPolygon,
    _Inout_updates_(*pcOutputPolygon) XPOINTF *ptOutputPolygon
    );

template _Check_return_ HRESULT ClipToPolygon(
    _In_ XUINT32 cSubjectPolygon,
    _In_reads_(cSubjectPolygon) const XPOINTF4 *ptSubjectPolygon,
    _In_ XUINT32 cClipPolygon,
    _In_reads_(cClipPolygon) const XPOINTF *ptClipPolygon,
    _Out_ XUINT32 *pcOutputPolygon,
    _Inout_updates_(*pcOutputPolygon) XPOINTF4 *ptOutputPolygon
    );

template _Check_return_ HRESULT ClipToPolygon(
    _In_ XUINT32 cSubjectPolygon,
    _In_reads_(cSubjectPolygon) const PointWithAAMasks *ptSubjectPolygon,
    _In_ XUINT32 cClipPolygon,
    _In_reads_(cClipPolygon) const XPOINTF *ptClipPolygon,
    _Out_ XUINT32 *pcOutputPolygon,
    _Inout_updates_(*pcOutputPolygon) PointWithAAMasks *ptOutputPolygon
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the length of a vector.
//
//------------------------------------------------------------------------
XFLOAT Length(_In_ const XPOINTF& vector)
{
    return sqrtf(vector.x * vector.x + vector.y * vector.y);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Produces the normalized vector of the vector provided.
//
//------------------------------------------------------------------------
XPOINTF
Normalize(_In_ const XPOINTF& vector)
{
    XPOINTF normal;
    XFLOAT length = Length(vector);

    if (length == 0)
    {
        return vector;
    }

    normal.x = vector.x / length;
    normal.y = vector.y / length;

    return normal;
}


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
AreMatricesAxisAligned(_In_ const CMILMatrix* pMatrixA, _In_ const CMILMatrix* pMatrixB)
{
    // Get the unit vectors of pMatrixA and pMatrixB
    XPOINTF axisA_x;
    XPOINTF axisB_x;
    XPOINTF axisB_y;

    axisA_x.x = pMatrixA->GetM11();
    axisA_x.y = pMatrixA->GetM12();

    axisB_x.x = pMatrixB->GetM11();
    axisB_x.y = pMatrixB->GetM12();

    axisB_y.x = pMatrixB->GetM21();
    axisB_y.y = pMatrixB->GetM22();

    // Check axis x of A against axes of B
    if (IsCloseReal(CrossProductZ(axisA_x, axisB_x), 0) || IsCloseReal(CrossProductZ(axisA_x, axisB_y), 0))
    {
        XPOINTF axisA_y;
        axisA_y.x = pMatrixA->GetM21();
        axisA_y.y = pMatrixA->GetM22();

        // Check axis y of A against axes of B
        if (IsCloseReal(CrossProductZ(axisA_y, axisB_x), 0) || IsCloseReal(CrossProductZ(axisA_y, axisB_y), 0))
        {
            return true;
        }
    }

    return false;
}

XPOINTF ConvertPOINTToXPOINTF(_In_ const POINT& point)
{
    XPOINTF pointf = {static_cast<float>(point.x), static_cast<float>(point.y)};

    return pointf;
}

POINT ConvertXPOINTFToPOINT(_In_ const XPOINTF& pointf)
{
    POINT point = {static_cast<LONG>(pointf.x), static_cast<LONG>(pointf.y)};

    return point;
}

//Converts an XRECTF into an array of 4 points (in counter-clockwise order)
void RectToPoint(_In_ const XRECTF_RB& rect, _Inout_updates_(4) XPOINTF* points)
{
    points[0].x = rect.left;
    points[0].y = rect.top;
    points[1].x = rect.left;
    points[1].y = rect.bottom;
    points[2].x = rect.right;
    points[2].y = rect.bottom;
    points[3].x = rect.right;
    points[3].y = rect.top;

    EnsureCounterClockwiseWindingOrder<XPOINTF>(4, points);
}

XCORNERRADIUS CornerRadius(float uniform) { return { uniform, uniform, uniform, uniform }; }

XTHICKNESS Thickness(float uniform) { return {uniform, uniform, uniform, uniform}; }

XTHICKNESS SubtractThickness(_In_ const XTHICKNESS& first, _In_ const XTHICKNESS& second)
{
    return {first.left-second.left, first.top-second.top, first.right-second.right, first.bottom-second.bottom};
}

XRECTF ShrinkRectByThickness(_In_ const XRECTF& bounds, _In_ const XTHICKNESS& margin)
{
    return {
        bounds.X + margin.left,
        bounds.Y + margin.top,
        MAX(0.0f, bounds.Width - (margin.left + margin.right)),
        MAX(0.0f, bounds.Height - (margin.top + margin.bottom))};
}

XRECTF EnlargeRectByThickness(_In_ const XRECTF& bounds, _In_ const XTHICKNESS& margin)
{
    const XTHICKNESS inverseMargin = {-margin.left, -margin.top, -margin.right, -margin.bottom};
    return ShrinkRectByThickness(bounds, inverseMargin);
}

bool IsRectContainedBy(_In_ const XRECTF& r, _In_ const XRECTF& container)
{
    return r.X >= container.X
        && r.Y >= container.Y
        && r.X + r.Width <= container.X + container.Width
        && r.Y + r.Height <= container.Y + container.Height;
}

float ConvertRadianToDegrees(_In_ float radians)
{
    return radians * 180.0f / (float)M_PI;
}

float ConvertDegreesToRadian(_In_ float degrees)
{
    return degrees * (float)M_PI / 180.0f;
}

void ScaleRect(_Inout_ XRECTF_RB& rect, float factor)
{
    rect.left *= factor;
    rect.top *= factor;
    rect.right *= factor;
    rect.bottom *= factor;
}

