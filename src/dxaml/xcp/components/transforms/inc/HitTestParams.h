// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Transform3D.h"

#include <GraphicsUtility.h>

#include "paltypes.h"
#include "transforms.h"
#include "palgfx.h"
#include "HitTestPolygon.h"

struct XVERTEX25D;

// Perf bookkeeping for a hit test walk.
// This is kept in a separate struct as HitTestParams. As we walk down the tree we create new HitTestParams
// on the stack to hold modified transforms and such, but the perf data that we collect points to the same
// bag of numbers.
struct HitTestPerfData
{
    // Note: DirectXMath offers SIMD-friendly vector and matrix types along with optimized matrix operations.
    // That's one alternative if we see high numbers of matrix math during the hit testing (and rendering walk).
    // The conversion touches a lot of code, though. XMVECTOR should be filled with specialized load functions:
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ee420742(v=vs.85).aspx

    unsigned int m_outerBoundsRecalc = 0;
    unsigned int m_outerBoundsReuse = 0;
    unsigned int m_innerBoundsRecalc = 0;
    unsigned int m_innerBoundsReuse = 0;
    unsigned int m_contentBoundsRecalc = 0;
    unsigned int m_contentBoundsReuse = 0;
    unsigned int m_childBoundsRecalc = 0;
    unsigned int m_childBoundsReuse = 0;
};

// Parameters for Transform3D HitTesting Walk
class HitTestParams
{
public:
    HitTestParams(_In_ HitTestPerfData* hitTestPerfData);

    HitTestParams(_In_opt_ const HitTestParams* inParams);

    CMILMatrix4x4 GetCombinedTransformMatrix();

    void UpdateCombinedTransformMatrix(_In_ const CMILMatrix *transformMatrix);

    void UpdateCombinedTransformMatrix(_In_ const CMILMatrix4x4* transformMatrix);

    void AppendTransformMatrix(_In_ const CMILMatrix *transformMatrix);

    void AppendTransformMatrix(_In_ const CMILMatrix4x4* transformMatrix);

    void GetTransformerCommon(_Outptr_ CPerspectiveTransformer **ppTransformer);

    static void
    Create3DQuad(
        _In_ const XSIZEF &elementSize,
        _In_ const XRECTF_RB &quadPadding,
        _In_ const XRECTF_RB &textureBounds,
        _Out_writes_(4) SDPoint4UV *pQuad,
        _In_opt_ const CMILMatrix4x4 *pProjectionPipeline
    );

    void GetContentBounds(
        _In_ const XSIZEF &elementSize,
        _In_ const XRECTF_RB &quadPadding,
        _Out_ XRECTF_RB* pContentBounds
    );

    void TransformBounds(
        _In_ const XRECTF_RB &elementBounds,
        _Out_ XRECTF_RB* transformedBounds
    );

    static void GetTransformerCommonHelper(
        _In_ const CMILMatrix4x4 *matPipeline,
        _In_ const XSIZEF &elementSize,
        _Outptr_ CPerspectiveTransformer **ppTransformer);

    static void
    SetUpTextureInterpolators(
        _In_reads_(uiClippedPolygonVertices) XPOINTF *pScreenSpacePoints,
        _In_reads_(uiClippedPolygonVertices) SDPoint4UV *pClippedPolygon,
        _In_range_(3, c_nMaxClippedVertexCount) XUINT32 uiClippedPolygonVertices,
        _Out_ XVERTEX25D *pTopLeft,
        _Out_ XVERTEX25D *pDX,
        _Out_ XVERTEX25D *pDY,
        _Out_ bool *pfIsSingularProjection
    );

    static void
    GetContentBoundsHelper(
        _In_ XPOINTF4 *pPtsOut,
        const int numPoints,
        _Out_ XRECTF_RB* pContentBounds
    );

    void SaveWorldSpaceHitTarget(const XPOINTF& point);
    void SaveWorldSpaceHitTarget(const HitTestPolygon& polygon);

    void GetWorldSpaceHitTarget(XPOINTF& point);
    void GetWorldSpaceHitTarget(HitTestPolygon& polygon);

    bool GetWorldSpacePointInLocalSpace(XPOINTF& localSpacePoint) const;
    bool GetWorldSpaceRectInLocalSpace(HitTestPolygon& localSpacePolygon) const;

private:
    void SetDefaultValues();

public:
    HitTestPerfData* m_hitTestPerfData{nullptr};

    // combinedTransformMatrix collects the matrix representations of transforms during the hit testing walk.
    // It represents a collection of every transform on every ancestor of "this" element, up to the root.
    CMILMatrix4x4 combinedTransformMatrix;

    // The world space point/rect that kicked off the hit testing walk. Typically we transform the point as we walk down
    // the tree, but this doesn't work once we encounter an element with a 3D transform on it (e.g. a 90 degree rotation
    // that later gets canceled by another 90 degree rotation). Instead, we'll collect a cumulative transform on the way
    // down to 3D elements, then transform the original world space point/rect directly into the local plane of the 3D
    // element.
    XPOINTF originalPoint { 0, 0 };
    XRECTF originalRect { 0, 0, 0, 0 };

    // TODO: HitTest: remove
    bool hasTransform3DInSubtree;
};
