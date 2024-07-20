// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HitTestParams.h"
#include "Transform3D.h"

HitTestParams::HitTestParams(_In_ HitTestPerfData* hitTestPerfData)
{
    m_hitTestPerfData = hitTestPerfData;
    SetDefaultValues();
}

HitTestParams::HitTestParams(_In_opt_ const HitTestParams* inParams)
{
    if (inParams != nullptr)
    {
        m_hitTestPerfData = inParams->m_hitTestPerfData;
        combinedTransformMatrix = inParams->combinedTransformMatrix;
        originalPoint = inParams->originalPoint;
        originalRect = inParams->originalRect;
        hasTransform3DInSubtree = inParams->hasTransform3DInSubtree;
    }
    else
    {
        SetDefaultValues();
    }
}

void HitTestParams::SetDefaultValues()
{
    combinedTransformMatrix.SetToIdentity();
    originalPoint = {0,0};
    hasTransform3DInSubtree = false;
}

CMILMatrix4x4 HitTestParams::GetCombinedTransformMatrix()
{
    return combinedTransformMatrix;
}

void HitTestParams::UpdateCombinedTransformMatrix(_In_ const CMILMatrix* transformMatrix)
{
    CMILMatrix4x4 temp(transformMatrix); // Convert the incoming matrix to a (flat) 4x4

    // TODO: HitTest: Make the ctor never flatten
    temp._33 = 1.0f; // Don't flatten.

    combinedTransformMatrix.Prepend(temp);
}

void HitTestParams::UpdateCombinedTransformMatrix(_In_ const CMILMatrix4x4* transformMatrix)
{
    combinedTransformMatrix.Prepend(*transformMatrix);
}

// Used when we go up the tree (from leaf to root)
void HitTestParams::AppendTransformMatrix(_In_ const CMILMatrix* transformMatrix)
{
    CMILMatrix4x4 temp(transformMatrix); // Convert the incoming matrix to a (flat) 4x4

    temp._33 = 1.0f; // Don't flatten.

    combinedTransformMatrix.Append(temp);
}

// Used when we go up the tree (from leaf to root)
void HitTestParams::AppendTransformMatrix(_In_ const CMILMatrix4x4* transformMatrix)
{
    combinedTransformMatrix.Append(*transformMatrix);
}

void HitTestParams::GetTransformerCommon(_Outptr_ CPerspectiveTransformer **ppTransformer)
{
    CMILMatrix4x4 matPipeline;

    // The size passed in can be arbitrary. It's used to create the 3D quad that will be transformed to world space
    // as well as set up the texture coordinates that will bookkeep local space. It doesn't matter what the size is,
    // as long as we use the same size for both.
    XSIZEF elementSize { 100, 100 };

    matPipeline = GetCombinedTransformMatrix();

    HitTestParams::GetTransformerCommonHelper(&matPipeline, elementSize, ppTransformer);
}

