// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"
#include "RenderParams.h"
#include "Matrix.h"
#include <fwd/Windows.UI.Composition.h>

struct IPALSurface;
struct XVERTEX25D;
class CPerspectiveTransformer;
class WinRTExpressionConversionContext;
struct SDPoint4UV;

const XUINT32 c_nMaxClippedVertexCount = 10;

// 3D rect
struct SDRect3D
{
    XFLOAT originX, originY, originZ;
    XFLOAT sizeX, sizeY, sizeZ;
};

template <typename PointType>
void
ProjectionClipAgainstNearPlane(
    _In_reads_(sourcePointCount) const PointType *pPointSource,
    XUINT32 sourcePointCount,
    _Out_writes_(destBufferSize) PointType *pPointDest,
    _Inout_ XUINT32 *pDestPointCount,
    XUINT32 destBufferSize,
    _Out_ bool *pWasClipped
    );

class CProjection : public CMultiParentShareableDependencyObject
{
public:
    DECLARE_CREATE(CProjection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CProjection>::Index;
    }

    virtual bool Is2DAligned() { return true; }

    void GetTransformerCommon(
        const XSIZEF& elementSize,
        _Outptr_ CPerspectiveTransformer **ppTransformer);

    _Check_return_ HRESULT GetContentBounds(
        _In_ const XSIZEF &elementSize,
        _In_ const XRECTF_RB &quadPadding,
        _Out_ XRECTF_RB* pContentBounds,
        _Out_ bool *pAreAllWValuesPositive
        );

    CMILMatrix4x4 GetOverallProjectionMatrix(_In_ const XSIZEF &elementSize);

    void SetDCompResourceDirty() override;

    static void NWSetRenderDirty(
        _In_ CDependencyObject* pTarget,
        DirtyFlags flags
        );

    void ReleaseDCompResources() override;

    virtual void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        );

    virtual void ClearWUCExpression();

    WUComp::IExpressionAnimation* GetWinRTExpression()
    {
        return m_spWinRTProjection.Get();
    }

    void EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context) override;

    void SetCached3DTransformDirty() { m_is3DTransformDirty = true; }

    virtual CMILMatrix4x4 GetProjectionMatrix() const;

    CMILMatrix4x4 GetOverallMatrix() const { return m_overallMatrix; }

protected:
    CProjection(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {
    }

    ~CProjection() override;

    // 3D math helpers
    virtual void Update3DTransform(_In_ XFLOAT rWidth, _In_ XFLOAT rHeight)
    {
        UNREFERENCED_PARAMETER(rWidth);
        UNREFERENCED_PARAMETER(rHeight);
    }

private:
    _Check_return_ HRESULT BuildProjectionCommon(
        _In_ const XSIZEF &elementSize,
        _In_ const XRECTF_RB &quadPadding,
        _In_ const XRECTF_RB &textureBounds,
        _Inout_opt_ XPOINTF4 *pPtsOut,
        _Out_ XUINT32* pointCount,
        _Out_opt_ bool *pAreAllWValuesPositive
        );

    void Create3DQuad(
        _In_ const XSIZEF &elementSize,
        _In_ const XRECTF_RB &quadPadding,
        _In_ const XRECTF_RB &textureBounds,
        _Out_writes_(4) SDPoint4UV *pQuad,
        _Out_opt_ CMILMatrix4x4 *pProjectionPipeline
        );

    _Check_return_ HRESULT Clip3DQuad(
        _In_reads_(4) SDPoint4UV *pQuad,
        _Deref_out_range_(0, c_nMaxClippedVertexCount) XUINT32 *puClippedPolygonVerticesCount,
        _Out_writes_(c_nMaxClippedVertexCount) SDPoint4UV *pClippedPolygon
        );

    void ProjectToPlane(
        _In_reads_(uiClippedPolygonVertices) SDPoint4UV *pClippedPolygon,
        __range(3, c_nMaxClippedVertexCount) XUINT32 uiClippedPolygonVertices,
        _Out_writes_(uiClippedPolygonVertices) XPOINTF *pScreenSpacePoints,
        _Inout_opt_ XPOINTF4 *pPtsOut,
        _Out_opt_ bool *pAreAllWValuesPositive,
        _Out_ XUINT32* pointCount
        );

protected:
    // The 3D transform applied by the projection. PlaneProjection exposes this via PlaneProjection.ProjectionMatrix
    CMILMatrix4x4 m_mat3DTransform { true };

    // The overall 3D matrix applied by the projection. Includes the 3D transform and a perspective transform.
    CMILMatrix4x4 m_overallMatrix { true };

    // The last element size used to calculate m_overallMatrix. In PlaneProjection, the element size determines where
    // the camera is aimed, and affects the overall matrix. This affects m_overallMatrix dirtiness.
    XSIZEF m_previousElementSize {};

    bool m_is3DTransformDirty = false;
    bool m_isWinRTProjectionDirty = true;

    // TODO_WinRT: We should store the transform a single (IUnknown) pointer
    //             since only or the other is ever used with a given transform.
    Microsoft::WRL::ComPtr<WUComp::IExpressionAnimation> m_spWinRTProjection;
};
