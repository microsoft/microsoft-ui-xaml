// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualContentRenderer.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to create an rectangular figure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CreateRect(
    _In_ CCoreServices *pCore,
    _In_ const XRECTF& rc,
    _Outptr_ CPathFigure **ppFigure
    )
{
    HRESULT hr;
    CPointCollection *pPoints = nullptr;
    CPolyLineSegment *pCurves = nullptr;
    CPathSegmentCollection *pSegments = nullptr;
    CPathFigure *pFigure = nullptr;
    CValue value;

    // Build a rectangular geometry parametrically
    XPOINTF aPoints[4];

    // First compute the X coordinates
    aPoints[0].x = aPoints[3].x = rc.X;
    aPoints[1].x = aPoints[2].x = rc.X + rc.Width;

    // Then the Y coordinates
    aPoints[0].y = aPoints[1].y = rc.Y;
    aPoints[2].y = aPoints[3].y = rc.Y + rc.Height;

    CREATEPARAMETERS cp(pCore);
    IFC(CPointCollection::Create(reinterpret_cast<CDependencyObject**>(&pPoints), &cp));
    IFC(pPoints->InitFromArray(3, &aPoints[1]));

    IFC(CPolyLineSegment::Create(reinterpret_cast<CDependencyObject**>(&pCurves), &cp));
    value.WrapObjectNoRef(pPoints);
    IFC(pCurves->SetValue(pCurves->GetPropertyByIndexInline(KnownPropertyIndex::PolyLineSegment_Points), value));

    IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject**>(&pSegments), &cp));
    value.WrapObjectNoRef(pCurves);
    IFC(pSegments->SetValue(pSegments->GetContentProperty(), value));

    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject**>(&pFigure), &cp));
    value.WrapObjectNoRef(pSegments);
    IFC(pFigure->SetValue(pFigure->GetContentProperty(), value));

    // Set its start point, mark it filled and closed.  Since these values aren't
    // sparsely stored we can directly assign them to the object.
    pFigure->m_ptStart = aPoints[0];
    pFigure->m_bClosed = true;

   *ppFigure = pFigure;
    pFigure = nullptr;