// Creates a 3D quad to fill with the transform brush span. The
// quad will have any transforms specified on the Transform
// applied and will have the perspective parameter (w).
/* static */ void
HitTestParams::Create3DQuad(
    _In_ const XSIZEF &elementSize,
    _In_ const XRECTF_RB &quadPadding,
    _In_ const XRECTF_RB &textureBounds,
    _Out_writes_(4) SDPoint4UV *pQuad,
    _In_opt_ const CMILMatrix4x4 *pProjectionPipeline
    )
{
    // Create a 3D quad on the XY plane. Start by creating a rectangle
    // with the size of the element having its top left corner at (0,0),
    // then adjust its bounds by the amount specified in quadPadding.
    // See comment in CUIElement::NWDrawProjection for more details.
    //
    // The values in quadPadding are positive if the quad is outside the
    // element. For right & bottom positive means to the right and below,
    // so they're added. For left & top negative means to the right and
    // below, so they're negated.

    XPOINTF4 pts[4];
    pts[0].x = pts[3].x = -quadPadding.left;
    pts[0].y = pts[1].y = -quadPadding.top;
    pts[1].x = pts[2].x = elementSize.width + quadPadding.right;
    pts[2].y = pts[3].y = elementSize.height + quadPadding.bottom;
    pts[0].z = pts[1].z = pts[2].z = pts[3].z = 0.0f;
    pts[0].w = pts[1].w = pts[2].w = pts[3].w = 1.0f;

    // Transform the 3D points through the pipeline
    XPOINTF4 pHomogeneousSpacePoints[4];
    pProjectionPipeline->Transform_PreserveW(pts, pHomogeneousSpacePoints);

    // Create the 3D quad and initialize the texture coordinates
    for (XINT32 i = 0; i < 4; i++)
    {
        pQuad[i].x = pHomogeneousSpacePoints[i].x;
        pQuad[i].y = pHomogeneousSpacePoints[i].y;
        pQuad[i].z = pHomogeneousSpacePoints[i].z;
        pQuad[i].w = pHomogeneousSpacePoints[i].w;
    }
    pQuad[0].v = pQuad[1].v = textureBounds.top;
    pQuad[1].u = pQuad[2].u = textureBounds.right;
    pQuad[2].v = pQuad[3].v = textureBounds.bottom;
    pQuad[3].u = pQuad[0].u = textureBounds.left;
}

// Calculates the contents bounds of the element as it is transformed
void HitTestParams::TransformBounds(
    _In_ const XRECTF_RB &elementBounds,
    _Out_ XRECTF_RB* transformedBounds
    )
{
    XRECTF elementBoundsWH = ToXRectF(elementBounds);
    XRECTF transformedBoundsWH;

    const auto& transformMatrix = GetCombinedTransformMatrix();
    transformMatrix.TransformBoundsIgnoreZ(&elementBoundsWH, &transformedBoundsWH);

    *transformedBounds = ToXRectFRB(transformedBoundsWH);
}

// Calculates the contents bounds of the element as it is transformed
void HitTestParams::GetContentBounds(
    _In_ const XSIZEF &elementSize,
    _In_ const XRECTF_RB &quadPadding,
    _Out_ XRECTF_RB* pContentBounds
    )
{
    const int numPoints = 4; // There are only four points in the unclipped bounding box
    XPOINTF4 pPtsOut[numPoints];
    SDPoint4UV pQuad[numPoints];
    XRECTF_RB textureBounds = {0, 0, elementSize.width, elementSize.height};
    CMILMatrix4x4 matPipeline = GetCombinedTransformMatrix();

    pPtsOut[0].x = pPtsOut[0].y = 0;

    Create3DQuad(
        elementSize,
        quadPadding,
        textureBounds,
        pQuad,
        &matPipeline
        );

    for (int i = 0; i < numPoints; i++)
    {
        pPtsOut[i].x = pQuad[i].x / pQuad[i].w;
        pPtsOut[i].y = pQuad[i].y / pQuad[i].w;
        pPtsOut[i].z = pQuad[i].z / pQuad[i].w;
    }

    HitTestParams::GetContentBoundsHelper(pPtsOut, numPoints, pContentBounds);
}

