// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      (ctor) - Create a new point hit test helper.
//
//------------------------------------------------------------------------
PointHitTestHelper::PointHitTestHelper(
    const XPOINTF& hitTestPoint,
    XFLOAT tolerance,
    _In_opt_ const CMILMatrix* pTransform
    )
    : HitTestHelper(tolerance, pTransform)
    , m_hitTestPoint(hitTestPoint)
    , m_windingNumber(0)
    , m_encounteredEdgeNear(FALSE)
{
    //
    // Move the hit test point to the origin.
    //
    m_transform.AppendTranslation(-m_hitTestPoint.x, -m_hitTestPoint.y);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      (dtor) - Clean up the point hit test helper.
//
//------------------------------------------------------------------------
PointHitTestHelper::~PointHitTestHelper(
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle a new point.
//
//------------------------------------------------------------------------
void
PointHitTestHelper::AcceptPoint(
    _In_ const XPOINTF& endPoint
    )
{
    CheckIfSegmentNearTheOrigin(endPoint);
    CheckForNaN(endPoint);

    //
    // If this segment crosses the x axis we have to determine whether it
    // does it at the positive half.  The x of the intersection is a weighted
    // average of the x coordinates of the segment's endpoints. By triangle
    // similarity, the ratio of the distances between the crossing x and
    // the x's of the endpoints is equal to |ptEnd.y| / |m_ptCurrent.y|.
    //
    //
    //           * ptEnd
    //           |\
    //      *----*-\--*-------------
    //              \ |
    //               \|
    //                * m_ptCurrent
    //
    // This translates to x = s * m_ptCurrent.x + t * ptEnd.x, where
    // s = |ptEnd.y|/r, t = |m_ptCurrent.y|/r, r = |m_ptCurrent.y|+|ptEnd.y|.
    // Since we are only interested in the sign of x, we can multiply that by
    // r (which is known to be positive) and examine the sign of
    // |m_ptCurrent.y| * ptEnd.x + |ptEnd.y| * m_ptCurrent.x.
    //
    // Instead of taking abs of both y's we check their signs, which we need anyway,
    // and adjust them to be + when we test.
    //

    if (m_currentPoint.y > 0)
    {
        if (endPoint.y <= 0)
        {
            //
            // We have crossed the x axis going down
            //
            if (m_currentPoint.x * endPoint.y - endPoint.x * m_currentPoint.y >= 0)
            {
                //
                // The crossing was on the positive side
                //
                m_windingNumber--;
            }
        }
    }
    else
    {
        if (endPoint.y > 0)
        {
            //
            // We have crossed the x axis going up
            //
            if (endPoint.x * m_currentPoint.y - m_currentPoint.x * endPoint.y >= 0)
            {
                //
                // The crossing was on the positive side
                //
                m_windingNumber++;
            }
        }
    }

    //
    // Update next start point.
    //
    m_currentPoint = endPoint;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Check if a new segment is near to the origin (the hit test point).
//
//------------------------------------------------------------------------
void
PointHitTestHelper::CheckIfSegmentNearTheOrigin(
    _In_ const XPOINTF& endPoint
    )
{
    XDOUBLE squaredThreshold = m_tolerance * m_tolerance;

    // Check if the endpoint is near the origin.  No need to check the start
    // point, it was checked as the endpoint of the previous segment
    m_encounteredEdgeNear = m_encounteredEdgeNear || (endPoint * endPoint < squaredThreshold);

    if (!m_encounteredEdgeNear)
    {
        //
        // Now check if there is point in the segment that is close enough to the origin
        // Let vec = ptEnd - m_ptCurrent be the segment vector.  The segment is
        //
        //      P(t) = m_ptCurrent + t*vec.
        //
        // If P(t) is the point on the line nearest to the origin then P(t) is
        // perpendicular to segment, i.e. P(t) * vec = 0. The equation for t is then
        //
        //      (m_ptCurrent + t*vec) * vec = 0.
        //
        // The solution is
        //
        //      t = -(m_ptCurrent * vec) / (vec * vec)
        //
        // and it is inside the segment if 0 < t < 1.
        //
        // The point at t is
        //
        //      P = m_ptCurrent + ( (m_ptCurrent * vec) / (vec * vec) )*vec.
        //
        // and its squared distance from the origin is P * P.  If (0<t<=1) we
        // want to check if P * P < m_rSquaredThreshold.  But to avoid
        // divisions, we'll set r = vec * vec, and multiply 0 < t < = 1 by r
        // and P * P < m_rSquaredThreshold by r*r.
        //

        XPOINTF vec = endPoint - m_currentPoint;

        XFLOAT r = vec * vec;
        XFLOAT t = -(m_currentPoint * vec);

        if (0 <= t && t <= r)
        {
            //
            // The nearest point is inside the segment, examine its distance
            //
            XPOINTF Pr = m_currentPoint * r + vec * t;

            m_encounteredEdgeNear = Pr * Pr < squaredThreshold * r * r;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the current winding number.
//
//------------------------------------------------------------------------
HRESULT
PointHitTestHelper::GetWindingNumber(
    _Out_ XINT32* pWindingNumber
    )
{
    IFCEXPECT_RETURN(!m_encounteredNaN);

    *pWindingNumber = m_windingNumber;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Query if a near edge was found.
//
//------------------------------------------------------------------------
bool
PointHitTestHelper::EncounteredEdgeNear(
    )
{
    return m_encounteredEdgeNear;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the hit test result.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
PointHitTestHelper::GetResult(bool *pWasHit)
{
    IFCEXPECT_RETURN(!m_encounteredNaN);

    *pWasHit = (m_windingNumber != 0) || m_encounteredEdgeNear;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Reset state.
//
//------------------------------------------------------------------------
void
PointHitTestHelper::Reset()
{
    m_windingNumber = 0;
}
