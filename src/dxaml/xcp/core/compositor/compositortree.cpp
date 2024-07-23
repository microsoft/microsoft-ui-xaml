// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Describes the basic structures for the compositor rendering tree

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      ApplyTransformAndClip_DivideW helper method. If the point order
//      in the array was swapped after transforming, the AA masks need
//      to be swapped as well.
//
//------------------------------------------------------------------------
/* static */ void
PointOrderFlipped(
    XUINT32 pointCount,
    _Inout_updates_(pointCount) PointWithAAMasks *pPoints
    )
{
    //
    // The AA masks on the vertices reference the previous and next vertex in the list. If the
    // order of the list was reversed to get it back to counterclockwise winding, we have to
    // swap the AA masks of each vertex to keep the AA consistent.
    //
    XUINT32 temp;
    for (XUINT32 i = 0; i < pointCount; i++)
    {
        temp = pPoints[i].aaMaskInteriorToNextPoint;
        pPoints[i].aaMaskInteriorToNextPoint = pPoints[i].aaMaskInteriorToPreviousPoint;
        pPoints[i].aaMaskInteriorToPreviousPoint = temp;

        temp = pPoints[i].aaMaskExteriorToNextPoint;
        pPoints[i].aaMaskExteriorToNextPoint = pPoints[i].aaMaskExteriorToPreviousPoint;
        pPoints[i].aaMaskExteriorToPreviousPoint = temp;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ApplyTransformAndClip_DivideW helper method. The XPOINTF4 version
//      does nothing.
//
//------------------------------------------------------------------------
/* static */ void
PointOrderFlipped(
    XUINT32 pointCount,
    _Inout_updates_(pointCount) XPOINTF4 *pPoints
    )
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
HWClip::HWClip()
    : m_pPoints(NULL)
    , m_cPoints(0)
    , m_fIsEmpty(FALSE)
    , m_fIsInfinite(TRUE)
    , m_transform(TRUE)
    , m_fIsRectangular(TRUE)
{
    SetInfiniteClip(&m_clip);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
HWClip::HWClip(_In_ const HWClip &other)
    : m_pPoints(NULL)
    , m_cPoints(0)
    , m_fIsEmpty(other.m_fIsEmpty)
    , m_fIsInfinite(other.m_fIsInfinite)
    , m_transform(other.m_transform)
    , m_fIsRectangular(other.m_fIsRectangular)
    , m_clip(other.m_clip)
{
    if (other.m_pPoints != NULL)
    {
        XCP_FAULT_ON_FAILURE(FALSE);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
HWClip::~HWClip()
{
    SAFE_DELETE_ARRAY(m_pPoints);
 }

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to determine value equality.
//
//------------------------------------------------------------------------
bool
HWClip::Equals(_In_ const HWClip &other) const
{
    if (   m_fIsEmpty == other.m_fIsEmpty
        && m_fIsInfinite == other.m_fIsInfinite
        && m_clip.X == other.m_clip.X
        && m_clip.Y == other.m_clip.Y
        && m_clip.Width == other.m_clip.Width
        && m_clip.Height == other.m_clip.Height
        && m_cPoints == other.m_cPoints)
    {
        for (XUINT32 i = 0; i < m_cPoints; i++)
        {
            if (   m_pPoints[i].x != other.m_pPoints[i].x
                || m_pPoints[i].y != other.m_pPoints[i].y)
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Set this clip to a new clip rectangle
//
//------------------------------------------------------------------------
void
HWClip::Set(_In_ const XRECTF *pClip)
{
    Reset();

    if (pClip != NULL)
    {
        m_clip = *pClip;
        m_fIsEmpty = IsEmptyRectF(m_clip);
        m_fIsInfinite = FALSE;
    }

    ASSERT(m_cPoints != 0 || m_pPoints == NULL);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copy the clip
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::Set(_In_ const HWClip *pClip)
{
    HRESULT hr = S_OK;
    Reset();

    if (pClip->IsRectilinear())
    {
        m_clip = pClip->m_clip;
    }
    else
    {
        m_cPoints = pClip->m_cPoints;
        m_pPoints = new XPOINTF[m_cPoints];
        memcpy(m_pPoints, pClip->m_pPoints, m_cPoints * sizeof(XPOINTF));
    }

    m_fIsInfinite = pClip->m_fIsInfinite;
    m_fIsEmpty = pClip->m_fIsEmpty;
    m_fIsRectangular = pClip->m_fIsRectangular;
    m_transform = pClip->m_transform;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Infinite clip
//
//------------------------------------------------------------------------
void
HWClip::Reset()
{
    SAFE_DELETE_ARRAY(m_pPoints);
    SetInfiniteClip(&m_clip);
    m_cPoints = 0;
    m_fIsEmpty = FALSE;
    m_fIsInfinite = TRUE;
    m_fIsRectangular = TRUE;
    m_transform.SetToIdentity();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transform this clip
//      - If the transform is only a scale or translate, the clip is
//        preserved as a simple rectangle
//      - If a rotate is applied, the clip is transformed into a set of
//        points, CCW winding order. Once the clip has degenerated into
//        a set of points, it will never return to a simple rect.
//
//        FUTURE: An optimization could be applied to maintain the applied
//        transforms separately from the original clip rect, so that when
//        two clips were intersected that had the same rotation angle it
//        could be simply detected.
//
//      - If the clip is infinite or empty, the transform is ignored
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::Transform(_In_ const CMILMatrix *pTransform)
{
    HRESULT hr = S_OK;

    if (pTransform->IsIdentity() || m_fIsInfinite || m_fIsEmpty)
    {
        goto Cleanup;
    }

    if (IsRectilinear())
    {
        if (pTransform->IsScaleOrTranslationOnly())
        {
            // Keep clip as a rectangle!
            pTransform->TransformBounds(&m_clip, &m_clip);
            m_fIsEmpty = IsEmptyRectF(m_clip);
        }
        else
        {
            //
            // Generate polygon if we're rotated
            // We can optimize further here by storing all the transforms that
            // have been applied along with the original clip rect, and then
            // when we intersect another clip we could check if they have the
            // same rotation. Not doing this for now though.
            //
            ASSERT(m_pPoints == NULL);
            m_pPoints = new XPOINTF[4];
            m_cPoints = 4;
            FillPointsFromRectCCW(m_pPoints, m_clip);

            TransformPointsHelper(pTransform);
        }
    }
    else
    {
        TransformPointsHelper(pTransform);
    }

    m_transform.Prepend(*pTransform);

Cleanup:
    ASSERT(m_cPoints != 0 || m_pPoints == NULL);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method for HWClip::Transform
//      Transforms the points for a HWClip in point form. Also ensures
//      winding order and checks for emptiness.
//
//------------------------------------------------------------------------
void
HWClip::TransformPointsHelper(_In_ const CMILMatrix *pTransform)
{
    ASSERT(m_pPoints != NULL);

    pTransform->Transform(m_pPoints, m_pPoints, m_cPoints);
    EnsureCounterClockwiseWindingOrder(m_cPoints, m_pPoints);

    if (IsPolygonConcave(m_cPoints, m_pPoints))
    {
        // Due to floating point rounding error, we can sometimes get a resulting polygon
        // that is slightly concave. In those cases, we return the convex hull.
        ComputeConvexHull(m_cPoints, m_pPoints, &m_cPoints, m_pPoints);
    }

    XRECTF pointBounds = {};
    BoundPoints(m_pPoints, m_cPoints, &pointBounds);
    m_fIsEmpty = IsEmptyRectF(pointBounds);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The common transform and clip method used to get an optimized clip
//      and for geometry generation.
//
//      Takes a list of points (XPOINTF4 or PointWithAAMasks), an optional
//      3D projection, an optional 2D transform, and an optional 2D clip.
//      Transforms the points through the projection and the transform,
//      clips the resulting points against the 2D clip, then clips them
//      against the near plane. Drops z before returning the final points.
//
//      The input points are transformed in-place. The output points are
//      either NULL, the same as the input points, or a new buffer
//      allocated in this method.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT HWClip::ApplyTransformAndClip_DropZPreserveW(
    XUINT32 sourcePointCount,
    _Inout_updates_(sourcePointCount) XPOINTF4 *pSourcePoints,
    _In_opt_ const std::shared_ptr<const CMILMatrix4x4> p3DProjection,
    _In_ const CMILMatrix *p2DTransform,
    _In_ const HWClip *p2DClip,
    _Out_ XUINT32 *pDestPointCount,
    _Outptr_result_buffer_(*pDestPointCount) XPOINTF4 **ppDestPoints,
    _Out_opt_ bool *pWasClipped
    )
{
    HRESULT hr = S_OK;
    bool shouldClip = false;
    bool shouldNearPlaneClip = false;
    XPOINTF4 *pDestPoints = pSourcePoints;
    XUINT32 pointCount = sourcePointCount;
    XRECTF_RB pointBounds = {};

    if (pointCount < 3)
    {
        pDestPoints = NULL;
        pointCount = 0;
    }
    else
    {
        if (p3DProjection != NULL && !p3DProjection->IsIdentity())
        {
            const gsl::span<XPOINTF4> points(pDestPoints, pointCount);
            p3DProjection->Transform_PreserveW(points, points);
        }

        if (!p2DTransform->IsIdentity())
        {
            const gsl::span<XPOINTF4> points(pDestPoints, pointCount);
            p2DTransform->Transform3DPoints_PreserveW(points);
        }

        //
        // A projection through the near plane can change the points from being wound in a loop
        // to being wound in a figure 8 (the points cross over as they cross the near plane).
        // So near plane clipping must be done first to get the points back into a loop.
        //
        if (pointCount > 2 && p3DProjection != NULL && !p3DProjection->IsIdentity())
        {
            for (XUINT32 i = 0; i < pointCount; i++)
            {
                // Ignore w here. Near plane clipping happens inside the projection.
                if (pDestPoints[i].z < 0)
                {
                    shouldNearPlaneClip = TRUE;
                    break;
                }
            }

            if (shouldNearPlaneClip)
            {
                XPOINTF4 *pNearPlaneClippedPoints = NULL;
                pNearPlaneClippedPoints = new XPOINTF4[pointCount + 1];

                ProjectionClipAgainstNearPlane(
                    pDestPoints,
                    pointCount,
                    pNearPlaneClippedPoints,
                    &pointCount,
                    pointCount + 1,
                    &shouldNearPlaneClip
                    );

                if (pDestPoints != pSourcePoints)
                {
                    SAFE_DELETE_ARRAY(pDestPoints);
                }

                pDestPoints = pNearPlaneClippedPoints;
            }

            //
            // A PlaneProjection will change the winding to a figure 8 as points cross the near plane
            // because it derives the w value of the transformed points from the untransformed z values.
            // A Matrix3DProjection uses an arbitrary matrix to derive w, so it can also change winding
            // order arbitrarily. Clipping the points such that all have positive w values is difficult
            // to do during hit testing, so we just check the transformed points and skip both rendering
            // and hit testing if any of them have a nonpositive w value.
            //
            for (XUINT32 i = 0; i < pointCount; i++)
            {
                if (pDestPoints[i].w <= 0)
                {
                    TRACE(TraceAlways, L"A projected point has a nonpositive w value: { x: %f, y: %f, z: %f, w: %f }.", pDestPoints[i].x, pDestPoints[i].y, pDestPoints[i].z, pDestPoints[i].w);
                    pointCount = 0;
                    break;
                }
            }
        }

        bool orderWasFlipped = EnsureCounterClockwiseWindingOrder(pointCount, pDestPoints);
        if (orderWasFlipped)
        {
            PointOrderFlipped(pointCount, pDestPoints);
        }

        if (IsPolygonConcave(pointCount, pDestPoints))
        {
            // Due to floating point rounding error, we can sometimes get a resulting polygon
            // that is slightly concave. In those cases, we return the convex hull.
            XUINT32 convexPointCount;
            ComputeConvexHull(pointCount, pDestPoints, &convexPointCount, pDestPoints);

            // OACR detects a buffer overflow lower down because it doesn't realize that ComputeConvexHull
            // will never grow the size of the buffer. Assert to make that explicit.
            ASSERT(convexPointCount <= pointCount);
            pointCount = convexPointCount;
        }

        if (!p2DClip->IsInfinite() && pointCount > 0)
        {
            shouldClip = !p2DClip->DoesEntirelyContainPoints(pointCount, pDestPoints);

            if (shouldClip)
            {
                IFC(p2DClip->Clip(
                    pointCount,
                    pDestPoints,
                    &pointCount,
                    &pDestPoints
                    ));
            }
        }

        if (pointCount > 2)
        {
            const gsl::span<XPOINTF4> points(pDestPoints, pointCount);
            pointBounds = BoundPoints_RB(points);
        }

        if (!IsEmptyRectF(pointBounds))
        {
            for (XUINT32 i = 0; i < pointCount; i++)
            {
                pDestPoints[i].z = 0.0f;
            }
        }
        else
        {
            if (pDestPoints != pSourcePoints)
            {
                SAFE_DELETE_ARRAY(pDestPoints);
            }
            else
            {
                pDestPoints = NULL;
            }
            pointCount = 0;
        }
    }

    *pDestPointCount = pointCount;
    *ppDestPoints = pDestPoints;
    pDestPoints = NULL;

    if (pWasClipped != NULL)
    {
        *pWasClipped = shouldClip || shouldNearPlaneClip;
    }

Cleanup:
    SAFE_DELETE_ARRAY(pDestPoints);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies a projection, transform, and clip to this HWClip. Used to
//      transform the bounds of a primitive from local space up to world
//      space.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::ApplyTransformAndClip(
    _In_opt_ const std::shared_ptr<const CMILMatrix4x4> p3DProjection,
    _In_ const CMILMatrix *p2DTransform,
    _In_ const HWClip *p2DClip,
    _Out_opt_ bool *pWasClipped
    )
{
    HRESULT hr = S_OK;
    XPOINTF4 *pPoints = NULL;
    XPOINTF4 *pTransformedPoints = NULL;

    // This method is used to transform primitive bounds, and the bounds are never infinite.
    ASSERT(!IsInfinite());

    if (p3DProjection != NULL)
    {
        if (IsRectilinear())
        {
            ASSERT(m_pPoints == NULL);
            pPoints = new XPOINTF4[4];
            m_cPoints = 4;
            FillPointsFromRectCCW(pPoints, m_clip);
        }
        else
        {
            pPoints = new XPOINTF4[m_cPoints];
            for (XUINT32 i = 0; i < m_cPoints; i++)
            {
                pPoints[i].x = m_pPoints[i].x;
                pPoints[i].y = m_pPoints[i].y;
                pPoints[i].z = 0.0f;
                pPoints[i].w = 1.0f;
            }
        }

        // No need to update m_transform. This clip is no longer rectangular and will never be again.
        m_fIsRectangular = FALSE;

        IFC(HWClip::ApplyTransformAndClip_DropZPreserveW(
            m_cPoints /* sourcePointCount */,
            pPoints /* pSourcePoints */,
            p3DProjection,
            p2DTransform,
            p2DClip,
            &m_cPoints /* pDestPointCount */,
            &pTransformedPoints /* ppDestPoints */,
            pWasClipped
            ));

        if (pTransformedPoints != pPoints)
        {
            SAFE_DELETE_ARRAY(pPoints);
        }
        pPoints = pTransformedPoints;
        pTransformedPoints = NULL;

        SAFE_DELETE_ARRAY(m_pPoints);
        if (m_cPoints > 0)
        {
            m_pPoints = new XPOINTF[m_cPoints];
            for (XUINT32 i = 0; i < m_cPoints; i++)
            {
                m_pPoints[i].x = pPoints[i].x / pPoints[i].w;
                m_pPoints[i].y = pPoints[i].y / pPoints[i].w;
                ASSERT(pPoints[i].z == 0.0f);
            }

            if (IsPolygonConcave(m_cPoints, m_pPoints))
            {
                // Due to floating point rounding error, we can sometimes get a resulting polygon
                // that is slightly concave. In those cases, we return the convex hull.
                ComputeConvexHull(m_cPoints, m_pPoints, &m_cPoints, m_pPoints);
            }
        }
        else
        {
            EmptyRectF(&m_clip);
            m_fIsEmpty = TRUE;
            m_fIsInfinite = FALSE;
            m_fIsRectangular = TRUE;
        }
    }
    else
    {
        bool wasClipped = false;
        IFC(Transform(p2DTransform));

        if (!p2DClip->IsInfinite())
        {
            wasClipped = !p2DClip->DoesEntirelyContain(this);

            if (wasClipped)
            {
                IFC(Intersect(p2DClip));
            }
        }

        if (pWasClipped != NULL)
        {
            *pWasClipped = wasClipped;
        }
    }

Cleanup:
    SAFE_DELETE_ARRAY(pPoints);
    SAFE_DELETE_ARRAY(pTransformedPoints);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if the clip is rectangular and is axis aligned with the world transform
//
//------------------------------------------------------------------------
bool
HWClip::IsClipAxisAlignedWith(_In_ const CMILMatrix* pWorldTransform) const
{
    return m_fIsRectangular && AreMatricesAxisAligned(pWorldTransform, &m_transform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersect this clip
//      - Intersect with another HWClip
//      - This will maintain the simple rect clip if both are simple rect
//        clip
//      - If either clip is in point array mode, the intersection will be in
//        point array mode
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::Intersect(_In_opt_ const HWClip *pOther)
{
    HRESULT hr = S_OK;
    XPOINTF *pResultPoints = NULL;

    if (pOther == NULL || m_fIsEmpty || pOther->IsInfinite())
    {
        goto Cleanup;
    }

    if (m_fIsInfinite)
    {
        ASSERT((m_pPoints == NULL) && (m_cPoints == 0));

        IFC(Set(pOther));
    }
    else if (IsRectilinear() && pOther->IsRectilinear())
    {
        m_fIsEmpty = !IntersectRect(&m_clip, &pOther->m_clip);
    }
    else if (DoesEntirelyContain(pOther))
    {
        IFC(Set(pOther));
    }
    else if (!pOther->DoesEntirelyContain(this))
    {
        if (m_fIsRectangular)
        {
            // If the other clip is rectangular, and the matrices are axis aligned then
            // the intersection will maintain the rectangular nature of the clip, except
            // in cases of floating point error.
            m_fIsRectangular = pOther->IsClipAxisAlignedWith(&m_transform);
        }

        XUINT32 cOtherPoints = pOther->m_cPoints;
        XPOINTF *pOtherPoints = pOther->m_pPoints;

        // On the stack to avoid allocation.
        XPOINTF pRectPoints[4];

        if (m_pPoints == NULL)
        {
            m_cPoints = 4;
            m_pPoints = new XPOINTF[m_cPoints];
            FillPointsFromRectCCW(m_pPoints, m_clip);
        }

        if (pOtherPoints == NULL)
        {
            FillPointsFromRectCCW(pRectPoints, pOther->m_clip);

            cOtherPoints = 4;
            pOtherPoints = pRectPoints;
        }

        {
            // Allocate maximum number of points
            // ClipToPolygon iterates over all clip edges and all input edges. It intersects each clip edge against
            // the entire polygon, which should intersect with at most 2 edges of the convex polygon and introduce
            // at most 1 extra point. So the total number of points should be capped at (clip point count + polygon
            // point count). Floating point errors are causing the algorithm to find more than 2 intersections per clip
            // edge, producing more than 1 point per iteration, which overflows its buffer. Since the algorithm loops
            // over each and adds 2 points per iteration, the total becomes (clip point count * polygon point count * 2).
            XUINT32 cResultPoints = m_cPoints * cOtherPoints * 2;
            pResultPoints = new XPOINTF[cResultPoints];

            IFC(ClipToPolygon(
                m_cPoints,
                m_pPoints,
                cOtherPoints,
                pOtherPoints,
                &cResultPoints,
                pResultPoints
                ));

            SAFE_DELETE_ARRAY(m_pPoints);
            m_cPoints = cResultPoints;

            if (cResultPoints > 0)
            {
                // Floating point error can cause us to get back a clip rect with
                // more than 4 points when intersecting axis aligned rects. So account
                // for that here.
                m_fIsRectangular = m_fIsRectangular && cResultPoints == 4;

                m_pPoints = pResultPoints;
                pResultPoints = NULL;
            }
            else
            {
                m_pPoints = NULL;
                EmptyRectF(&m_clip);
                m_fIsEmpty = TRUE;
                m_fIsInfinite = FALSE;
                m_fIsRectangular = TRUE;
            }
        }
    }

Cleanup:
    ASSERT(m_cPoints != 0 || m_pPoints == NULL);

    SAFE_DELETE_ARRAY(pResultPoints);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get the bounds of this clip
//
//------------------------------------------------------------------------
void
HWClip::GetBounds(_Out_ XRECTF *pBounds) const
{
    if (IsRectilinear())
    {
        *pBounds = m_clip;
    }
    else
    {
        BoundPoints(m_pPoints, m_cPoints, pBounds);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersect this clip
//      - Intersect with an XRECTF clip
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::Intersect(_In_opt_ const XRECTF *pClip)
{
    XPOINTF *pResultPoints = NULL;

    HRESULT hr = S_OK;

    if (!m_fIsEmpty && pClip)
    {
        if (m_fIsInfinite)
        {
            ASSERT(!m_fIsEmpty);

            m_clip = *pClip;
            m_fIsInfinite = FALSE;

            if (IsEmptyRectF(m_clip))
            {
                m_fIsEmpty = TRUE;
            }
        }
        else if (IsRectilinear())
        {
            ASSERT(!m_fIsInfinite);
            ASSERT(!m_fIsEmpty);

            IntersectRect(&m_clip, pClip);

            if (IsEmptyRectF(m_clip))
            {
                m_fIsEmpty = TRUE;
            }
        }
        else
        {
            XPOINTF points[4];
            FillPointsFromRectCCW(points, *pClip);

            if (!IsEntirelyContained(m_cPoints, m_pPoints, 4, points))
            {
                if (m_fIsRectangular)
                {
                    // if the current transform is some 90 degree rotation of the identity matrix then the
                    // intersection will maintain the rectangularity of the clip, except in cases of
                    // floating point error.
                    CMILMatrix identity(true);
                    m_fIsRectangular = AreMatricesAxisAligned(&m_transform, &identity);
                }

                // Allocate maximum number of points
                // ClipToPolygon iterates over all clip edges and all input edges. It intersects each clip edge against
                // the entire polygon, which should intersect with at most 2 edges of the convex polygon and introduce
                // at most 1 extra point. So the total number of points should be capped at (clip point count + polygon
                // point count). Floating point errors are causing the algorithm to find more than 2 intersections per clip
                // edge, producing more than 1 point per iteration, which overflows its buffer. Since the algorithm loops
                // over each and adds 2 points per iteration, the total becomes (clip point count * polygon point count * 2).
                XUINT32 cResultPoints = m_cPoints * 4 * 2;
                pResultPoints = new XPOINTF[cResultPoints];

                IFC(ClipToPolygon(
                    m_cPoints,
                    m_pPoints,
                    4,
                    points,
                    &cResultPoints,
                    pResultPoints
                    ));

                SAFE_DELETE_ARRAY(m_pPoints);

                if (cResultPoints > 0)
                {
                    // Floating point error can cause us to get back a clip rect with
                    // more than 4 points when intersecting axis aligned rects. So account
                    // for that here.
                    m_fIsRectangular = m_fIsRectangular && cResultPoints == 4;

                    m_pPoints = pResultPoints;
                    pResultPoints = NULL;
                    m_cPoints = cResultPoints;
                }
                else
                {
                    m_cPoints = 0;
                    EmptyRectF(&m_clip);
                    m_fIsEmpty = TRUE;
                    m_fIsInfinite = FALSE;
                    m_fIsRectangular = TRUE;
                }
            }
        }
    }

Cleanup:
    ASSERT(m_cPoints != 0 || m_pPoints == NULL);

    delete[] pResultPoints;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns TRUE if the XRECTF clip intersects with this clip
//
//------------------------------------------------------------------------
bool
HWClip::DoesIntersect(_In_ const XRECTF& clip) const
{
    if (m_fIsEmpty || IsEmptyRectF(clip))
    {
        return false;
    }
    else
    {
        if (m_fIsInfinite)
        {
            ASSERT(!m_fIsEmpty);

            return true;
        }
        else if (IsRectilinear())
        {
            ASSERT(!m_fIsInfinite);
            ASSERT(!m_fIsEmpty);

            return DoRectsIntersect(m_clip, clip);
        }
        else
        {
            XPOINTF points[4];
            FillPointsFromRectCCW(points, clip);

            return DoPolygonsIntersect(m_cPoints, m_pPoints, 4, points);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a set of points against this HWClip. Used for projections.
//      The z values of the incoming points do matter, and the w values
//      must be preserved for correct perspective texturing.
//
//      The output points are either NULL, the same as the input points,
//      or a new buffer allocated by this method.
//
//------------------------------------------------------------------------
template <typename PointType>
_Check_return_ HRESULT
HWClip::Clip(
    XUINT32 cUnclippedPoints,
    _In_reads_(cUnclippedPoints) PointType *pUnclippedPoints,
    _Out_ XUINT32 *pClippedPointsCount,
    _Outptr_result_buffer_(*pClippedPointsCount) PointType **ppClippedPoints
    ) const
{
    HRESULT hr = S_OK;

    XUINT32 clippedPointsCount = 0;
    PointType *pClippedPoints = NULL;

    if (m_fIsEmpty)
    {
        // All points are clipped out.
        clippedPointsCount = 0;
        pClippedPoints = NULL;
    }
    else if (m_fIsInfinite)
    {
        // No points are clipped out.
        clippedPointsCount = cUnclippedPoints;
        pClippedPoints = pUnclippedPoints;
    }
    else
    {
        XUINT32 cClip;
        XPOINTF *pClip;
        XPOINTF rectPoints[4];

        if (IsRectilinear())
        {
            FillPointsFromRectCCW(rectPoints, m_clip);

            cClip = 4;
            pClip = rectPoints;
        }
        else
        {
            cClip = m_cPoints;
            pClip = m_pPoints;
        }

        if (IsEntirelyContained(cUnclippedPoints, pUnclippedPoints, cClip, pClip))
        {
            // No points are clipped out.
            clippedPointsCount = cUnclippedPoints;
            pClippedPoints = pUnclippedPoints;
        }
        else
        {
            // Points are clipped.

            // Allocate maximum number of points
            // ClipToPolygon iterates over all clip edges and all input edges. It intersects each clip edge against
            // the entire polygon, which should intersect with at most 2 edges of the convex polygon and introduce
            // at most 1 extra point. So the total number of points should be capped at (clip point count + polygon
            // point count). Floating point errors are causing the algorithm to find more than 2 intersections per clip
            // edge, producing more than 1 point per iteration, which overflows its buffer. Since the algorithm loops
            // over each and adds 2 points per iteration, the total becomes (clip point count * polygon point count * 2).
            clippedPointsCount = cClip * cUnclippedPoints * 2;
            pClippedPoints = new PointType[clippedPointsCount];

            IFC(ClipToPolygon(
                cUnclippedPoints,
                pUnclippedPoints,
                cClip,
                pClip,
                &clippedPointsCount,
                pClippedPoints
                ));
        }
    }

    *pClippedPointsCount = clippedPointsCount;
    *ppClippedPoints = pClippedPoints;
    pClippedPoints = NULL;

Cleanup:
    SAFE_DELETE_ARRAY(pClippedPoints);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      - Check if this clip entirely contains a point.
//        Used for hit testing against the brush clip in PC rendering.
//
//------------------------------------------------------------------------
bool
HWClip::Contains(
    _In_ const XPOINTF *pPoint
    ) const
{
    if (IsRectilinear())
    {
        return (pPoint->x >= m_clip.X
            && pPoint->y >= m_clip.Y
            && pPoint->x < m_clip.X + m_clip.Width
            && pPoint->y < m_clip.Y + m_clip.Height
            );
    }
    else
    {
        return IsEntirelyContained(
            1,
            pPoint,
            m_cPoints,
            m_pPoints
            );
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      - Check if this clip entirely contains pOther.
//        Intended to be used if this clip is a higher clip in the stack,
//        to determine if pOther and this need to be intersected when
//        pOther is pushed onto the clip stack.
//
//------------------------------------------------------------------------
bool
HWClip::DoesEntirelyContain(_In_ const HWClip *pOther) const
{
    if (pOther->IsRectilinear())
    {
        return DoesEntirelyContain(&pOther->m_clip);
    }
    else if (IsRectilinear())
    {
        XPOINTF points[4];
        FillPointsFromRectCCW(points, m_clip);

        return IsEntirelyContained(pOther->m_cPoints,
                                   pOther->m_pPoints,
                                   4,
                                   points
                                   );
    }
    else
    {
        return IsEntirelyContained(pOther->m_cPoints,
                                   pOther->m_pPoints,
                                   m_cPoints,
                                   m_pPoints
                                   );
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      - Check if this clip entirely contains pBounds.
//        Intended to be used if this clip is a higher clip in the stack,
//        to determine if pBounds and this need to be intersected when
//        pBounds is pushed onto the clip stack.
//
//------------------------------------------------------------------------
bool
HWClip::DoesEntirelyContain(_In_ const XRECTF *pBounds) const
{
    if (IsRectilinear())
    {
        XRECTF clipCopy = m_clip;
        IntersectRect(&clipCopy, pBounds);
        return    (clipCopy.X == pBounds->X)
               && (clipCopy.Y == pBounds->Y)
               && (clipCopy.Width == pBounds->Width)
               && (clipCopy.Height == pBounds->Height);
    }
    else
    {
        XPOINTF bounds[4];
        FillPointsFromRectCCW(bounds, *pBounds);

        return IsEntirelyContained(4,
                                   bounds,
                                   m_cPoints,
                                   m_pPoints
                                   );
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      - Check if this clip entirely contains a set of points.
//        Intended to be used to check whether a polygon would be clipped
//        by this HWClip.
//
//------------------------------------------------------------------------
template <typename PointType>
bool
HWClip::DoesEntirelyContainPoints(
    XUINT32 pointCount,
    _In_reads_(pointCount) const PointType *pPoints
    ) const
{
    if (IsRectilinear())
    {
        XPOINTF points[4];
        FillPointsFromRectCCW(points, m_clip);

        return IsEntirelyContained(
            pointCount,
            pPoints,
            4,
            points
            );
    }
    else
    {
        return IsEntirelyContained(
            pointCount,
            pPoints,
            m_cPoints,
            m_pPoints
            );
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is this clip empty?
//
//------------------------------------------------------------------------
bool
HWClip::IsEmpty() const
{
    return m_fIsEmpty;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Is this infinite? Not a clip at all, really.
//
//------------------------------------------------------------------------
bool
HWClip::IsInfinite() const
{
    return m_fIsInfinite;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the simple clip rectangle back if it's available.
//
//------------------------------------------------------------------------
void
HWClip::GetRectangularClip(_Out_ XRECTF *pRect) const
{
    XCP_FAULT_ON_FAILURE(m_fIsRectangular);

    *pRect = m_clip;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the rectilinear clip.
//      - IsClipAxisAlignedWith pTransform must be true
//      - The inverse for the transform will be used to find the rectilinear clip
//     This is used to find the local clip paired with the transform for fast path drawing
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
HWClip::GetRectilinearClip(_Out_ XRECTF *pRect, _In_ const CMILMatrix* pTransform) const
{
    XCP_FAULT_ON_FAILURE(m_fIsRectangular);

    CMILMatrix inverse = *pTransform;

    if (inverse.Invert() == 0)
    {
        return E_FAIL;
    }

    if (m_cPoints == 0)
    {
        XCP_FAULT_ON_FAILURE(pTransform->IsAxisAlignedScaleOrTranslationReal());

        *pRect = m_clip;

        inverse.TransformBounds(pRect, pRect);
    }
    else
    {
        XCP_FAULT_ON_FAILURE(AreMatricesAxisAligned(pTransform, &m_transform));

        XPOINTF coords[4];

        inverse.Transform(m_pPoints, coords, 4);

        XRECTF clip;

        clip.X = MIN(MIN(coords[0].x, coords[1].x), coords[2].x);
        clip.Y = MIN(MIN(coords[0].y, coords[1].y), coords[2].y);

        clip.Width = MAX(MAX(coords[0].x, coords[1].x), coords[2].x) - clip.X;
        clip.Height = MAX(MAX(coords[0].y, coords[1].y), coords[2].y) - clip.Y;

        *pRect = clip;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the polygon clip back if it's available. Memory allocation is
//      owned by this class.
//
//------------------------------------------------------------------------
void
HWClip::GetPolygonClip(_Outptr_ XPOINTF **ppPoints, _Out_ XUINT32 *pPointsCount) const
{
    ASSERT(m_cPoints != 0 || m_pPoints == NULL);

    XCP_FAULT_ON_FAILURE(!IsRectilinear() && !IsEmpty());

    *ppPoints = m_pPoints;
    *pPointsCount = m_cPoints;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
TransformAndClipFrame::TransformAndClipFrame()
    : m_2DTransform(CMILMatrix(TRUE))
    , m_p3DTransform()
    , m_clip()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
TransformAndClipFrame::~TransformAndClipFrame()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copies another transform and clip frame.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::Reset()
{
    m_clip.Reset();
    m_2DTransform.SetToIdentity();
    m_p3DTransform.reset();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copies another transform and clip frame.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipFrame::Set(_In_ const TransformAndClipFrame *pOther)
{
    ASSERT(m_2DTransform.IsIdentity() && m_p3DTransform == NULL && m_clip.IsInfinite());

    SetTransformAndProjection(pOther);
    RRETURN(m_clip.Set(&pOther->m_clip));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends a transform.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::PrependTransform(_In_ const CMILMatrix& transform)
{
    ASSERT(m_p3DTransform == NULL);
    m_2DTransform.Prepend(transform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects a clip. The clip is in the same 2D plane as this frame.
//      The clip is also in its local space and needs to be transformed
//      into the space of this frame.
//
//      Modifies the clip passed in.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipFrame::IntersectLocalSpaceClip(_Inout_ HWClip *pClip)
{
    ASSERT(m_p3DTransform == NULL);

    IFC_RETURN(pClip->Transform(&m_2DTransform));
    IFC_RETURN(m_clip.Intersect(pClip));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the projection transform.
//
//------------------------------------------------------------------------
void TransformAndClipFrame::SetProjection(const CMILMatrix4x4* const pProjection)
{
    ASSERT(m_p3DTransform == NULL);
    m_p3DTransform.reset(pProjection);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copies another transform and clip frame.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::SetTransformAndProjection(_In_ const TransformAndClipFrame *pOther)
{
    ASSERT(m_p3DTransform == NULL);

    m_2DTransform = pOther->m_2DTransform;
    m_p3DTransform = pOther->m_p3DTransform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Combines this transform and clip frame with another one. The other
//      frame is farther from the root, so its transforms are prepended.
//      The clips are intersected together.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipFrame::Prepend(_In_ const TransformAndClipFrame *pOther)
{
    ASSERT(m_p3DTransform == NULL);

    //
    // The clips are stored in the coordinate space of the rootmost element in the frame. The
    // other frame is farther from the root than this frame, so the rootmost element of the
    // combined frame will be the rootmost element of this frame. The clip of the other frame
    // must be transformed through the transform in this frame to bring it up to the root.
    //
    HWClip otherClip;
    IFC_RETURN(otherClip.Set(&pOther->m_clip));
    IFC_RETURN(IntersectLocalSpaceClip(&otherClip));

    PrependTransform(pOther->m_2DTransform);

    m_p3DTransform = pOther->m_p3DTransform;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to determine value equality.
//
//------------------------------------------------------------------------
bool
TransformAndClipFrame::Equals(_In_ const TransformAndClipFrame &other) const
{
    return m_2DTransform == other.m_2DTransform
        && (m_p3DTransform == NULL && other.m_p3DTransform == NULL
            || m_p3DTransform != NULL && other.m_p3DTransform != NULL && *m_p3DTransform == *other.m_p3DTransform)
        && m_clip.Equals(other.m_clip);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips and transforms the world space HWClip to move it to local space.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipFrame::TransformToLocalSpace(
    _Inout_ HWClip *pWorldClip
    ) const
{
    CMILMatrix inverse2D = m_2DTransform;

    IFC_RETURN(pWorldClip->Intersect(&m_clip));

    if (inverse2D.Invert())
    {
        IFC_RETURN(pWorldClip->Transform(&inverse2D));
    }
    else
    {
        pWorldClip->Reset();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends the projection transform with the z values dropped.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::GetProjectionWithoutZ(_Out_ CMILMatrix4x4 *pProjectionTransform) const
{
    if (m_p3DTransform != NULL)
    {
        *pProjectionTransform = *m_p3DTransform;
    }
    else
    {
        pProjectionTransform->SetToIdentity();
    }

    // Drop z component by setting the z column to 0.
    pProjectionTransform->SetM13(0.0f);
    pProjectionTransform->SetM23(0.0f);
    pProjectionTransform->SetM33(0.0f);
    pProjectionTransform->SetM43(0.0f);

    // Also set the z row to 0. Input points are all 2D and won't have a z value, so this won't matter.
    // TODO: This will change if projections don't have to flatten.
    pProjectionTransform->SetM31(0.0f);
    pProjectionTransform->SetM32(0.0f);
    pProjectionTransform->SetM34(0.0f);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends the net transform in this frame to a 4x4 matrix. The net
//      transform is the 3D transform prepended to the 2D transform, with
//      the Z values dropped.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::PrependNetTransformTo(_Inout_ CMILMatrix4x4 *pCombinedTransform) const
{
    CMILMatrix4x4 netTransform;
    GetProjectionWithoutZ(&netTransform);

    // 3D prepended to 2D is the same as 2D appended to 3D
    netTransform.Append(m_2DTransform);

    pCombinedTransform->Prepend(netTransform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the 2D transform in this frame. Used by text for pixel
//      snapping.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::Get2DTransform(
    _Outptr_ CMILMatrix *p2DTransform
    ) const
{
    *p2DTransform = m_2DTransform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the 2D transform in this frame. Used by text for pixel
//      snapping.
//
//------------------------------------------------------------------------
void
TransformAndClipFrame::Set2DTransform(_In_ const CMILMatrix& transform)
{
    m_2DTransform = transform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the clip is infinite.
//
//------------------------------------------------------------------------
bool
TransformAndClipFrame::HasInfiniteClip() const
{
    return m_clip.IsInfinite();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the clip is zero-sized.
//
//------------------------------------------------------------------------
bool
TransformAndClipFrame::HasZeroSizedClip() const
{
    return m_clip.IsEmpty();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether there's a projection.
//
//------------------------------------------------------------------------
bool
TransformAndClipFrame::HasProjection() const
{
    return m_p3DTransform != NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
TransformAndClipStack::TransformAndClipStack()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
TransformAndClipStack::~TransformAndClipStack()
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      assignment operator
//
//-------------------------------------------------------------------------
TransformAndClipStack&
TransformAndClipStack::operator=(_In_ const TransformAndClipStack &other)
{
    // Use Reset() and PushTransformsAndClips() instead
    XCP_FAULT_ON_FAILURE(FALSE);
    return *this;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a copy of an existing transform and clip stack.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
TransformAndClipStack::Create(
    _In_opt_ const TransformAndClipStack *pOriginalTransformAndClipStack,
    _Outptr_ TransformAndClipStack **ppTransformAndClipStackCopy
    )
{
    HRESULT hr = S_OK;
    TransformAndClipStack *pTransformAndClipStack = NULL;

    pTransformAndClipStack = new TransformAndClipStack();

    // This causes the top-level-leak finder to de-emphasize what appear to be top-level leaks of these, as the marking of m_dependentInstances isn't
    // quite perfect.  The m_dependentInstances is marked as containing weak pointers only, but also contains pointers to TransformAndClipStack, and those
    // pointers are actually strong, but marked incorrectly.  Rather than make xvector too smart and complicated about this top-level-leak finding stuff,
    // tell the top-level-leak finder to basically just de-emphasize these until there's nothing better to report (in which case these may actually be
    // directly relevant to fixing a leak, but probably only a leak of these, since they don't appear to AddRef() other stuff).
    XcpMarkHardToTrack(pTransformAndClipStack);

    if (pOriginalTransformAndClipStack != NULL)
    {
        IFC(pTransformAndClipStack->Set(pOriginalTransformAndClipStack));
    }

    *ppTransformAndClipStackCopy = pTransformAndClipStack;
    pTransformAndClipStack = NULL;

Cleanup:
    SAFE_DELETE(pTransformAndClipStack);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the specified frame. Frame 0 is the bottom of the stack
//      and is inlined into the stack. Frames 1 to n are in an xvector.
//
//------------------------------------------------------------------------
TransformAndClipFrame*
TransformAndClipStack::GetFrame(XUINT32 number)
{
    if (number == 0)
    {
        return &m_bottomFrame;
    }
    else
    {
        ASSERT(m_additionalFrames.size() > number - 1);
        return &m_additionalFrames[number - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the top frame.
//
//------------------------------------------------------------------------
TransformAndClipFrame*
TransformAndClipStack::GetTopFrame()
{
    if (m_additionalFrames.size() == 0)
    {
        return &m_bottomFrame;
    }
    else
    {
        return &m_additionalFrames[m_additionalFrames.size() - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the top frame.
//
//------------------------------------------------------------------------
const TransformAndClipFrame*
TransformAndClipStack::GetTopFrameConst() const
{
    if (m_additionalFrames.size() == 0)
    {
        return &m_bottomFrame;
    }
    else
    {
        return &m_additionalFrames[m_additionalFrames.size() - 1];
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes a new frame on the stack and returns it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipStack::PushFrame(_Outptr_result_maybenull_ TransformAndClipFrame **ppNewFrame)
{
    TransformAndClipFrame newFrame;
    IFC_RETURN(m_additionalFrames.push_back(newFrame));
    if (ppNewFrame != NULL)
    {
        *ppNewFrame = &m_additionalFrames[m_additionalFrames.size() - 1];
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the transform and clip stack.
//
//------------------------------------------------------------------------
void
TransformAndClipStack::Reset()
{
    m_bottomFrame.Reset();
    m_additionalFrames.clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Copies another transform and clip stack.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipStack::Set(_In_opt_ const TransformAndClipStack *pOther)
{
    Reset();

    if (pOther != NULL)
    {
        IFC_RETURN(m_bottomFrame.Set(&pOther->m_bottomFrame));

        if (!pOther->m_additionalFrames.empty())
        {
            for (XUINT32 otherFrameNumber = 0; otherFrameNumber < pOther->m_additionalFrames.size(); otherFrameNumber++)
            {
                TransformAndClipFrame *pNewFrame = NULL;
                IFC_RETURN(PushFrame(&pNewFrame));
                IFC_RETURN(pNewFrame->Set(&pOther->m_additionalFrames[otherFrameNumber]));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends a transform to the current frame in the stack.
//
//------------------------------------------------------------------------
void
TransformAndClipStack::PrependTransform(_In_ const CMILMatrix& transform)
{
    GetTopFrame()->PrependTransform(transform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends a projection to the current frame in the stack. Pushes
//      a new frame in the stack.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT TransformAndClipStack::PrependProjection(const CMILMatrix4x4* pProjection)
{
    GetTopFrame()->SetProjection(pProjection);
    IFC_RETURN(PushFrame(NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects a clip into the current frame in the stack. The clip
//      is in its local space and needs to be transformed into the space
//      of the frame.
//
//      Modifies the clip passed in.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipStack::IntersectLocalSpaceClip(_Inout_ HWClip *pClip)
{
    IFC_RETURN(GetTopFrame()->IntersectLocalSpaceClip(pClip));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes another transform and clip stack on top of this one. The
//      other stack contains transforms and clips farther from the root.
//      The bottom frame of the other stack is merged with the top frame
//      of this stack.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipStack::PushTransformsAndClips(_In_opt_ const TransformAndClipStack *pOther)
{
    if (pOther != NULL)
    {
        IFC_RETURN(GetTopFrame()->Prepend(&pOther->m_bottomFrame));

        for (XUINT32 otherFrameNumber = 0; otherFrameNumber < pOther->m_additionalFrames.size(); otherFrameNumber++)
        {
            TransformAndClipFrame *pNewFrame = NULL;
            IFC_RETURN(PushFrame(&pNewFrame));
            IFC_RETURN(pNewFrame->Set(&pOther->m_additionalFrames[otherFrameNumber]));
        }

        ASSERT(!GetTopFrame()->HasProjection());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper to determine value equality.
//
//------------------------------------------------------------------------
bool
TransformAndClipStack::Equals(_In_opt_ const TransformAndClipStack &other) const
{
    bool areEqual = m_additionalFrames.size() == other.m_additionalFrames.size();

    if (areEqual)
    {
        areEqual = m_bottomFrame.Equals(other.m_bottomFrame);
    }

    for (XUINT32 frameNumber = 0; areEqual && frameNumber < m_additionalFrames.size(); frameNumber++)
    {
        areEqual &= m_additionalFrames[frameNumber].Equals(other.m_additionalFrames[frameNumber]);
    }

    return areEqual;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the combined transform in this stack as a 4x4 matrix by
//      concatenating the z-dropping transform in each frame.
//
//------------------------------------------------------------------------
void
TransformAndClipStack::GetCombinedTransform(_Out_ CMILMatrix4x4 *pCombinedTransform) const
{
    CMILMatrix4x4 combinedTransform(TRUE);

    m_bottomFrame.PrependNetTransformTo(&combinedTransform);
    if (!m_additionalFrames.empty())
    {
        for (xvector<TransformAndClipFrame>::const_iterator currentFrame = m_additionalFrames.begin(); currentFrame != m_additionalFrames.end(); currentFrame++)
        {
            currentFrame->PrependNetTransformTo(&combinedTransform);
        }
    }

    *pCombinedTransform = combinedTransform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the 2D transform in the leafmost projection in this stack.
//      Used by text for pixel snapping.
//
//------------------------------------------------------------------------
void
TransformAndClipStack::Get2DTransformInLeafmostProjection(
    _Outptr_ CMILMatrix *p2DTransform
    ) const
{
    GetTopFrameConst()->Get2DTransform(p2DTransform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the 2D transform in the leafmost projection in this stack.
//      Used by text for pixel snapping.
//
//------------------------------------------------------------------------
void
TransformAndClipStack::Set2DTransformInLeafmostProjection(_In_ const CMILMatrix& transform)
{
    GetTopFrame()->Set2DTransform(transform);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the clip is zero-sized.
//
//------------------------------------------------------------------------
bool
TransformAndClipStack::HasZeroSizedClip() const
{
    bool hasZeroSizedClip = m_bottomFrame.HasZeroSizedClip();

    if (!m_additionalFrames.empty())
    {
        for (xvector<TransformAndClipFrame>::const_reverse_iterator currentFrame = m_additionalFrames.rbegin(); !hasZeroSizedClip && currentFrame != m_additionalFrames.rend(); currentFrame++)
        {
            hasZeroSizedClip |= currentFrame->HasZeroSizedClip();
        }
    }

    return hasZeroSizedClip;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether there is a projection in the stack.
//
//------------------------------------------------------------------------
bool
TransformAndClipStack::HasProjection() const
{
    return m_additionalFrames.size() > 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Transforms and intersects a world clip through this TransformAndClipStack into
//      local space.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
TransformAndClipStack::TransformToLocalSpace(
    _Inout_ HWClip *pWorldClip
    ) const
{
    // TODO: HWPC: This is purely used for optimizations right now - we don't bother attempting to clip through projections.
    if (!m_additionalFrames.empty())
    {
        pWorldClip->Reset();
    }
    else
    {
        IFC_RETURN(m_bottomFrame.TransformToLocalSpace(pWorldClip));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//-------------------------------------------------------------------------
CTransformToRoot::CTransformToRoot()
    // transformToRoot3D will be initialized when it's needed
    : m_transformToRoot2D(TRUE)
    , m_is3DMode(FALSE)
    , m_transformToRoot2DWithRasterizationScale(TRUE)
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//-------------------------------------------------------------------------
CTransformToRoot::~CTransformToRoot()
{
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends a 2D transform.
//
//-------------------------------------------------------------------------
void
CTransformToRoot::Append(_In_ const CMILMatrix &transform2D)
{
    if (m_is3DMode)
    {
        m_transformToRoot3D.Append(transform2D);
    }

    m_transformToRoot2D.Append(transform2D);
    m_transformToRoot2DWithRasterizationScale.Append(transform2D);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Appends a 2D transform.
//
//-------------------------------------------------------------------------
void
CTransformToRoot::Prepend(_In_ const CMILMatrix &transform2D)
{
    if (m_is3DMode)
    {
        m_transformToRoot3D.Prepend_DropZ(transform2D);
    }

    m_transformToRoot2D.Prepend(transform2D);
    m_transformToRoot2DWithRasterizationScale.Prepend(transform2D);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Sets to a uniform 2D scale.
//
//-------------------------------------------------------------------------
void
CTransformToRoot::SetTo2DScale(XFLOAT scaleXY)
{
    m_transformToRoot2D.SetToIdentity();
    m_transformToRoot2D.Scale(scaleXY, scaleXY);
    m_transformToRoot2DWithRasterizationScale.SetToIdentity();
    m_transformToRoot2DWithRasterizationScale.Scale(scaleXY, scaleXY);
    m_transformToRoot2DWithRasterizationScale.Scale(m_additionalRasterizationScale, m_additionalRasterizationScale);
    m_is3DMode = FALSE;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Prepends a 3D transform.
//
//-------------------------------------------------------------------------
void
CTransformToRoot::Prepend(_In_ const CMILMatrix4x4 &transform3D)
{
    if (!m_is3DMode)
    {
        m_transformToRoot3D.SetTo2DTransform(&m_transformToRoot2D);
        m_is3DMode = TRUE;
    }

    m_transformToRoot3D.Prepend(transform3D);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Appends a 3D transform.
//
//-------------------------------------------------------------------------
void
CTransformToRoot::Append(_In_ const CMILMatrix4x4 &transform3D)
{
    if (!m_is3DMode)
    {
        m_transformToRoot3D.SetTo2DTransform(&m_transformToRoot2D);
        m_is3DMode = TRUE;
    }

    m_transformToRoot3D.Append(transform3D);
}

void CTransformToRoot::MultiplyRasterizationScale(double scale)
{
    ASSERT(scale > 0);
    m_transformToRoot2DWithRasterizationScale.Scale(static_cast<XFLOAT>(scale), static_cast<XFLOAT>(scale));
    m_additionalRasterizationScale *= static_cast<float>(scale);
}

// Returns the scale dimensions that will be applied to an element. Takes RealizationScale into account.
_Check_return_ HRESULT CTransformToRoot::GetScaleDimensions(
    _In_ CUIElement *pElement,
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY) const
{
    if (!m_is3DMode)
    {
        m_transformToRoot2DWithRasterizationScale.GetScaleDimensions(pScaleX, pScaleY);
    }
    else
    {
        XSIZEF elementSize;
        IFC_RETURN(pElement->GetElementSizeForProjection(&elementSize));

        GetScaleDimensions(
            &elementSize,
            pScaleX,
            pScaleY
            );
        // m_additionalRasterizationScale is already multiplied in
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the scale dimensions that will be applied to an element,
//      given its size.
//
//-------------------------------------------------------------------------
void CTransformToRoot::GetScaleDimensions(
    _In_ XSIZEF *pElementSize,
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY) const
{
    if (!m_is3DMode)
    {
        m_transformToRoot2DWithRasterizationScale.GetScaleDimensions(pScaleX, pScaleY);
    }
    else
    {
        XFLOAT minScaleX, minScaleY;
        GetMinimumScaleDimensions(&minScaleX, &minScaleY);

        m_transformToRoot3D.Get2DScaleDimensions(*pElementSize, minScaleX, minScaleY, pScaleX, pScaleY);

        *pScaleX *= m_additionalRasterizationScale;
        *pScaleY *= m_additionalRasterizationScale;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the minimum scale dimensions that will be applied to an
//      element. This ignores any scales applied by a projection.
//
//-------------------------------------------------------------------------
void CTransformToRoot::GetMinimumScaleDimensions(
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY) const
{
    m_transformToRoot2DWithRasterizationScale.GetScaleDimensions(pScaleX, pScaleY);
}

// Includes scales and subpixel offsets
CMILMatrix CTransformToRoot::GetRasterizationMatrix(_In_ CUIElement* element) const
{
    XFLOAT realizationScaleX, realizationScaleY;
    IFCFAILFAST(GetScaleDimensions(element, &realizationScaleX, &realizationScaleY));
    // Note: GetScaleDimensions already accounts for m_additionalRasterizationScale

    CMILMatrix realizationMatrix(true);
    realizationMatrix.Scale(realizationScaleX, realizationScaleY);
    if (!m_is3DMode)
    {
        // The subpixel offsets must take m_additionalRasterizationScale into account as well, because it affects
        // where content will land on the screen.
        realizationMatrix.SetDx(FractionReal(m_transformToRoot2DWithRasterizationScale.GetDx()));
        realizationMatrix.SetDy(FractionReal(m_transformToRoot2DWithRasterizationScale.GetDy()));
    }
    else
    {
        // If there's 3D involved, ignore the subpixel offsets and just return the effective scaling.
    }
    return realizationMatrix;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a 2D transform to the root.
//
//-------------------------------------------------------------------------
CMILMatrix CTransformToRoot::Get2DTransformToRoot(_In_ CUIElement *pElement) const
{
    if (!m_is3DMode)
    {
        return m_transformToRoot2D;
    }
    else
    {
        CMILMatrix matrix(true);

        // There's a projection involved. Ignore the subpixel offsets and
        // just return the effective scaling.

        XFLOAT scaleX, scaleY;
        IFCFAILFAST(GetScaleDimensions(pElement, &scaleX, &scaleY));
        // We want the transform that Xaml applies, so divide out m_additionalRasterizationScale which is included
        // in GetScaleDimensions.
        scaleX /= m_additionalRasterizationScale;
        scaleY /= m_additionalRasterizationScale;
        matrix.Scale(scaleX, scaleY);

        return matrix;
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether this transform is the same as another one.
//
//-------------------------------------------------------------------------
bool
CTransformToRoot::IsSameAs(
    _In_ const CTransformToRoot *pOther
    ) const
{
    if (m_is3DMode)
    {
        return pOther->m_is3DMode
            && m_transformToRoot3D == pOther->m_transformToRoot3D
            && m_additionalRasterizationScale == pOther->m_additionalRasterizationScale;
    }
    else
    {
        return !pOther->m_is3DMode
            && m_transformToRoot2D == pOther->m_transformToRoot2D
            && m_additionalRasterizationScale == pOther->m_additionalRasterizationScale;
    }
}

// Compare only scales and sub-pixel offset in Y direction, if different this requires new realizations.
bool CTransformToRoot::IsSameScaleAndSubPixelOffsetAs(_In_ const CTransformToRoot *other) const
{
    if (m_is3DMode)
    {
        // Comparing 3D transforms is trickier.  For simplicity just compare the entire transform.
        return other->m_is3DMode
            && m_transformToRoot3D == other->m_transformToRoot3D
            && m_additionalRasterizationScale == other->m_additionalRasterizationScale;
    }
    else
    {
        if (!other->m_is3DMode
            && m_transformToRoot2D._11 == other->m_transformToRoot2D._11
            && m_transformToRoot2D._22 == other->m_transformToRoot2D._22
            && FractionReal(m_transformToRoot2D._32) == FractionReal(other->m_transformToRoot2D._32)
            && m_additionalRasterizationScale == other->m_additionalRasterizationScale)
        {
            const float tolerance = 0.0004f;
            ASSERT(m_transformToRoot2DWithRasterizationScale._11 == other->m_transformToRoot2DWithRasterizationScale._11
                && m_transformToRoot2DWithRasterizationScale._22 == other->m_transformToRoot2DWithRasterizationScale._22
                && DirectUI::DoubleUtil::AreWithinTolerance(FractionReal(m_transformToRoot2DWithRasterizationScale._32), FractionReal(other->m_transformToRoot2DWithRasterizationScale._32), tolerance));

            return true;
        }
        else
        {
            return false;
        }
        
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Sets this transform to another one.
//
//-------------------------------------------------------------------------
void CTransformToRoot::Set(_In_ const CTransformToRoot *pOther)
{
    m_is3DMode = pOther->m_is3DMode;
    m_transformToRoot2D = pOther->m_transformToRoot2D;
    m_transformToRoot3D = pOther->m_transformToRoot3D;
    m_additionalRasterizationScale = pOther->m_additionalRasterizationScale;
    m_transformToRoot2DWithRasterizationScale = pOther->m_transformToRoot2DWithRasterizationScale;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether this transform has 3D components.
//
//-------------------------------------------------------------------------
bool
CTransformToRoot::Is3DMode() const
{
    return m_is3DMode;
}

// Pixel Snapping:  Rounds the translation components to the nearest pixel
void CTransformToRoot::PixelSnap()
{
    m_transformToRoot2D.SetDx(static_cast<float>(XcpRound(m_transformToRoot2D.GetDx())));
    m_transformToRoot2D.SetDy(static_cast<float>(XcpRound(m_transformToRoot2D.GetDy())));
    m_transformToRoot2DWithRasterizationScale.SetDx(static_cast<float>(XcpRound(m_transformToRoot2DWithRasterizationScale.GetDx())));
    m_transformToRoot2DWithRasterizationScale.SetDy(static_cast<float>(XcpRound(m_transformToRoot2DWithRasterizationScale.GetDy())));
}