/* static */ void HitTestParams::GetTransformerCommonHelper(
    _In_ const CMILMatrix4x4 *matPipeline,
    _In_ const XSIZEF &elementSize,
    _Outptr_ CPerspectiveTransformer **ppTransformer)
{
    // To transform from inner projection space to outer projection space, we need the
    // projection pipeline matrix. To transform from outer projection space to inner
    // projection space, we need the perspective interpolator values. The helper methods
    // used in BuildProjectionCommon can be used here to get the needed information.
    CPerspectiveTransformer *pTransformer = NULL;
    XPOINTF pProjected2DPoints[4];
    SDPoint4UV pQuad[4];
    bool fIsSingularProjection = false;
    XVERTEX25D vTopLeft, vDX, vDY;
    ASSERT(vDX.IsIdentity());
    ASSERT(vDY.IsIdentity());

    // When rendering the projection we had an intermediate surface that may not have
    // the same bounds as the element, and we needed to account for the differences
    // when creating the quad. When transforming points between coordinate spaces we
    // don't have to worry about such differences. We effectively always have a texture
    // that's the same size as the element, so we can always create the quad at the same
    // size as the element.
    XRECTF_RB quadPadding = {0, 0, 0, 0};
    XRECTF_RB textureBounds = {0, 0, elementSize.width, elementSize.height};

    // When rendering the projection we needed to provide the world transform so that
    // the quad is created in screen space. The screen space coordinates can then be used
    // to create the perspective brush span that maps (x,y) screen space pixels to (u,v)
    // texture coordinates. When transforming points between coordinate spaces, we need
    // to provide the local space because this transformer is only responsible for the
    // projection itself. There will be other transformers chained ahead of us to handle
    // the other transforms.
    Create3DQuad(
        elementSize,
        quadPadding,
        textureBounds,
        pQuad,
        matPipeline
        );

    // The transformer is only responsible for transforming points between coordinate
    // spaces, so there's no need to handle clipping. Project the 3D points directly to
    // 2D space.
    for (XINT32 i = 0; i < 4; i++)
    {
        pProjected2DPoints[i].x = pQuad[i].x / pQuad[i].w;
        pProjected2DPoints[i].y = pQuad[i].y / pQuad[i].w;
    }

    // Set up the texture interpolators that will be used to transform from outer projection
    // space to inner projection space. Note that there are always 4 points since we don't
    // handle clipping.
    SetUpTextureInterpolators(
        pProjected2DPoints,
        pQuad,
        4,  // vertex count
        &vTopLeft,
        &vDX,
        &vDY,
        &fIsSingularProjection
        );

    // Create the 3D transformer. If the projection is singular and non-invertible, vDX and
    // vDY will be their default values which will have no effect during the reverse transform,
    // making all outer projection points transform to (0, 0) in inner projection space. Also
    // note that if we're forcing this behavior, then we don't apply the 0.5 pixel texture
    // offset adjustment.
    pTransformer = new CPerspectiveTransformer(
        *matPipeline,
        vTopLeft,
        vDX,
        vDY,
        fIsSingularProjection
        );

    *ppTransformer = pTransformer;
}