Cleanup:
    ReleaseInterface(pPoints);
    ReleaseInterface(pCurves);
    ReleaseInterface(pSegments);
    ReleaseInterfaceNoNULL(pFigure);

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to create an rectangular figure with rounded corners.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CreateRoundedRect(
    _In_ CCoreServices *pCore,
    _In_ const XRECTF& rc,
    _In_ const XFLOAT& inputRadiusX,
    _In_ const XFLOAT& inputRadiusY,
    _Outptr_ CPathFigure **ppFigure
    )
{
    HRESULT hr = S_OK;

    XFLOAT radiusX = inputRadiusX;
    XFLOAT radiusY = inputRadiusY;

    // Deal with negative or out of bound radii
    if (radiusX < 0)
    {
        radiusX = -radiusX;
    }
    if (radiusY < 0)
    {
        radiusY = -radiusY;
    }
    if (radiusX > (rc.Width / 2.0f))
    {
        radiusX = (rc.Width / 2.0f);
    }
    if (radiusY > (rc.Height / 2.0f))
    {
        radiusY = (rc.Height / 2.0f);
    }

    // Use (1 - ARC_AS_BEZIER) - because we measure from the edge of the rectangle
    // not the center of the arc.
    XFLOAT  eBezierX = (1.0f - ARC_AS_BEZIER) * radiusX;
    XFLOAT  eBezierY = (1.0f - ARC_AS_BEZIER) * radiusY;
    XFLOAT  eRight = rc.X + rc.Width;
    XFLOAT  eBottom = rc.Y + rc.Height;
    XPOINTF pPoints[17];

    // First set all the X coordinates
    pPoints[1].x  = pPoints[0].x  = pPoints[15].x = pPoints[14].x = rc.X;
    pPoints[2].x  = pPoints[13].x = rc.X  + eBezierX;
    pPoints[3].x  = pPoints[12].x = rc.X  + radiusX;
    pPoints[4].x  = pPoints[11].x = eRight  - radiusX;
    pPoints[5].x  = pPoints[10].x = eRight  - eBezierX;
    pPoints[6].x  = pPoints[7].x  = pPoints[8].x  = pPoints[9].x  = eRight;

    // Then all the Y coordinates
    pPoints[2].y  = pPoints[3].y  = pPoints[4].y  = pPoints[5].y  = rc.Y;
    pPoints[1].y  = pPoints[6].y  = rc.Y  + eBezierY;
    pPoints[0].y  = pPoints[7].y  = rc.Y  + radiusY;
    pPoints[15].y = pPoints[8].y  = eBottom - radiusY;
    pPoints[14].y = pPoints[9].y  = eBottom - eBezierY;
    pPoints[13].y = pPoints[12].y = pPoints[11].y = pPoints[10].y = eBottom;

    // Copy the last point from the first point
    pPoints[16] = pPoints[0];

    // Now create the array of segments. We'll need four line segments as well as
    // four cubic Bezier segments.
    CREATEPARAMETERS cp(pCore);
    CPathSegment *apSegments[8];
    CPathSegmentCollection *pSegments = NULL;
    CPathFigure *pFigure = NULL;
    CValue value;
    XUINT32 cSegment;

    memset(apSegments, 0, 8 * sizeof(CPathSegment *));

    for (cSegment = 0; cSegment < 8; cSegment += 2)
    {
        IFC(CBezierSegment::Create(reinterpret_cast<CDependencyObject**>(&apSegments[cSegment]), &cp));
        (static_cast<CBezierSegment*>(apSegments[cSegment]))->m_apt[0] = pPoints[cSegment * 2 + 1];
        (static_cast<CBezierSegment*>(apSegments[cSegment]))->m_apt[1] = pPoints[cSegment * 2 + 2];
        (static_cast<CBezierSegment*>(apSegments[cSegment]))->m_apt[2] = pPoints[cSegment * 2 + 3];

        IFC(CLineSegment::Create(reinterpret_cast<CDependencyObject**>(&apSegments[cSegment + 1]), &cp));
        (static_cast<CLineSegment*>(apSegments[cSegment + 1]))->m_pt = pPoints[cSegment * 2 + 4];
    }


    IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject**>(&pSegments), &cp));

    for (cSegment = 0; cSegment < 8; cSegment++)
    {
        value.WrapObjectNoRef(apSegments[cSegment]);
        IFC(pSegments->SetValue(pSegments->GetContentProperty(), value));
    }

    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject**>(&pFigure), &cp));

    value.WrapObjectNoRef(pSegments);
    IFC(pFigure->SetValue(pFigure->GetContentProperty(), value));

    // Set its start point, mark it filled and closed
    pFigure->m_ptStart = pPoints[0];
    pFigure->m_bClosed = true;

   *ppFigure = pFigure;
    pFigure = nullptr;

Cleanup:
    for (cSegment = 0; cSegment < 8; cSegment++)
    {
        ReleaseInterfaceNoNULL(apSegments[cSegment]);
    }
    ReleaseInterfaceNoNULL(pSegments);
    ReleaseInterfaceNoNULL(pFigure);

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an instance of a rectangular shape object
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangle::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
   *ppObject = (CDependencyObject *) new CRectangle(pCreate->m_pCore);
   return S_OK; //RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the geometry that will be used to render this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangle::UpdateRenderGeometry()
{
    HRESULT hr = S_OK;
    CRectangleGeometry *pNewGeometry = nullptr;
    CRectangleGeometry *pGeometryNoRef = nullptr;

    if (m_pRender == nullptr)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC(CRectangleGeometry::Create(
            reinterpret_cast<CDependencyObject **>(&pNewGeometry),
            &cp));
        pNewGeometry->m_fillMode = XcpFillModeAlternate;
        pNewGeometry->m_fNeedsStretching = FALSE;

        ReplaceRenderGeometry(pNewGeometry);
    }

    pGeometryNoRef = static_cast<CRectangleGeometry *>(m_pRender);
    pGeometryNoRef->m_eRadiusX = m_eRadiusX;
    pGeometryNoRef->m_eRadiusY = m_eRadiusY;
    pGeometryNoRef->m_rc = GetOutlineRect(TRUE /*checkDegenerateStroke*/);

    InvalidateGeometryBounds();

Cleanup:
    ReleaseInterface(pNewGeometry);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to handle shape stretching.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRectangle::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    if (m_Stretch == DirectUI::Stretch::UniformToFill)
    {
        XFLOAT width = availableSize.width;
        XFLOAT height = availableSize.height;

        if (IsInfiniteF(width) && IsInfiniteF(height))
        {
            desiredSize.width = m_eWidth;
            desiredSize.height = m_eHeight;
            return S_OK;
        }
        else if (IsInfiniteF(width) || IsInfiniteF(height))
        {
            width = MIN(width, height);
        }
        else
        {
            width = MAX(width, height);
        }

        desiredSize.width = width;
        desiredSize.height = width;
        return S_OK;
    }

    desiredSize.width = m_eWidth;
    desiredSize.height = m_eHeight;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Override to short-circuit arrange logic.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangle::ArrangeOverride(XSIZEF finalSize, XSIZEF& newFinalSize)

