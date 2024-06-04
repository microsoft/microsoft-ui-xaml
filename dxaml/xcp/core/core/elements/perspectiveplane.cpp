// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HitTestParams.h"

#ifdef DEBUG
    void DebugPrintInterpolator( XVERTEX25D v, WCHAR *szMSG = NULL )
    {
        RAWTRACE( TraceAlways, L"%s - [%f %f] (%f,%f,%f) *W(%f,%f,%f)", szMSG, v.x, v.y, v.uOverW, v.vOverW, v.oneOverW, v.u(), v.v(), v.w() );
    }
    void DebugPrintVertices( _In_reads_(4) XPOINTF4* ppt, WCHAR *szMSG = NULL )
    {
        RAWTRACE( TraceAlways, L"[%s]", szMSG );
        RAWTRACE( TraceAlways, L"%f %f %f %f ", ppt[0].x, ppt[0].y, ppt[0].z, ppt[0].w );
        RAWTRACE( TraceAlways, L"%f %f %f %f ", ppt[1].x, ppt[1].y, ppt[1].z, ppt[1].w );
        RAWTRACE( TraceAlways, L"%f %f %f %f ", ppt[2].x, ppt[2].y, ppt[2].z, ppt[2].w );
        RAWTRACE( TraceAlways, L"%f %f %f %f ", ppt[3].x, ppt[3].y, ppt[3].z, ppt[3].w );
    }
#else
    #define DebugPrintInterpolator( v, sz ) ;
    #define DebugPrintVertices( ppt, szMSG );
#endif

//#define DEBUG_SHOW_3D_OVERLAY