// Calculate the values needed by the perspective texture brush span
// to fill the projected quad. We need the screen space (x,y)
// coordinates that correspond to (0,0) in projected space, and the
// changes in (u/w, v/w, 1/w) from moving 1 pixel right and from
// moving 1 pixel down in screen space.
/* static */ void
HitTestParams::SetUpTextureInterpolators(
    _In_reads_(uiClippedPolygonVertices) XPOINTF *pScreenSpacePoints,
    _In_reads_(uiClippedPolygonVertices) SDPoint4UV *pClippedPolygon,
    _In_range_(3, c_nMaxClippedVertexCount) XUINT32 uiClippedPolygonVertices,
    _Out_ XVERTEX25D *pTopLeft,
    _Out_ XVERTEX25D *pDX,
    _Out_ XVERTEX25D *pDY,
    _Out_ bool *pfIsSingularProjection
    )
{

    // First define the projected space. Find three consecutive vertices in
    // the clipped polygon that are not collinear and among which there are
    // no coincident vertices. We do this by finding the three consecutive
    // vertices for which the magnitude of the cross product of the vectors
    // spanning the vertices is greatest. These vertices then produce the
    // axes for the projected space.
    XUINT32 v0Idx = uiClippedPolygonVertices - 1;
    XUINT32 v1Idx = 0;
    XUINT32 v2Idx = 1;

    XFLOAT maxCrossProductMagnitude = XFLOAT_MIN;
    XUINT32 v1IdxSelected = 0;

    do
    {
        XFLOAT crossProductMagnitude = XcpAbsF(
            (pScreenSpacePoints[v0Idx].x - pScreenSpacePoints[v1Idx].x) * (pScreenSpacePoints[v2Idx].y - pScreenSpacePoints[v1Idx].y) -
            (pScreenSpacePoints[v0Idx].y - pScreenSpacePoints[v1Idx].y) * (pScreenSpacePoints[v2Idx].x - pScreenSpacePoints[v1Idx].x));

        if (crossProductMagnitude > maxCrossProductMagnitude)
        {
            maxCrossProductMagnitude = crossProductMagnitude;
            v1IdxSelected = v1Idx;
        }

        v1Idx++;
        v0Idx = v1Idx - 1;
        v2Idx = (v2Idx+1) % uiClippedPolygonVertices;
    }
    while (v1Idx < uiClippedPolygonVertices);

    // Reset the vertex indices to the set of three that our criteria selected.
    v0Idx = (v1IdxSelected - 1 + uiClippedPolygonVertices) % uiClippedPolygonVertices;
    v1Idx = v1IdxSelected;
    v2Idx = (v1IdxSelected + 1) % uiClippedPolygonVertices;

    // Create the vertices. The XVERTEX25D constructor automatically calculates u/w,
    // v/w, and 1/w.
    XVERTEX25D v0(
            pScreenSpacePoints[v0Idx].x,
            pScreenSpacePoints[v0Idx].y,
            pClippedPolygon[v0Idx].u,
            pClippedPolygon[v0Idx].v,
            pClippedPolygon[v0Idx].w
            );

    XVERTEX25D v1(
            pScreenSpacePoints[v1Idx].x,
            pScreenSpacePoints[v1Idx].y,
            pClippedPolygon[v1Idx].u,
            pClippedPolygon[v1Idx].v,
            pClippedPolygon[v1Idx].w
            );

    XVERTEX25D v2(
            pScreenSpacePoints[v2Idx].x,
            pScreenSpacePoints[v2Idx].y,
            pClippedPolygon[v2Idx].u,
            pClippedPolygon[v2Idx].v,
            pClippedPolygon[v2Idx].w
            );

    XPOINTF vRightOne, vDownOne;

    // v0, v1, v2 are three adjacent points on the polygon that are non-collinear. They will
    // be used to form the new coordinate system of the projected clipped quad. Their .x
    // and .y values are their screen space coordinates, and their .uOverW, .vOverW, and
    // .oneOverW values are used to linearly interpolate the texture coordinates over the
    // quad.
    //
    // v1 will be used as the origin of the new coordinate system and is used as the
    // reference point for interpolating texture coordinates. v0 is used to produce
    // one axis, and v2 is used to produce another.
    *pTopLeft = v1;

    XVERTEX25D vRight = v0 - v1;
    XVERTEX25D vDown = v2 - v1;

    // vRight and vDown are the two axes of the projected coordinate system, with v1 at
    // the origin. These vectors are not collinear (but not necessarily perpendicular)
    // and do not have length 1. vRight does not necessarily point to the right, and
    // vDown does not necessarily point down. Their .x and .y values represent coordinates
    // in screen space, and their .uOverW, .vOverW, and .oneOverW values are the changes
    // in u/w, v/w, and 1/w from moving along these two vectors.
    //
    // Next we pick two linear combinations of vRight and vDown. One combination corresponds
    // to (1,0) in screen space, and the other corresponds to (0,1). These combinations
    // represent the changes in u/w, v/w, and 1/w from moving one pixel to the right in
    // screen space and from moving one pixel down in screen space. These combinations can
    // then be used for interpolating texture coordinates during rasterization.

    // Create a Mapping between the space defined by vRight and vDown
    // and the space defined by (1,0) and (0,1) in screen space:
    CMILMatrix matVectorBaseAdjust;
    matVectorBaseAdjust._11 = vRight.x;
    matVectorBaseAdjust._12 = vRight.y;
    matVectorBaseAdjust._21 = vDown.x;
    matVectorBaseAdjust._22 = vDown.y;
    matVectorBaseAdjust._31 = 0.0f;
    matVectorBaseAdjust._32 = 0.0f;

    // matVectorBaseAdjust now maps from vRight/vDown space to screen space.
    // Invert it to get a mapping from screen space to vRight/vDown space.
    if (!matVectorBaseAdjust.Invert())
    {
        pDX->SetToIdentity();
        pDY->SetToIdentity();

        *pfIsSingularProjection = TRUE;
    }
    else
    {
        // Now map (1,0) to get the equivalent of one pixel right in vRight/vDown space.
        vRightOne.x = 1.0f;
        vRightOne.y = 0.0f;
        matVectorBaseAdjust.Transform(&vRightOne, &vRightOne, 1);

        // vRightOne is now one pixel right in screen space, measured in the
        // vRight/vDown space. Its .x and .y values aren't in screen space
        // anymore, but are rather distances along the vRight (.x) and vDown
        // (.y) axes that correspond to one pixel right. We can now use vRightOne
        // to get the linear combination of vRight and vDown that corresponds
        // to one pixel right in screen space.

        // This vector's uOverW, vOverW, and oneOverW are changes in u/w, v/w,
        // and 1/w from moving one pixel right in screen space.
        *pDX = (vRight * vRightOne.x) + (vDown * vRightOne.y);

        // Now map (1,0) to get the equivalent of one pixel down in vRight/vDown space.
        vDownOne.x = 0.0f;
        vDownOne.y = 1.0f;
        matVectorBaseAdjust.Transform(&vDownOne, &vDownOne, 1);

        // vDownOne is now one pixel down in screen space, measured in the
        // vRight/vDown space. Its .x and .y values aren't in screen space
        // anymore, but are rather distances along the vRight (.x) and vDown
        // (.y) axes that correspond to one pixel down. We can now use vDownOne
        // to get the linear combination of vRight and vDown that corresponds
        // to one pixel down in screen space.

        // This vector's uOverW, vOverW, and oneOverW are changes in u/w, v/w,
        // and 1/w from moving one pixel down in screen space.
        *pDY = (vRight * vDownOne.x) + (vDown * vDownOne.y);

        *pfIsSingularProjection = FALSE;
    }
}

