// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Geometry.h>

class TransformAndClipStack;
class HWClip;

class CRectangleGeometry final : public CGeometry
{
private:
    CRectangleGeometry(_In_ CCoreServices *pCore)
        : CGeometry(pCore)
        , m_rc()
        , m_eRadiusX(0.0f)
        , m_eRadiusY(0.0f)
    {
    }

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CRectangleGeometry>::Index;
    }

    // CGeometry overrides
    bool CanBeAccelerated() override;

    static void ApplyClip(
        _In_ CRectangleGeometry *pClipGeometry,
        _In_opt_ const CMILMatrix *pTransformToClipRectSpace,
        _Inout_ XRECTF *pClipRect
        );

    static _Check_return_ HRESULT ApplyClip(
        _In_ CRectangleGeometry *pClipGeometry,
        _Inout_ TransformAndClipStack *pTransformsAndClips
        );

    static _Check_return_ HRESULT ApplyClip(
        _In_ CRectangleGeometry *pClipGeometry,
        _Inout_ HWClip *pClip
        );

    void ApplyClip(
        _Inout_ XRECTF *pRect
        );

    void GetRect(_Out_ XRECTF *pRect);

    WUComp::ICompositionGeometry* GetCompositionGeometry(_In_ VisualContentRenderer* renderer) override;

private:
    static _Check_return_ HRESULT InitializeRectangleClip(
        _In_ CRectangleGeometry *pClipGeometry,
        _Inout_ HWClip *pLocalClip
        );

    WUComp::ICompositionGeometry* GetSimpleRectangleCompositionGeometry(
        _In_ VisualContentRenderer* renderer,
        const wfn::Vector2 &size,
        const wfn::Vector2 &offset);
    WUComp::ICompositionGeometry* GetRoundedRectangleCompositionGeometry(
        _In_ VisualContentRenderer* renderer,
        const wfn::Vector2 &size,
        const wfn::Vector2 &offset);

public:
    // CRectangle fields
    XRECTF  m_rc;
    XFLOAT  m_eRadiusX;
    XFLOAT  m_eRadiusY;

//-----------------------------------------------------------------------------
// Printing Methods
//-----------------------------------------------------------------------------
protected:
    _Check_return_ HRESULT GetPrintGeometryVirtual(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const D2DRenderParams &printParams,
        IPALAcceleratedGeometry** ppGeometry
        ) override;

    //-----------------------------------------------------------------------------
    //
    //  Bounds and Hit Testing
    //
    //-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT GetBounds(
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT GetWidenedBounds(
        _In_ const CPlainPen& pen,
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT HitTestFill(
        _In_ const XPOINTF& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        ) override;

    _Check_return_ HRESULT HitTestFill(
        _In_ const HitTestPolygon& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        ) override;

private:
    template <typename HitType>
    _Check_return_ HRESULT HitTestFillImpl(
        _In_ const HitType& target,
        _In_opt_ const CMILMatrix* pTransform,
        _Out_ bool* pHit
        );

    static bool CanDoFastWidening(
        _In_ const CPlainPen& pen,
        _In_ const XRECTF& bounds
        );

protected:
    _Check_return_ HRESULT VisitSinkInternal(
        _In_ IPALGeometrySink* pSink
        ) override;
};
