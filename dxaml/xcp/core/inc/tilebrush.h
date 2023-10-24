// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class HWTextureManager;
class SurfaceCache;
class CTiledSurface;
class HWTexture;

//------------------------------------------------------------------------
//
//  Class:  CTileBrush
//
//  Synopsis:
//      A brush that supports tiling of its content.
//
//------------------------------------------------------------------------

class CTileBrush : public CBrush
{
protected:
    CTileBrush(_In_ CCoreServices *pCore)
        : CBrush(pCore)
    {}

   ~CTileBrush() override;

public:
// Creation method
    DECLARE_CREATE(CTileBrush);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTileBrush>::Index;
    }

    _Check_return_ HRESULT InitInstance() override;

    virtual bool IsOpaque()
    {
        ASSERT( FALSE ); // This method should never be called
        return true;
    }

    bool HasClipRect() final;

    _Check_return_ HRESULT HitTestBrushClipInLocalSpace(
        _In_ const XRECTF *pGeometryRenderBounds,
        _In_ const XPOINTF& target,
        _Out_ bool* pIsHit
        ) final;

    _Check_return_ HRESULT HitTestBrushClipInLocalSpace(
        _In_ const XRECTF *pGeometryRenderBounds,
        _In_ const HitTestPolygon& target,
        _Out_ bool* pIsHit
        ) final;

private:
    template <typename HitType>
    _Check_return_ HRESULT HitTestBrushClipInLocalSpaceImpl(
        _In_ const XRECTF *pGeometryRenderBounds,
        _In_ const HitType& target,
        _Out_ bool* pIsHit
        );

public:
// CTileBrush methods
    _Check_return_ HRESULT ComputeDeviceToSource(
        _In_ const CMILMatrix  *pWorldTransform,
        _In_ const XRECTF      *pRenderBounds,
        _Out_      CMILMatrix  *pDeviceToSource
        ) final;

    static _Check_return_ HRESULT ComputeStretchMatrix(
        _In_ const XRECTF *pNaturalBounds,
        _In_ const XRECTF *pRenderBounds,
        DirectUI::AlignmentX alignmentX,
        DirectUI::AlignmentY alignmentY,
        DirectUI::Stretch stretch,
        _Out_ CMILMatrix *pStretchMatrix
        );

    static _Check_return_ HRESULT ComputeStretchAndAlignment(
        _In_ const XRECTF *pNaturalBounds,
        _In_ const XRECTF *pRenderBounds,
        DirectUI::AlignmentX alignmentX,
        DirectUI::AlignmentY alignmentY,
        DirectUI::Stretch stretch,
        _Out_ XFLOAT *pOffsetX,
        _Out_ XFLOAT *pOffsetY,
        _Out_ XFLOAT *pScaleX,
        _Out_ XFLOAT *pScaleY
        );

    virtual _Check_return_ HRESULT AdjustDecodeRectForStretch(
        _Inout_ XRECTF *pDecodeBounds
        );

    virtual IPALSurface *GetSurface()
    {
        ASSERT( FALSE ); // This method should never be called
        return NULL;
    }

    virtual _Check_return_ HRESULT Lock(
        _Out_ void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight)
    {
        (ppAddress); // Ignore the parameter.
        (pnStride);  // Ignore the parameter.
        (puWidth);   // Ignore the parameter.
        (puHeight);  // Ignore the parameter.
        ASSERT( FALSE ); // This method should never be called
        return E_FAIL;
    }

    virtual XUINT32 UnLock()
    {
        ASSERT( FALSE ); // This method should never be called
        return 0;
    }

    virtual _Check_return_ HRESULT EnsureAndUpdateHardwareResources(
        _In_ HWTextureManager *pTextureManager,
        _In_ CWindowRenderTarget *pRenderTarget,
        _In_ SurfaceCache *pSurfaceCache
        )
    {
        ASSERT(FALSE); // This method should be overridden by subclasses and never called
        RRETURN(E_NOTIMPL);
    }

    virtual _Ret_maybenull_ HWTexture* GetHardwareSurface()
    {
        ASSERT(FALSE);
        return NULL;
    }

    virtual _Check_return_ HRESULT ReloadSoftwareSurfaceIfReleased()
    {
        ASSERT(FALSE);
        RRETURN(E_NOTIMPL);
    }

    virtual _Ret_maybenull_ IPALSurface* GetSoftwareSurface()
    {
        ASSERT(FALSE);
        return NULL;
    }

public:
    // CTileBrush fields

    DirectUI::AlignmentX      m_AlignmentX  = DirectUI::AlignmentX::Center;
    DirectUI::AlignmentY      m_AlignmentY  = DirectUI::AlignmentY::Center;
    DirectUI::Stretch         m_Stretch     = DirectUI::Stretch::Fill;
    CMILMatrix                m_matTileMode = CMILMatrix(TRUE);

//-----------------------------------------------------------------------------
// D2D Methods/Fields
//-----------------------------------------------------------------------------
public:
    _Check_return_ HRESULT D2DEnsureDeviceIndependentResources(
        _In_ const D2DPrecomputeParams& cp,
        _In_ const CMILMatrix *pMyAccumulatedTransform,
        _In_ const XRECTF_RB *pBrushBounds,
        _Inout_ AcceleratedBrushParams *pPALBrushParams
        ) final;

};