// Calculate the content bounds of the transformed element
/*static*/ void HitTestParams::GetContentBoundsHelper(
    _In_ XPOINTF4 *pPtsOut,
    const int numPoints,
    _Out_ XRECTF_RB* pContentBounds
    )
{
    XFLOAT rXMin,rXMax,rYMin,rYMax;

    rXMin = rXMax = pPtsOut[0].x;
    rYMin = rYMax = pPtsOut[0].y;

    for (int i = 0; i < numPoints; i++)
    {
        rXMin = MIN( rXMin, pPtsOut[i].x );
        rXMax = MAX( rXMax, pPtsOut[i].x );
        rYMin = MIN( rYMin, pPtsOut[i].y );
        rYMax = MAX( rYMax, pPtsOut[i].y );
    }

    pContentBounds->left = rXMin;
    pContentBounds->right = rXMax;
    pContentBounds->top = rYMin;
    pContentBounds->bottom = rYMax;
}

void HitTestParams::SaveWorldSpaceHitTarget(const XPOINTF& point)
{
    originalPoint = point;
}

void HitTestParams::SaveWorldSpaceHitTarget(const HitTestPolygon& polygon)
{
    ASSERT(polygon.IsRect());
    originalRect = polygon.GetRect();
}

void HitTestParams::GetWorldSpaceHitTarget(XPOINTF& point)
{
    point = originalPoint;
}

void HitTestParams::GetWorldSpaceHitTarget(HitTestPolygon& polygon)
{
    polygon.SetRect(originalRect);
}

bool HitTestParams::GetWorldSpacePointInLocalSpace(XPOINTF& localSpacePoint) const
{
    // TODO: HitTest: check for negative W value in CMILMatrix4x4 and either: accept negative W, disqualify point, or clip it ahead of time in UIElement.
    return combinedTransformMatrix.TransformWorldToLocalWithInverse(originalPoint, localSpacePoint);
}

bool HitTestParams::GetWorldSpaceRectInLocalSpace(HitTestPolygon& localSpacePolygon) const
{
    localSpacePolygon.SetRect(originalRect);
    return localSpacePolygon.TransformWorldToLocalWithInverse(combinedTransformMatrix);
}
