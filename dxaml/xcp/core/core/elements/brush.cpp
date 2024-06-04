// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for brush object
//
//------------------------------------------------------------------------
CBrush::~CBrush()
{
    ReleaseInterface(m_pPALBrush);

    if (GetContext()->NWGetWindowRenderTarget() != nullptr && GetDCompTreeHost() != nullptr)
    {
        DCompTreeHost* dcompTreeHost = GetDCompTreeHost();
        auto& lightTargetIdMap = dcompTreeHost->GetXamlLightTargetIdMap();
        lightTargetIdMap.RemoveTarget(this);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of the base brush class.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CBrush::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    // If we're passed a string create forward the call to the solid color brush
    // creator.

    if (pCreate->m_value.GetType() == valueString ||
        pCreate->m_value.GetType() == valueColor)
    {
        return CSolidColorBrush::Create(ppObject, pCreate);
    }

    // If we can't make an object, return an error

    return E_FAIL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor that copies all CBrush members needed for a Clone().
//
//------------------------------------------------------------------------
CBrush::CBrush(_In_ const CBrush& original, _Out_ HRESULT& hr)
    : CMultiParentShareableDependencyObject(original, hr)
    , m_pPALBrush(NULL)
{
    // Stop if base constructor had problems.
    VERIFYHR(hr);

    // Proceed to remainder of work.
    m_eOpacity = original.GetOpacity();

    auto originalTransform = original.GetTransform();
    if (originalTransform != nullptr)
    {
        xref_ptr<CTransform> transformClone;
        VERIFYHR(originalTransform->Clone(reinterpret_cast<CNoParentShareableDependencyObject**>(transformClone.ReleaseAndGetAddressOf())));
        VERIFYHR(SetValueByKnownIndex(KnownPropertyIndex::Brush_Transform, transformClone.get()));
    }

    auto originalRelativeTransform = original.GetRelativeTransform();
    if (originalRelativeTransform != nullptr)
    {
        xref_ptr<CTransform> relativeTransformClone;
        VERIFYHR(originalRelativeTransform->Clone(reinterpret_cast<CNoParentShareableDependencyObject**>(relativeTransformClone.ReleaseAndGetAddressOf())));
        VERIFYHR(SetValueByKnownIndex(KnownPropertyIndex::Brush_RelativeTransform, relativeTransformClone.get()));
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the device independent D2D resources for this brush,
//      including the brush transform and clip geometry. Called during
//      prerender. Note that the actual D2D brush does not need to exist
//      for the clip geometry to be created.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::D2DEnsureDeviceIndependentResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ const XRECTF_RB *pBrushBounds,
    _Inout_ AcceleratedBrushParams *pPALBrushParams
    )
{
    UNREFERENCED_PARAMETER(cp);
    UNREFERENCED_PARAMETER(pMyAccumulatedTransform);
    UNREFERENCED_PARAMETER(pBrushBounds);
    UNREFERENCED_PARAMETER(pPALBrushParams);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden to create and update the D2D/postscript brush. Called
//      during render.
//
//      The base implementation updates the accelerated brush's opacity.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::UpdateAcceleratedBrush(
    _In_ const D2DRenderParams& renderParams
    )
{
    // The derived class should override this method to create the accelerated brush before
    // calling the base implementation.
    ASSERT(m_pPALBrush != nullptr);

    IFC_RETURN(m_pPALBrush->SetOpacity(GetOpacity()));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the D2D brush corresponding to this brush.
//
//------------------------------------------------------------------------
_Ret_maybenull_ IPALAcceleratedBrush*
CBrush::GetAcceleratedBrush()
{
    return m_pPALBrush;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the print brush corresponding to this brush.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::GetPrintBrush(
    _In_ const D2DRenderParams &printParams,
    _Outptr_ IPALAcceleratedBrush **ppBrush
    )
{
    *ppBrush = nullptr;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      We have to override GetValue to provide an object as a default
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue)
{
    IFC_RETURN(CNoParentShareableDependencyObject::GetValue(pdp, pValue));
    if (pdp->GetIndex() == KnownPropertyIndex::Brush_Transform || pdp->GetIndex() == KnownPropertyIndex::Brush_RelativeTransform)
    {
        auto transform = pValue->AsObject();
        if (transform == nullptr)
        {
            // If we don't have a value, we'll want to return an Identity Transform.
            CREATEPARAMETERS cp(GetContext());
            IFC_RETURN(CMatrixTransform::Create(reinterpret_cast<CDependencyObject**>(&transform), &cp));
            pValue->SetObjectNoRef(transform);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Hit tests a point against the portion of the geometry that's
//      actually filled by this brush. The unfilled portions of the
//      geometry can't be hit.
//
//      Note: pGeometryRenderBounds are the bounds defined by the
//      geometry, not the actual portion of that geometry which is filled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const XPOINTF& target,
    _Out_ bool* pIsHit
    )
{
    // By default a brush fills the entire geometry, so it can all be hit.
    *pIsHit = TRUE;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Hit tests a polygon against the portion of the geometry that's
//      actually filled by this brush. The unfilled portions of the
//      geometry can't be hit.
//
//      Note: pGeometryRenderBounds are the bounds defined by the
//      geometry, not the actual portion of that geometry which is filled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const HitTestPolygon& target,
    _Out_ bool* pIsHit
    )
{
    // By default a brush fills the entire geometry, so it can all be hit.
    *pIsHit = TRUE;
    return S_OK;
}

DCompTreeHost* CBrush::GetDCompTreeHost()
{
    return GetContext()->NWGetWindowRenderTarget()->GetDCompTreeHost();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
AcceleratedBrushParams::AcceleratedBrushParams()
    : m_transform(CMILMatrix(TRUE))
    , m_pClipGeometry(nullptr)
{
    SetInfiniteClip(&m_clipRect);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
AcceleratedBrushParams::~AcceleratedBrushParams()
{
    ReleaseInterface(m_pClipGeometry);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the brush transform, clip, and clip layer.
//
//------------------------------------------------------------------------
void
AcceleratedBrushParams::Reset()
{
    m_transform = CMILMatrix(TRUE);
    ResetBrushClip();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resets the brush clip and brush clip layer.
//
//------------------------------------------------------------------------
void
AcceleratedBrushParams::ResetBrushClip()
{
    SetInfiniteClip(&m_clipRect);

    ReleaseInterface(m_pClipGeometry);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the brush transform and brush clip geometry. Adjusts the
//      clip by the transform as needed. Does not create a layer for the
//      clip geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
AcceleratedBrushParams::SetBrushTransformAndClip(
    _In_ IPALAcceleratedGraphicsFactory *pD2DFactory,
    _In_ CMILMatrix *pBrushTransform,
    _In_opt_ XRECTF *pBrushClipRect
    )
{
    HRESULT hr = S_OK;
    bool recreateClipGeometry = false;
    IPALAcceleratedGeometry *pUntransformedClip = nullptr;
    IPALAcceleratedGeometry *pClip = nullptr;

    if (m_transform != *pBrushTransform)
    {
        m_transform = *pBrushTransform;
        recreateClipGeometry = TRUE;
    }

    if (pBrushClipRect == nullptr)
    {
        ResetBrushClip();
        recreateClipGeometry = FALSE;
    }
    else if (m_clipRect.X != pBrushClipRect->X
        || m_clipRect.Y != pBrushClipRect->Y
        || m_clipRect.Width != pBrushClipRect->Width
        || m_clipRect.Height != pBrushClipRect->Height
        )
    {
        m_clipRect = *pBrushClipRect;
        recreateClipGeometry = TRUE;
    }

    if (recreateClipGeometry)
    {
        //
        // We have a brush clip, but it may not be the same as m_clipRect passed through
        // m_transform (e.g. a XRECTF that has been rotated will no longer be a XRECTF).
        // So we have to create a transformed geometry for it.
        //
        IFC(pD2DFactory->CreateRectangleGeometry(
            m_clipRect,
            &pUntransformedClip
            ));

        if (m_transform.IsIdentity())
        {
            SetInterface(pClip, pUntransformedClip);
        }
        else
        {
            IFC(pD2DFactory->CreateTransformedGeometry(
                pUntransformedClip,
                &m_transform,
                &pClip
                ));
        }

        ReplaceInterface(m_pClipGeometry, pClip);
    }

Cleanup:
    ReleaseInterface(pUntransformedClip);
    ReleaseInterface(pClip);

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes the brush clip. Uses PushAxisAlignedClip when possible.
//      Creates a layer for the brush geometry as needed. This step is
//      separate from D2DSetBrushTransformAndClip because it uses a
//      different factory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
AcceleratedBrushParams::PushBrushClip(
    bool allowPushAxisAlignedClip,
    _In_ const CMILMatrix *pWorldTransform,
    _In_ IPALAcceleratedRenderTarget *pD2DRenderTarget,
    _In_opt_ const XRECTF_RB *pContentBounds,
    _Inout_ bool *pPushedBrushClipLayer,
    _Inout_ bool *pPushedAxisAlignedBrushClip
    )
{
    if (m_pClipGeometry != nullptr)
    {
        bool isClipEmpty = false;

        //
        // No need to check m_pClipGeometry because it's always a transformed
        // rectangle. Just checking the brush transform is enough.
        //
        bool pushAxisAlignedClip = allowPushAxisAlignedClip &&
            pWorldTransform->IsScaleOrTranslationOnly() &&
            m_transform.IsScaleOrTranslationOnly();

        // Clip to layout size for stretch None or UniformToFill
        IFC_RETURN(CUIElement::D2DSetUpClipHelper(
            pD2DRenderTarget,
            pushAxisAlignedClip,
            m_pClipGeometry,
            pContentBounds,
            pPushedBrushClipLayer,
            pPushedAxisAlignedBrushClip,
            &isClipEmpty
            ));
    }

    return S_OK;
}

