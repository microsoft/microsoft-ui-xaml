// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor.  Cleans up parameterized code blocks.
//
//------------------------------------------------------------------------
CTileBrush::~CTileBrush()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called at creation time, allows us to create the critical section
//  that serializes access to the generated code blocks.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTileBrush::InitInstance()
{
    return S_OK;
}

bool CTileBrush::HasClipRect()
{
    return  m_Stretch != DirectUI::Stretch::Fill
        || GetTransform() != nullptr
        || GetRelativeTransform() != nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the effective brush transform taking into account stretch
//      and alignment.
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CTileBrush::ComputeStretchMatrix(
    _In_ const XRECTF *pNaturalBounds,
    _In_ const XRECTF *pRenderBounds,
    DirectUI::AlignmentX alignmentX,
    DirectUI::AlignmentY alignmentY,
    DirectUI::Stretch stretch,
    _Out_ CMILMatrix *pStretchMatrix
    )
{
    XFLOAT rScaleX = 1.0f;
    XFLOAT rScaleY = 1.0f;
    XFLOAT rDeltaX = 0.0f;
    XFLOAT rDeltaY = 0.0f;

    // Reset internal matrix cache
    pStretchMatrix->SetToIdentity();

    IFC_NOTRACE_RETURN(CTileBrush::ComputeStretchAndAlignment(
        pNaturalBounds,
        pRenderBounds,
        alignmentX,
        alignmentY,
        stretch,
        &rDeltaX,
        &rDeltaY,
        &rScaleX,
        &rScaleY));

    // Update adjust matrix
    pStretchMatrix->SetM11(rScaleX);
    pStretchMatrix->SetM22(rScaleY);
    pStretchMatrix->SetDx(rDeltaX);
    pStretchMatrix->SetDy(rDeltaY);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates the factors needed to transform the natural bounds
//      to the render bounds.  The offset depends on alignment and the
//      scale depends on stretch.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CTileBrush::ComputeStretchAndAlignment(
    _In_ const XRECTF *pNaturalBounds,
    _In_ const XRECTF *pRenderBounds,
    DirectUI::AlignmentX alignmentX,
    DirectUI::AlignmentY alignmentY,
    DirectUI::Stretch stretch,
    _Out_ XFLOAT *pOffsetX,
    _Out_ XFLOAT *pOffsetY,
    _Out_ XFLOAT *pScaleX,
    _Out_ XFLOAT *pScaleY
    )
{
    XFLOAT rScaleX = 1.0f;
    XFLOAT rScaleY = 1.0f;
    XFLOAT rDeltaX = 0.0f;
    XFLOAT rDeltaY = 0.0f;
    XFLOAT rAspectRatioXY = 1.0f;

    // Should we ASSERT this?  Currently we are regularly calling before the image bits
    // are downloaded, at which point the natural bounds are 0
    IFCCHECK_NOTRACE_RETURN((pNaturalBounds->Height > 0) && (pNaturalBounds->Width > 0));

    // Compute based on stretch mode
    switch (stretch)
    {
        // No stretching, just use the natural bounds
        case DirectUI::Stretch::None:
            rDeltaX = pRenderBounds->Width - pNaturalBounds->Width;
            rDeltaY = pRenderBounds->Height - pNaturalBounds->Height;
            break;

        // Stretch both ways, all of the image is visible.
        case DirectUI::Stretch::Fill:
            rScaleX = pRenderBounds->Width / pNaturalBounds->Width;
            rScaleY = pRenderBounds->Height / pNaturalBounds->Height;
            rDeltaX = 0.0f;
            rDeltaY = 0.0f;
            break;

        // Stretch to fill the smaller dimension and keep aspect ratio:
        //   all of the image is visible, parts of the brush may be transparent.
        case DirectUI::Stretch::Uniform:
            rScaleX = pRenderBounds->Width / pNaturalBounds->Width;
            rScaleY = pRenderBounds->Height / pNaturalBounds->Height;
            rAspectRatioXY = rScaleX / rScaleY;
            if (rAspectRatioXY > 1.0)
            {
                rScaleX = rScaleY;
                rDeltaY = 0.0f;
                rDeltaX = pRenderBounds->Width - (pNaturalBounds->Width * rScaleX);
            }
            else
            {
                rScaleY = rScaleX;
                rDeltaX = 0.0f;
                rDeltaY = pRenderBounds->Height - (pNaturalBounds->Height * rScaleY);
            }
            break;

        // Stretch to fill the larger dimension and keep aspect ratio:
        //   portions of the image may be clipped off, all of the brush is filled.
        case DirectUI::Stretch::UniformToFill:
            rScaleX = pRenderBounds->Width / pNaturalBounds->Width;
            rScaleY = pRenderBounds->Height / pNaturalBounds->Height;
            rAspectRatioXY = rScaleX / rScaleY;
            if (rAspectRatioXY > 1.0)
            {
                rScaleY = rScaleX;
                rDeltaX = 0.0f;
                rDeltaY = pRenderBounds->Height - (pNaturalBounds->Height * rScaleY);
            }
            else
            {
                rScaleX = rScaleY;
                rDeltaY = 0.0f;
                rDeltaX = pRenderBounds->Width - (pNaturalBounds->Width * rScaleX);
            }
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    // Compute based on horizontal Alignment mode
    switch (alignmentX)
    {
        case DirectUI::AlignmentX::Left:
            // Left alignment is always zero.
            rDeltaX = 0.0f;
            break;

        case DirectUI::AlignmentX::Center:
            // Right alignment is computed in the stretch calculation,
            //   so center is half of that.
            rDeltaX *= 0.5f;
            break;

        case DirectUI::AlignmentX::Right:
            // Right alignment is computed in the stretch calculation,
            //   so this is a no-op
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    // Compute based on vertical Alignment mode
    switch (alignmentY)
    {
        case DirectUI::AlignmentY::Top:
            // Top alignment is always zero.
            rDeltaY = 0.0f;
            break;

        case DirectUI::AlignmentY::Center:
            // Bottom alignment is computed in the stretch calculation,
            //   so center is half of that.
            rDeltaY *= 0.5f;
            break;

        case DirectUI::AlignmentY::Bottom:
            // Bottom alignment is computed in the stretch calculation,
            //   so this is a no-op
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    *pOffsetX = rDeltaX;
    *pOffsetY = rDeltaY;
    *pScaleX = rScaleX;
    *pScaleY = rScaleY;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adjusts the given target render rectangle to account for the
//      tile brush stretch and alignment so that the render rectangle
//      has the appropriate amount pixels for the stretch.
//
//      This is used in the decode to render size case to determine
//      the size to decode an image.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTileBrush::AdjustDecodeRectForStretch(
    _Inout_ XRECTF *pDecodeBounds
    )
{
    XRECTF naturalBounds;

    ASSERT(pDecodeBounds != NULL);

    // If the stretch mode is Uniform or UniformToFill, and the input
    // bounds is 0x0, we were falling back to decoding at full natural size.
    // This causes DecodeToRenderSize to essentially disable itself unnecessarily.
    // The fix is to remove that code in those two cases and avoid calling
    // AdjustDecodeRectForStretch in that case.
    ASSERT(pDecodeBounds->Width != 0.0f);
    ASSERT(pDecodeBounds->Height != 0.0f);

    GetNaturalBounds(&naturalBounds);

    // NOTE: Order of operations below is important to reduce rounding errors
    if ((naturalBounds.Width != 0) &&
        (naturalBounds.Height != 0))
    {
        switch (m_Stretch)
        {
        case DirectUI::Stretch::None:
            // No stretch, need to use all the image pixels
            *pDecodeBounds = naturalBounds;
            break;

        case DirectUI::Stretch::Fill:
            // Do nothing, use the current render bounds
            // Except clamp to natural Bounds
            break;

        case DirectUI::Stretch::Uniform:
            {
                if (((naturalBounds.Width * pDecodeBounds->Height) > (pDecodeBounds->Width * naturalBounds.Height)) ||
                    (pDecodeBounds->Height == 0))
                {
                    pDecodeBounds->Height = static_cast<XFLOAT>(XcpCeiling((pDecodeBounds->Width * naturalBounds.Height) / naturalBounds.Width));
                }
                else
                {
                    pDecodeBounds->Width = static_cast<XFLOAT>(XcpCeiling((pDecodeBounds->Height * naturalBounds.Width) / naturalBounds.Height));
                }
            }
            break;

        case DirectUI::Stretch::UniformToFill:
            {
                if (((naturalBounds.Width * pDecodeBounds->Height) > (pDecodeBounds->Width * naturalBounds.Height)) ||
                    (pDecodeBounds->Width == 0))
                {
                    pDecodeBounds->Width = static_cast<XFLOAT>(XcpCeiling((pDecodeBounds->Height * naturalBounds.Width) / naturalBounds.Height));
                }
                else
                {
                    pDecodeBounds->Height = static_cast<XFLOAT>(XcpCeiling((pDecodeBounds->Width * naturalBounds.Height) / naturalBounds.Width));
                }
            }
            break;

        default:
            ASSERT(FALSE);
            break;
        }

        // Clamp to natural bounds
        if (pDecodeBounds->Width > naturalBounds.Width)
        {
            pDecodeBounds->Width = naturalBounds.Width;

            pDecodeBounds->Height = naturalBounds.Height;
        }

        if (pDecodeBounds->Height > naturalBounds.Height)
        {
            pDecodeBounds->Height = naturalBounds.Height;

            pDecodeBounds->Width = naturalBounds.Width;
        }
    }

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Computes the effective brush transform taking into account stretch
//      and alignment, and prior transformation. Uses the local cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTileBrush::ComputeDeviceToSource(
    _In_ const CMILMatrix *pWorldTransform,
    _In_ const XRECTF *pRenderBounds,
    _Out_ CMILMatrix *pDeviceToSource
    )
{
    XRECTF naturalBounds;

    ASSERT(pWorldTransform);
    ASSERT(pRenderBounds);
    ASSERT(pDeviceToSource);

    // On error we just send back identity
    CMILMatrix aggregateTransform(FALSE);

    // Get the natural dimesions of this brush
    GetNaturalBounds(&naturalBounds);

    // Update cached bounds ...
    IFC_NOTRACE_RETURN(ComputeStretchMatrix(
        &naturalBounds,
        pRenderBounds,
        m_AlignmentX,
        m_AlignmentY,
        m_Stretch,
        &m_matTileMode
        ));

    aggregateTransform = m_matTileMode;

    // Add relative transform - we need to have valid viewbox here.
    auto relativeTransform = GetRelativeTransform();
    if (relativeTransform != nullptr)
    {
        // Get relative transform
        CMILMatrix matBrushRelative(FALSE);
        relativeTransform->GetTransform(&matBrushRelative);

        // Adjust scale to work in relative space:
        //
        //   Given a matrix M in relative space
        //   [ a  b  0 ]
        //   [ c  d  0 ]
        //   [ dx dy 1 ]
        //
        //   We need to scale our answer by the matrix S in our
        //   existing coordinate space
        //   [ 1/w  0    0 ]
        //   [ 0    1/h  0 ]
        //   [ 0    0    1 ]
        //
        //   and then scale the composite back to our space by using S-1:
        //   [ w    0    0 ]
        //   [ 0    h    0 ]
        //   [ 0    0    1 ]
        //
        //   So [S]x[M]x[S-1] =
        //   [ a      b*h/w   0 ]
        //   [ c*w/h  d       0 ]
        //   [ dx*w   dy*h    1 ]
        //
        //   ... adjust that here, save some multiply cycles.
        matBrushRelative._31 *= pRenderBounds->Width;
        matBrushRelative._32 *= pRenderBounds->Height;
        matBrushRelative._21 *= (pRenderBounds->Width / pRenderBounds->Height);
        matBrushRelative._12 *= (pRenderBounds->Height / pRenderBounds->Width);

        aggregateTransform.Append(matBrushRelative);
    }

    // Add brush offsets (for pens)
    if (pRenderBounds->X != 0 || pRenderBounds->Y != 0)
    {
        CMILMatrix matBoundsOffset(TRUE);
        matBoundsOffset.SetDx(pRenderBounds->X);
        matBoundsOffset.SetDy(pRenderBounds->Y);
        aggregateTransform.Append(matBoundsOffset);
    }

    // Add brush transform
    auto transform = GetTransform();
    if (transform != nullptr)
    {
        CMILMatrix matBrushTransform(FALSE);
        transform->GetTransform(&matBrushTransform);
        aggregateTransform.Append(matBrushTransform);
    }

    // Compound with current transform
    aggregateTransform.Append(*pWorldTransform);

    *pDeviceToSource = aggregateTransform;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the device independent D2D resources for the brush.
//      Called during prerender.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTileBrush::D2DEnsureDeviceIndependentResources(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform,
    _In_ const XRECTF_RB *pBrushBounds,
    _Inout_ AcceleratedBrushParams *pPALBrushParams
    )
{
    XRECTF rcBrushClip;

    IFC_RETURN(CBrush::D2DEnsureDeviceIndependentResources(
        cp,
        pMyAccumulatedTransform,
        pBrushBounds,
        pPALBrushParams
        ));

    GetNaturalBounds(&rcBrushClip);

    if (rcBrushClip.Width > 0 && rcBrushClip.Height > 0)
    {
        CMILMatrix identityTransform(TRUE);

        CMILMatrix brushTransform;
        XRECTF brushBounds = ToXRectF(*pBrushBounds);

        // Calculate brush transform in local space (hence the identityTransform)
        // The accelerated brush is in local space, unlike the software SL brush which is in surface space
        IFC_RETURN(ComputeDeviceToSource(
            &identityTransform,
            &brushBounds,
            &brushTransform
            ));

        IFC_RETURN(pPALBrushParams->SetBrushTransformAndClip(
            cp.GetFactory(),
            &brushTransform,
            HasClipRect() ? &rcBrushClip : NULL
            ));
    }
    else
    {
        // If there is no image or a bad image
        pPALBrushParams->Reset();
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
CTileBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const XPOINTF& target,
    _Out_ bool* pIsHit
    )
{
    RRETURN(HitTestBrushClipInLocalSpaceImpl(pGeometryRenderBounds, target, pIsHit));
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
CTileBrush::HitTestBrushClipInLocalSpace(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const HitTestPolygon& target,
    _Out_ bool* pIsHit
    )
{
    RRETURN(HitTestBrushClipInLocalSpaceImpl(pGeometryRenderBounds, target, pIsHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Hit tests a point/polygon against the portion of the geometry that's
//      actually filled by this brush. The unfilled portions of the
//      geometry can't be hit.
//
//      Note: pGeometryRenderBounds are the bounds defined by the
//      geometry, not the actual portion of that geometry which is filled.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CTileBrush::HitTestBrushClipInLocalSpaceImpl(
    _In_ const XRECTF *pGeometryRenderBounds,
    _In_ const HitType& target,
    _Out_ bool* pIsHit
    )
{
    HRESULT hr = S_OK;
    bool isHit = false;
    CRectangleGeometry *pRectangleGeometry = NULL;

    if (HasClipRect())
    {
        XRECTF brushNaturalBounds;
        GetNaturalBounds(&brushNaturalBounds);

        if (brushNaturalBounds.Width > 0 && brushNaturalBounds.Height > 0)
        {
            CMILMatrix identityTransform(TRUE);
            CMILMatrix brushTransform;

            // Calculate brush transform in local space (hence the identityTransform)
            IFC(ComputeDeviceToSource(
                &identityTransform,
                pGeometryRenderBounds,
                &brushTransform
                ));

            if (brushTransform.Invert())
            {
                HitType transformedTarget;
                ApplyTransformToHitType(&brushTransform, &target, &transformedTarget);
                isHit = DoesRectIntersectHitType(brushNaturalBounds, transformedTarget);
            }
        }
    }
    else
    {
        isHit = TRUE;
    }

Cleanup:
    *pIsHit = isHit;
    ReleaseInterfaceNoNULL(pRectangleGeometry);
    RRETURN(hr);
}