#define SWAP(Type, a, b) Type temp = a; a = b; b = temp;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Interpolates two clipping points
//
//------------------------------------------------------------------------
XCP_FORCEINLINE void Lerp3DPoint(
    _In_ const SDPoint4UV *p0,
    _In_ const SDPoint4UV *p1,
    const XFLOAT scale,
    bool isP1InsideClip,
    _Out_ SDPoint4UV *result
    )
{
    result->x = p0->x + scale * (p1->x - p0->x);
    result->y = p0->y + scale * (p1->y - p0->y);
    result->z = p0->z + scale * (p1->z - p0->z);
    result->w = p0->w + scale * (p1->w - p0->w);
    result->u = p0->u + scale * (p1->u - p0->u);
    result->v = p0->v + scale * (p1->v - p0->v);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Interpolates two clipping points.
//
//      The values of x/y/z/w are interpolated linearly here because we're
//      clipping inside the perspective, and w hasn't been divided out
//      yet. The point (x, y, z, w) will become (x/w, y/w, z/w, 1), then
//      be drawn on screen as the world space point (x/w, y/w). If we're
//      clipping the world space point, we'll need to account for w. Here
//      we can interpolate everything linearly.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE void Lerp3DPoint(
    _In_reads_(1) const XPOINTF4 *p0,
    _In_reads_(1) const XPOINTF4 *p1,
    const XFLOAT scale,
    bool isP1InsideClip,
    _Out_writes_(1) XPOINTF4 *result
    )
{
    result->x = p0->x + scale * (p1->x - p0->x);
    result->y = p0->y + scale * (p1->y - p0->y);
    result->z = p0->z + scale * (p1->z - p0->z);
    result->w = p0->w + scale * (p1->w - p0->w);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Interpolates two clipping points.
//
//------------------------------------------------------------------------
XCP_FORCEINLINE void Lerp3DPoint(
    _In_ const PointWithAAMasks *p0,
    _In_ const PointWithAAMasks *p1,
    const XFLOAT scale,
    bool isP1InsideClip,
    _Out_ PointWithAAMasks *result
    )
{
    result->x = p0->x + scale * (p1->x - p0->x);
    result->y = p0->y + scale * (p1->y - p0->y);
    result->z = p0->z + scale * (p1->z - p0->z);
    result->w = p0->w + scale * (p1->w - p0->w);

    GenerateAAMasksForClipPoint(*p0, *p1, isP1InsideClip, *result);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a line segment. Called for each segment in a polygon to
//      clip it against a plane. Adds the point of intersection between
//      the line segment and the clip plane, if it exists. Adds the
//      second point if it lies within the clip.
//
//------------------------------------------------------------------------
template <typename PointType>
void
ClipLineSegment(
    _Inout_updates_(destBufferSize) PointType *pPointDest,
    _Inout_ XUINT32 *pDestPointCount,
    XUINT32 destBufferSize,
    _In_ const PointType *pPoint0,
    XFLOAT BC0,
    _In_ const PointType *pPoint1,
    XFLOAT BC1,
    _Out_ bool *pWasClipped
    )
{
    XUINT32 pointCount = *pDestPointCount;

    //
    // BC0 and BC1 are the distances from the point to the clipping plane (e.g. pt.x - clip.left,
    // clip.right - pt.x). A point lies within the clip iff the distance is positive.
    //
    // A point p_n is associated with two edges: [p_n-1, p_n] and [p_n, p_n+1].
    // - If p_n lies inside the clip, then [p_n-1, p_n] will add the point. [p_n, p_n+1] will skip
    //   the point to prevent adding it twice.
    // - If p_n lies outside the clip, then it is never added.
    // - If edge [p_n-1, p_n] crosses the clip, then it is responsible for adding the point of
    //   intersection.
    //
    // Putting these together:
    //    p0 |  p1 | Action
    //   ----+-----+----------------------
    //    in |  in | skip p0,  add p1
    //   out |  in | skip p0,  add p1, add point of intersection
    //    in | out | skip p0, skip p1, add point of intersection
    //   out | out | skip p0, skip p1
    //

    if (BC1 >= 0)
    {
        // p1 inside
        if (BC0 < 0)
        {
            // p0 outside, p1 inside
            // output intersection of p0, p1
            const XFLOAT alpha = BC0 / (BC0 - BC1);
            ASSERT(pointCount < destBufferSize);
            Lerp3DPoint(pPoint0, pPoint1, alpha, TRUE /* isP1InsideClip */, &pPointDest[pointCount]);
            *pWasClipped = TRUE;
            pointCount++;
        }

        // output p1
        ASSERT(pointCount < destBufferSize);
        pPointDest[pointCount] = *pPoint1;
        pointCount++;
    }
    else if (BC0 >= 0)
    {
        // p0 inside, p1 outside
        // output intersection of p0, p1
        const XFLOAT alpha = BC0 / (BC0 - BC1);
        ASSERT(pointCount < destBufferSize);
        Lerp3DPoint(pPoint0, pPoint1, alpha, FALSE /* isP1InsideClip */, &pPointDest[pointCount]);
        *pWasClipped = TRUE;
        pointCount++;
    }

    *pDestPointCount = pointCount;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a polygon against the near plane of z = 0. The clip is done
//      inside the projection, before w is divided out.
//
//      pPointSource and pPointDest can't be the same buffer.
//
//------------------------------------------------------------------------
template <typename PointType>
void
ProjectionClipAgainstNearPlane(
    _In_reads_(sourcePointCount) const PointType *pPointSource,
    XUINT32 sourcePointCount,
    _Out_writes_(destBufferSize) PointType *pPointDest,
    _Inout_ XUINT32 *pDestPointCount,
    XUINT32 destBufferSize,
    _Out_ bool *pWasClipped
    )
{
    ASSERT(sourcePointCount <= destBufferSize);
    ASSERT(pPointSource != pPointDest);

    *pDestPointCount = 0;
    XUINT32 p0idx = sourcePointCount - 1;
    for (XUINT32 p1idx = 0; p1idx < sourcePointCount; p1idx++)
    {
        const XFLOAT BC0 = pPointSource[p0idx].z;
        const XFLOAT BC1 = pPointSource[p1idx].z;

        ClipLineSegment(
            pPointDest,
            pDestPointCount,
            destBufferSize,
            &pPointSource[p0idx],
            BC0,
            &pPointSource[p1idx],
            BC1,
            pWasClipped
            );

        p0idx = p1idx;
    }
}

template void ProjectionClipAgainstNearPlane(
    _In_reads_(sourcePointCount) const XPOINTF4 *pPointSource,
    XUINT32 sourcePointCount,
    _Out_writes_(destBufferSize) XPOINTF4 *pPointDest,
    _Inout_ XUINT32 *pDestPointCount,
    XUINT32 destBufferSize,
    _Out_ bool *pWasClipped
    );

template void ProjectionClipAgainstNearPlane(
    _In_reads_(sourcePointCount) const PointWithAAMasks *pPointSource,
    XUINT32 sourcePointCount,
    _Out_writes_(destBufferSize) PointWithAAMasks *pPointDest,
    _Inout_ XUINT32 *pDestPointCount,
    XUINT32 destBufferSize,
    _Out_ bool *pWasClipped
    );

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips a polygon against the view frustum
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
SDConvexPolyClipperPerform(
    _In_ const SDRect3D rcClipVolume,
    _In_reads_(cPolygonVertices) const SDPoint4UV* pPolygon,
    _In_ const XUINT32 cPolygonVertices,
    _Inout_updates_(cMaxClippedPolygonVertices) SDPoint4UV* pClippedPolygon,
    _In_ const XUINT32 cMaxClippedPolygonVertices,
    _Inout_updates_(1) XUINT32* pcClippedPolygonVertices
    )
{
    ASSERT(cPolygonVertices >= 3);
    ASSERT(cPolygonVertices <= 4); // don't support arbitrary polygons yet

    SDPoint4UV clippedPolyCurrentArray[10];
    const XUINT32 cClippedVerticesBound = MIN(10, cMaxClippedPolygonVertices);

    SDPoint4UV* pClippedPolyCurrent = clippedPolyCurrentArray;
    SDPoint4UV* pClippedPoly = pClippedPolygon;
    XUINT32 cClippedPolyVertsCurrent = cPolygonVertices;
    XUINT32 cClippedPolyVerts = 0;

    *pcClippedPolygonVertices = 0;

    // Clone the input
    IFCEXPECT_ASSERT_RETURN(cPolygonVertices <= ARRAY_SIZE(clippedPolyCurrentArray));
    memcpy(pClippedPolyCurrent, pPolygon, sizeof(SDPoint4UV) * cPolygonVertices);

    ASSERT(rcClipVolume.originZ == 0.0f);

    bool wasClipped_dontCare;
    ProjectionClipAgainstNearPlane(
        pClippedPolyCurrent /* pPointSource */,
        cClippedPolyVertsCurrent /* sourcePointCount */,
        pClippedPoly /* pPointDest */,
        &cClippedPolyVerts /* pDestPointCount */,
        cClippedVerticesBound /* destBufferSize */,
        &wasClipped_dontCare
        );

    if (cClippedPolyVerts == 0)
    {
        return S_OK;
    }

    // Swap the current with the output
    SWAP(SDPoint4UV*, pClippedPoly, pClippedPolyCurrent);
    cClippedPolyVertsCurrent = cClippedPolyVerts;

    // Set the output points
    if (pClippedPolyCurrent != pClippedPolygon)
    {
        IFCEXPECT_ASSERT_RETURN( cClippedPolyVertsCurrent <= cMaxClippedPolygonVertices );
        memcpy(pClippedPolygon, pClippedPolyCurrent, sizeof(SDPoint4UV) * cClippedPolyVertsCurrent);
    }

    // Set the output number
    *pcClippedPolygonVertices = cClippedPolyVertsCurrent;

    return S_OK;
}

// Given the element's size, the size of the quad to project,
// coordinates for the corners of the texture, and any clips to
// apply, do the projection.
_Check_return_ HRESULT
CProjection::BuildProjectionCommon(
    _In_ const XSIZEF &elementSize,
    _In_ const XRECTF_RB &quadPadding,
    _In_ const XRECTF_RB &textureBounds,
    _Inout_opt_ XPOINTF4 *pPtsOut,
    _Out_ XUINT32* pointCount,
    _Out_opt_ bool *pAreAllWValuesPositive
    )
{
    SDPoint4UV pQuad[4];
    XPOINTF pScreenSpacePoints[c_nMaxClippedVertexCount];
    SDPoint4UV pClippedPolygon[c_nMaxClippedVertexCount];
    XUINT32 uClippedPolygonVerticesCount = 0;

    Create3DQuad(
        elementSize,
        quadPadding,
        textureBounds,
        pQuad,
        NULL    // out param - projection pipeline matrix (not needed here)
        );

    // pHomogeneousSpacePoints are the corners of the projection quad in screen space with all
    // transforms applied. The projection parameter (w) is set, but hasn't been divided out yet
    // so the points are still in 3D. Next we clip the quad to our view boundaries, then divide
    // out the w parameter to project the quad to a 2D plane.
    IFC_RETURN(Clip3DQuad(
        pQuad,
        &uClippedPolygonVerticesCount,
        pClippedPolygon
        ));

    if (uClippedPolygonVerticesCount <= 2)
    {
        *pointCount = 0;
    }
    else
    {
        ProjectToPlane(
            pClippedPolygon,
            uClippedPolygonVerticesCount,
            pScreenSpacePoints,
            pPtsOut,
            pAreAllWValuesPositive,
            pointCount
            );
    }

    return S_OK;
}

// Creates a 3D quad to fill with the perspective brush span. The
// quad will have any transforms specified on the <PlaneProjection>
// applied and will have the perspective parameter (w). It is ready
// to be clipped and projected to a plane.
void
CProjection::Create3DQuad(
    _In_ const XSIZEF &elementSize,
    _In_ const XRECTF_RB &quadPadding,
    _In_ const XRECTF_RB &textureBounds,
    _Out_writes_(4) SDPoint4UV *pQuad,
    _Out_opt_ CMILMatrix4x4 *pProjectionPipeline
    )
{
    // Build 3D Pipeline = [Projection Matrix] x [World Transform]
    CMILMatrix4x4 matPipeline = GetOverallProjectionMatrix(elementSize);

    HitTestParams::Create3DQuad(elementSize, quadPadding, textureBounds, pQuad, &matPipeline);

    if (pProjectionPipeline)
    {
        *pProjectionPipeline = matPipeline;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clips the 3D polygon. The polygon is clipped against the target
//      vector buffer's size, if it exists, and against a set of optionally
//      provided bounds.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CProjection::Clip3DQuad(
    _In_reads_(4) SDPoint4UV *pQuad,
    _Deref_out_range_(0, c_nMaxClippedVertexCount) XUINT32 *puClippedPolygonVerticesCount,
    _Out_writes_(c_nMaxClippedVertexCount) SDPoint4UV *pClippedPolygon
    )
{
    SDRect3D clipBounds;
    clipBounds.originX = 0.0f;
    clipBounds.originY = 0.0f;
    clipBounds.originZ = 0;
    clipBounds.sizeX = XFLOAT_MAX;
    clipBounds.sizeY = XFLOAT_MAX;
    clipBounds.sizeZ = 1;

    IFC_RETURN(SDConvexPolyClipperPerform(
        clipBounds,
        pQuad,
        4,  // quad vertex count
        pClippedPolygon,
        c_nMaxClippedVertexCount,
        puClippedPolygonVerticesCount
        ));

    IFCEXPECT_ASSERT_RETURN(*puClippedPolygonVerticesCount <= c_nMaxClippedVertexCount);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Divides a 3D quad by its w coordinates to project the 3D points
//      onto a plane with constant z. The z can then be dropped and we
//      get the points for the projected quad.
//
//------------------------------------------------------------------------
void
CProjection::ProjectToPlane(
    _In_reads_(uiClippedPolygonVertices) SDPoint4UV *pClippedPolygon,
    __range(3, c_nMaxClippedVertexCount) XUINT32 uiClippedPolygonVertices,
    _Out_writes_(uiClippedPolygonVertices) XPOINTF *pScreenSpacePoints,
    _Inout_opt_ XPOINTF4 *pPtsOut,
    _Out_opt_ bool *pAreAllWValuesPositive,
    _Out_ XUINT32* pointCount
    )
{
    //
    // Since we divide by w at the end, having a negative value could mess up the winding order and
    // having 0 results in a divide by 0. Near plane clipping should have removed points that would
    // generate bad w values for PlaneProjections, but Matrix3DProjections use an arbitrary matrix
    // and can still produce bad w values. Catch them here.
    //
    bool areAllWValuesPositive = true;

    //
    // Divide by w. This projects all vertices onto a plane with constant z (see
    // projection matrix set up in CMILMatrix4x4::SetToPerspective). We then drop
    // the z value to get the projected points in 2D.
    //
    for (XUINT32 i = 0; i < uiClippedPolygonVertices; i++)
    {
        pScreenSpacePoints[i].x = pClippedPolygon[i].x / pClippedPolygon[i].w;
        pScreenSpacePoints[i].y = pClippedPolygon[i].y / pClippedPolygon[i].w;

        if (pPtsOut)
        {
            pPtsOut[i].x = pScreenSpacePoints[i].x;
            pPtsOut[i].y = pScreenSpacePoints[i].y;
            pPtsOut[i].z = pClippedPolygon[i].z / pClippedPolygon[i].w;
            pPtsOut[i].w = 1 / pClippedPolygon[i].w;
        }

        areAllWValuesPositive = areAllWValuesPositive && (pClippedPolygon[i].w > 0);
    }

    // Store the actual number of points written into the path, since the entire array
    // may not have been filled.
    *pointCount = uiClippedPolygonVertices;

    if (pAreAllWValuesPositive != NULL)
    {
        *pAreAllWValuesPositive = areAllWValuesPositive;
    }
}


// Creates an abstract transformation object capable of transforming XPOINTF up and down the object hierarchy.
// Shared by old and new walks.
void CProjection::GetTransformerCommon(
    _In_ const XSIZEF &elementSize,
    _Outptr_ CPerspectiveTransformer **ppTransformer)
{
    XRECTF_RB quadPadding = {0, 0, 0, 0};
    CMILMatrix identity(TRUE);
    XRECTF_RB textureBounds = {0, 0, elementSize.width, elementSize.height};
    SDPoint4UV pQuad[4];
    CMILMatrix4x4 matPipeline;

    Create3DQuad(
        elementSize,
        quadPadding,
        textureBounds,
        pQuad,
        &matPipeline // out param - the matrix that transforms from inner projection space to outer projection space. Passed into GetTransformerCommonHelper.
        );

    HitTestParams::GetTransformerCommonHelper(&matPipeline, elementSize, ppTransformer);
}

// Calculates the contents bounds of the element as it is clipped and projected to the screen.
_Check_return_ HRESULT
CProjection::GetContentBounds(
    _In_ const XSIZEF &elementSize,
    _In_ const XRECTF_RB &quadPadding,
    _Out_ XRECTF_RB* pContentBounds,
    _Out_ bool *pAreAllWValuesPositive
    )
{
    XRECTF_RB textureBounds = {0, 0, elementSize.width, elementSize.height};
    XUINT32 pointCount;
    XPOINTF4 pPtsOut[c_nMaxClippedVertexCount];
    pPtsOut[0].x = pPtsOut[0].y = 0;

    IFC_RETURN(BuildProjectionCommon(
        elementSize,
        quadPadding,
        textureBounds,
        pPtsOut,
        &pointCount,
        pAreAllWValuesPositive
        ));

    HitTestParams::GetContentBoundsHelper(pPtsOut, pointCount, pContentBounds);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the cached model transform from public fields
//
//------------------------------------------------------------------------
void
CPlaneProjection::Update3DTransform( _In_ XFLOAT rWidth, _In_ XFLOAT rHeight )
{
    if (m_is3DTransformDirty)
    {
        CMILMatrix4x4 matTemp;
        CMILMatrix4x4 matFwdCenter;
        CMILMatrix4x4 matBackCenter;

        // Compute relative CenterOfRotationXY
        XFLOAT rCenterOfRotationXInPixels = (m_rCenterOfRotationX - 0.5f) * rWidth;
        XFLOAT rCenterOfRotationYInPixels = (m_rCenterOfRotationY - 0.5f) * rHeight;

        //
        // Compute 3D transformation matrix
        //

        // Compute off-center helpers
        matFwdCenter.SetToTranslation( -rCenterOfRotationXInPixels, -rCenterOfRotationYInPixels, -m_rCenterOfRotationZ );
        matBackCenter.SetToTranslation( rCenterOfRotationXInPixels, rCenterOfRotationYInPixels, m_rCenterOfRotationZ );

        // Apply local offset translation
        m_mat3DTransform.SetToTranslation( m_rLocalOffsetX, m_rLocalOffsetY, m_rLocalOffsetZ );

        // Apply rotations
        matTemp.SetToRotationX(m_rRotationX);
        matTemp.Prepend(matFwdCenter);
        matTemp.Append(matBackCenter);
        m_mat3DTransform.Append(matTemp);

        matTemp.SetToRotationY(m_rRotationY);
        matTemp.Prepend(matFwdCenter);
        matTemp.Append(matBackCenter);
        m_mat3DTransform.Append(matTemp);

        matTemp.SetToRotationZ(m_rRotationZ);
        matTemp.Prepend(matFwdCenter);
        matTemp.Append(matBackCenter);
        m_mat3DTransform.Append(matTemp);

        // Apply global offset translation
        matTemp.SetToTranslation( m_rGlobalOffsetX, m_rGlobalOffsetY, m_rGlobalOffsetZ );
        m_mat3DTransform.Append(matTemp);

        m_is3DTransformDirty = false;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The accessor to PlaneProjection to get the 3D matrix used to create a given
//      plane projection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPlaneProjection::GetProjectionMatrix(
    _In_ CDependencyObject *pObject,
    _In_ XUINT32 cArgs,
    _In_reads_(cArgs) CValue *pArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult)
{
    CPlaneProjection *pPlaneProjection = nullptr;
    CMatrix4x4* pCMatrix4x4 = nullptr;

    IFC_RETURN(cArgs == 0 && pObject && pResult ? S_OK : E_INVALIDARG);
    IFC_RETURN(DoPointerCast(pPlaneProjection, pObject));

    CREATEPARAMETERS cp(pPlaneProjection->GetContext());
    IFC_RETURN(CMatrix4x4::Create((CDependencyObject**)&pCMatrix4x4, &cp));

    // Note that here we simply copy m_matTransform3D to m_matrix for the accessor, and m_matTransform3D might not have been
    // updated before an Update3DTransform() call.
    pCMatrix4x4->m_matrix = pPlaneProjection->m_mat3DTransform;
    pResult->SetObjectNoRef(pCMatrix4x4);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      CMatrix3DProjection destructor
//
//------------------------------------------------------------------------
CMatrix3DProjection::~CMatrix3DProjection()
{
    ReleaseInterface(m_pMatrix);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the cached model transform from public fields
//
//------------------------------------------------------------------------
void
CMatrix3DProjection::Update3DTransform( _In_ XFLOAT rWidth, _In_ XFLOAT rHeight )
{
    if (m_is3DTransformDirty)
    {
        CMILMatrix4x4 matTemp;

        m_mat3DTransform = m_pMatrix ? m_pMatrix->m_matrix : CMILMatrix4x4(TRUE);

        // Undo the translations that will be prepended and appended to the
        // transform pipeline.
        matTemp.SetToTranslation( +rWidth/2.0f, +rHeight/2.0f, 0.0f );
        m_mat3DTransform.Prepend(matTemp);
        matTemp.SetToTranslation( -rWidth/2.0f, -rHeight/2.0f, 0.0f );
        m_mat3DTransform.Append(matTemp);

        m_is3DTransformDirty = false;
    }
}