{
    newFinalSize = finalSize;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the stretch mode of the shape will stretch the
//      geometry inside. CRectangle will generate its geometry at the
//      stretched size, so the geometry does not need to be stretched
//      again.
//
//------------------------------------------------------------------------
bool CRectangle::CanStretchGeometry() const
{
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns true if the geometry can be cloned for hardware-
//      acceleration and independent-animation.
//
//------------------------------------------------------------------------
bool CRectangleGeometry::CanBeAccelerated()
{
    // Only axis-aligned rectangular clips are supported, so this geometry
    // can't be accelerated if it has a non-axis-aligning transform, or if
    // it isn't a simple rectangle.
    bool fTransformPreservesAxisAlignment = true;
    if (m_pTransform != NULL)
    {
        // TODO: MERGE: This doesn't detect the case where the transform might not be rotated or skewed now,
        // TODO: MERGE: but will be when ticked as an independent animaton on the render thread.  See SL#95156.
        CMILMatrix mat;
        m_pTransform->GetTransform(&mat);
        fTransformPreservesAxisAlignment = mat.IsScaleOrTranslationOnly();
    }

    return fTransformPreservesAxisAlignment
        && m_eRadiusX == 0.0f
        && m_eRadiusY == 0.0f;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns an axis-aligned rectangle representation of this geometry.
//
//------------------------------------------------------------------------
void CRectangleGeometry::GetRect(_Out_ XRECTF *pRect)
{
    // We can only accurately return an axis-aligned rectangle if this geometry
    // meets the criteria to be accelerated.
    *pRect = m_rc;

    // Apply the geometry transform if it exists.
    if (m_pTransform)
    {
        CMILMatrix mat;
        m_pTransform->GetTransform(&mat);
        mat.TransformBounds(pRect, pRect);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects the geometry with pClipRect.
//
//------------------------------------------------------------------------
/*static*/ void CRectangleGeometry::ApplyClip(
    _In_ CRectangleGeometry *pClipGeometry,
    _In_opt_ const CMILMatrix *pTransformToClipRectSpace,
    _Inout_ XRECTF *pClipRect
    )
{
    XRECTF localClip;
    pClipGeometry->GetRect(&localClip);

    if (pTransformToClipRectSpace)
    {
        pTransformToClipRectSpace->TransformBounds(&localClip, &localClip);
    }

    IntersectRect(pClipRect, &localClip);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects the geometry with pTransformsAndClips
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CRectangleGeometry::ApplyClip(
    _In_ CRectangleGeometry *pClipGeometry,
    _Inout_ TransformAndClipStack *pTransformsAndClips
    )
{
    HWClip localHWClip;
    IFC_RETURN(CRectangleGeometry::InitializeRectangleClip(pClipGeometry, &localHWClip));

    IFC_RETURN(pTransformsAndClips->IntersectLocalSpaceClip(&localHWClip));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects the geometry with pClip
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CRectangleGeometry::ApplyClip(
    _In_ CRectangleGeometry *pClipGeometry,
    _Inout_ HWClip *pClip
    )
{
    HWClip localHWClip;
    IFC_RETURN(CRectangleGeometry::InitializeRectangleClip(pClipGeometry, &localHWClip));

    IFC_RETURN(pClip->Intersect(&localHWClip));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Intersects the geometry with pClip. Assumes this geometry is axis-aligned.
//
//------------------------------------------------------------------------
void CRectangleGeometry::ApplyClip(
    _Inout_ XRECTF *pRect
    )
{
#if DBG
    if (m_pTransform != nullptr)
    {
        CMILMatrix transform(TRUE);
        m_pTransform->GetTransform(&transform);
        ASSERT(transform.IsScaleOrTranslationOnly());
    }
#endif

    // Don't pass in the transform to ApplyClip - ApplyClip already accounts for
    // the transform on the geometry itself.
    CRectangleGeometry::ApplyClip(this, nullptr, pRect);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets pClip to the shape defined by pClipGeometry
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CRectangleGeometry::InitializeRectangleClip(
    _In_ CRectangleGeometry *pClipGeometry,
    _Inout_ HWClip *pClip
    )
{
    pClip->Set(&pClipGeometry->m_rc);

    if (pClipGeometry->m_pTransform)
    {
        CMILMatrix transform;
        pClipGeometry->m_pTransform->GetTransform(&transform);

        IFC_RETURN(pClip->Transform(&transform));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  CRectangleGeometry::GetPrintGeometryVirtual
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      It does not account for any transforms, stretching or clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;
    CPathFigure* pFigure = NULL;

    // TODO:: Use CreateRectangleGeometry instead of PathGeometry
    // TODO:: Merge code with UpdateAcceleratedGeometry
    if (m_eRadiusX == 0.0f && m_eRadiusY == 0.0f)
    {
        IFC(CreateRect(GetContext(), m_rc, &pFigure));
    }
    else
    {
        IFC(CreateRoundedRect(GetContext(), m_rc, m_eRadiusX, m_eRadiusY, &pFigure));
    }

    if (pFigure)
    {
        IFC(cp.GetFactory()->CreatePathGeometry(
            &pPALGeometry
            ));

        IFC(pPALGeometry->Open(&pPALGeometrySink));
        IFC(pFigure->AddAcceleratedFigure(pPALGeometrySink));
        pPALGeometrySink->SetFillMode(static_cast<GeometryFillMode>(m_fillMode));
        IFC(pPALGeometrySink->Close());

        SetInterface(*ppGeometry, pPALGeometry);
    }

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    ReleaseInterface(pPALGeometry);
    ReleaseInterface(pFigure);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    CMILMatrix geometryTransform(TRUE);

    if (m_pTransform != nullptr)
    {
        m_pTransform->GetTransform(&geometryTransform);
    }

    //
    // If the geometry transform is axis aligned or if there are no rounded corners, we can take
    // the bounds of the rectangle and transform the bounds. Otherwise, we'll have to call the
    // base implementation to bound the transformed rounded rectangle.
    //
    if (geometryTransform.IsScaleOrTranslationOnly()
        || (m_eRadiusX == 0.0f && m_eRadiusY == 0.0f))
    {
        *pBounds = ToXRectFRB(m_rc);

        if (!geometryTransform.IsIdentity())
        {
            geometryTransform.TransformBounds(pBounds, pBounds);
        }
    }
    else
    {
        IFC_RETURN(CGeometry::GetBounds(pBounds));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate widened bounds for the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::GetWidenedBounds(
    _In_ const CPlainPen& pen,
    _Out_ XRECTF_RB* pBounds
    )
{
    CMILMatrix geometryTransform(TRUE);

    if (m_pTransform != NULL)
    {
        m_pTransform->GetTransform(&geometryTransform);
    }

    //
    // If the geometry transform is axis aligned, we don't have rounded corners, and the stroke is simple,
    // we can widen our fill bounds trivially and transform them. Otherwise, we'll have to call the expensive
    // base class implementation.
    //
    if (geometryTransform.IsScaleOrTranslationOnly() &&
        (m_eRadiusX == 0.0f && m_eRadiusY == 0.0f))
    {
        if (!pen.IsEmpty())
        {
            XRECTF bounds = m_rc;

            if (!geometryTransform.IsIdentity())
            {
                // Transform first. Stroking is done in geometry-transformed space.
                geometryTransform.TransformBounds(&bounds, &bounds);
            }

            if (CanDoFastWidening(pen, bounds))
            {
                XFLOAT strokeOffset = pen.GetWidth() * 0.5f; // Half of the stroke lies outside of the rect.
                *pBounds = ToXRectFRB(bounds);

                pBounds->left -= strokeOffset;
                pBounds->top -= strokeOffset;
                pBounds->right += strokeOffset;
                pBounds->bottom += strokeOffset;

                return S_OK;
            }
        }
        else
        {
            EmptyRectF(pBounds);
            return S_OK;
        }
    }

    IFC_RETURN(CGeometry::GetWidenedBounds(pen, pBounds));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether or not we can trivially determine how much the
//      given pen will widen the fill rect.
//
//  Note:
//      These heuristics are overly strict, but cover several common cases.
//
//------------------------------------------------------------------------
/*static*/ bool CRectangleGeometry::CanDoFastWidening(
    _In_ const CPlainPen& pen,
    _In_ const XRECTF& bounds
    )
{
    if (IsEmptyRectF(bounds))
    {
        // The geometry is empty, which means we don't know the real bounds as this
        // could be due to the stroke being larger than the element bounds in a
        // Rectangle.
        return false;
    }

    if (pen.GetWidth() > bounds.Width  || pen.GetWidth() > bounds.Height)
    {
        // The stroke covers the entire rect. We don't know the real bounds.
        return false;
    }

    if (pen.GetDashStyle() == XcpDashStyleSolid)
    {
        return true;
    }

    if (pen.GetDashStyle() == XcpDashStyleCustom && pen.GetDashCount() > 0)
    {
        XFLOAT maxDash = pen.GetDash(0);

        for (XUINT32 i = 1; i < pen.GetDashCount(); i++)
        {
            maxDash = MAX(maxDash, pen.GetDash(i));
        }

        maxDash *= pen.GetWidth();

        if (maxDash < bounds.Width && maxDash < bounds.Height)
        {
            // If no dash/gap is larger than the sides of the unwidened rect, there will be a dash
            // touching every side of the rectangle, so we can safely widen on all sides.
            return true;
        }
    }

    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::HitTestFill(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    RRETURN(HitTestFillImpl(target, pTransform, pHit));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry intersects the specified polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::HitTestFill(
    _In_ const HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    return HitTestFillImpl(target, pTransform, pHit);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry intersects the specified
//      point/polygon.
//
//------------------------------------------------------------------------
template <typename HitType>
_Check_return_ HRESULT
CRectangleGeometry::HitTestFillImpl(
    _In_ const HitType& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    bool hit = false;

    if (m_eRadiusX > 0 || m_eRadiusY > 0)
    {
        //
        // Slow path for rounded rectangles.
        //

        IFC_RETURN(CGeometry::HitTestFill(target, pTransform, &hit));
    }
    else
    {
        //
        // Fast path for axis aligned rectangle.
        //

        if (pTransform != NULL || m_pTransform != NULL)
        {
            CMILMatrix transform(TRUE);

            if (pTransform != NULL)
            {
                transform = *pTransform;
            }

            if (m_pTransform != NULL)
            {
                CMILMatrix geometryTransform;

                m_pTransform->GetTransform(&geometryTransform);

                transform.Append(geometryTransform);
            }

            if (transform.Invert())
            {
                HitType transformedTarget;

                ApplyTransformToHitType(&transform, &target, &transformedTarget);

                hit = DoesRectIntersectHitType(m_rc, transformedTarget);
            }
            else
            {
                hit = FALSE;
            }
        }
        else
        {
            hit = DoesRectIntersectHitType(m_rc, target);
        }
    }

    *pHit = hit;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRectangleGeometry::VisitSinkInternal(
    _In_ IPALGeometrySink* pSink
    )
{
    HRESULT hr = S_OK;

    pSink->SetFillMode(GeometryFillMode::Alternate);

    if (m_eRadiusX > 0 || m_eRadiusY > 0)
    {
        XFLOAT radiusX = m_eRadiusX;
        XFLOAT radiusY = m_eRadiusY;

        // Do some extra validation
        if (radiusX < 0.0f)
        {
            radiusX = -radiusX;
        }

        if (radiusY < 0.0f)
        {
            radiusY = -radiusY;
        }

        if (radiusX > (m_rc.Width / 2.0f))
        {
            radiusX = (m_rc.Width / 2.0f);
        }

        if (radiusY > (m_rc.Height / 2.0f))
        {
            radiusY = (m_rc.Height / 2.0f);
        }

        // Use (1 - ARC_AS_BEZIER) - because we measure from the edge of the rectangle
        // not the center of the arc.
        XFLOAT rBezierX = (1.0f - ARC_AS_BEZIER) * radiusX;
        XFLOAT rBezierY = (1.0f - ARC_AS_BEZIER) * radiusY;
        XFLOAT rRight = m_rc.X + m_rc.Width;
        XFLOAT rBottom = m_rc.Y + m_rc.Height;

        XPOINTF points[16];

        // First set all the X coordinates
        points[1].x  = points[0].x  = points[15].x = points[14].x = m_rc.X;
        points[2].x  = points[13].x = m_rc.X  + rBezierX;
        points[3].x  = points[12].x = m_rc.X  + radiusX;
        points[4].x  = points[11].x = rRight - radiusX;
        points[5].x  = points[10].x = rRight - rBezierX;
        points[6].x  = points[7].x  = points[8].x  = points[9].x  = rRight;

        // Then all the Y coordinates
        points[2].y  = points[3].y  = points[4].y  = points[5].y  = m_rc.Y;
        points[1].y  = points[6].y  = m_rc.Y  + rBezierY;
        points[0].y  = points[7].y  = m_rc.Y  + radiusY;
        points[15].y = points[8].y  = rBottom - radiusY;
        points[14].y = points[9].y  = rBottom - rBezierY;
        points[13].y = points[12].y = points[11].y = points[10].y = rBottom;

        const XPOINTF* pReadOffset = points;

        pSink->BeginFigure(*pReadOffset, FALSE);
        pReadOffset += 1;

        pSink->AddBeziers(pReadOffset, 3);
        pReadOffset += 3;

        pSink->AddLine(*pReadOffset);
        pReadOffset += 1;

        pSink->AddBeziers(pReadOffset, 3);
        pReadOffset += 3;

        pSink->AddLine(*pReadOffset);
        pReadOffset += 1;

        pSink->AddBeziers(pReadOffset, 3);
        pReadOffset += 3;

        pSink->AddLine(*pReadOffset);
        pReadOffset += 1;

        pSink->AddBeziers(pReadOffset, 3);
        pReadOffset += 3;

        pSink->EndFigure(TRUE);
    }
    else
    {
        XPOINTF startPoint = { };

        startPoint.x = m_rc.X;
        startPoint.y = m_rc.Y;

        pSink->BeginFigure(startPoint, FALSE);

        XPOINTF points[] =
        {
            { m_rc.X + m_rc.Width, m_rc.Y },
            { m_rc.X + m_rc.Width, m_rc.Y + m_rc.Height },
            { m_rc.X, m_rc.Y + m_rc.Height }
        };

        pSink->AddLines(points, ARRAY_SIZE(points));

        pSink->EndFigure(TRUE);
    }

    return hr;
}

WUComp::ICompositionGeometry* CRectangleGeometry::GetCompositionGeometry(_In_ VisualContentRenderer* renderer)
{
    wfn::Vector2 size = { m_rc.Width, m_rc.Height };
    wfn::Vector2 offset = { m_rc.X, m_rc.Y };

    if (m_eRadiusX == 0.0f && m_eRadiusY == 0.0f)
    {
        return GetSimpleRectangleCompositionGeometry(renderer, size, offset);
    }
    else
    {
        return GetRoundedRectangleCompositionGeometry(renderer, size, offset);
    }

}

WUComp::ICompositionGeometry* CRectangleGeometry::GetSimpleRectangleCompositionGeometry(
    _In_ VisualContentRenderer* renderer,
    const wfn::Vector2 &size,
    const wfn::Vector2 &offset)
{
    if (IsGeometryDirty() || m_wucGeometry == nullptr)
    {
        // Create shape-specific geometry
        wrl::ComPtr<WUComp::ICompositionRectangleGeometry> rectGeo;

        if (m_wucGeometry == nullptr || FAILED(m_wucGeometry.As(&rectGeo)))
        {
            IFCFAILFAST(renderer->GetCompositor5()->CreateRectangleGeometry(&rectGeo));
            IFCFAILFAST(rectGeo.As(&m_wucGeometry));
        }

        // Set size and offset
        IFCFAILFAST(rectGeo->put_Size(size));
        IFCFAILFAST(rectGeo->put_Offset(offset));

        SetWUCGeometryDirty(false);
    }

    return m_wucGeometry.Get();
}

WUComp::ICompositionGeometry* CRectangleGeometry::GetRoundedRectangleCompositionGeometry(
    _In_ VisualContentRenderer* renderer,
    const wfn::Vector2 &size,
    const wfn::Vector2 &offset)
{
    if (IsGeometryDirty() || m_wucGeometry == nullptr)
    {
        // Create shape-specific geometry
        wrl::ComPtr<WUComp::ICompositionRoundedRectangleGeometry> rectGeo;

        if (m_wucGeometry == nullptr || FAILED(m_wucGeometry.As(&rectGeo)))
        {
            IFCFAILFAST(renderer->GetCompositor5()->CreateRoundedRectangleGeometry(&rectGeo));
            IFCFAILFAST(rectGeo.As(&m_wucGeometry));
        }

        // Set size and offset
        IFCFAILFAST(rectGeo->put_Size(size));
        IFCFAILFAST(rectGeo->put_Offset(offset));

        // Set corner radius
        wfn::Vector2 cornerRadius = { m_eRadiusX, m_eRadiusY };
        IFCFAILFAST(rectGeo->put_CornerRadius(cornerRadius));

        SetWUCGeometryDirty(false);
    }

    return m_wucGeometry.Get();
}

