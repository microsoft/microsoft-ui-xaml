// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualContentRenderer.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to create an elliptical figure
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CreateEllipse(
    _In_ CCoreServices *pCore,
    _In_ const XPOINTF& ptCenter,
    _In_ const XFLOAT& inputRadiusX,
    _In_ const XFLOAT& inputRadiusY,
    _Outptr_ CPathFigure **ppFigure
    )
{
    HRESULT hr;
    XFLOAT radiusX = inputRadiusX;
    XFLOAT radiusY = inputRadiusY;

    CPointCollection *pPoints = nullptr;
    CPolyBezierSegment *pCurves = nullptr;
    CPathSegmentCollection *pSegments = nullptr;
    CPathFigure *pFigure = nullptr;
    CValue value;

    // Do some parameter validation
    if (radiusX < 0.0f)
    {
        radiusX = -radiusX;
    }
    if (radiusY < 0.0f)
    {
        radiusY = -radiusY;
    }

    // Build an ellipse geometry of the current height/width
    XFLOAT  eMid;
    XPOINTF aPoints[13];

    // First compute the X coordinates
    eMid = radiusX * ARC_AS_BEZIER;

    aPoints[0].x = aPoints[1].x = aPoints[11].x = aPoints[12].x = ptCenter.x + radiusX;
    aPoints[2].x = aPoints[10].x = ptCenter.x + eMid;
    aPoints[3].x = aPoints[9].x = ptCenter.x;
    aPoints[4].x = aPoints[8].x = ptCenter.x - eMid;
    aPoints[5].x = aPoints[6].x = aPoints[7].x  = ptCenter.x - radiusX;

    // Then the Y coordinates
    eMid = radiusY * ARC_AS_BEZIER;

    aPoints[2].y = aPoints[3].y = aPoints[4].y = ptCenter.y + radiusY;
    aPoints[1].y = aPoints[5].y = ptCenter.y + eMid;
    aPoints[0].y = aPoints[6].y = aPoints[12].y = ptCenter.y;
    aPoints[7].y = aPoints[11].y = ptCenter.y - eMid;
    aPoints[8].y = aPoints[9].y = aPoints[10].y = ptCenter.y - radiusY;

    CREATEPARAMETERS cp(pCore);
    IFC(CPointCollection::Create(reinterpret_cast<CDependencyObject**>(&pPoints), &cp));
    IFC(pPoints->InitFromArray(12, &aPoints[1]));

    IFC(CPolyBezierSegment::Create(reinterpret_cast<CDependencyObject**>(&pCurves), &cp));
    value.WrapObjectNoRef(pPoints);
    IFC(pCurves->SetValue(pCurves->GetPropertyByIndexInline(KnownPropertyIndex::PolyBezierSegment_Points), value));

    IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject**>(&pSegments), &cp));
    value.WrapObjectNoRef(pCurves);
    IFC(pSegments->SetValue(pSegments->GetContentProperty(), value));

    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject**>(&pFigure), &cp));
    value.WrapObjectNoRef(pSegments);
    IFC(pFigure->SetValue(pFigure->GetContentProperty(), value));

    // Set its start point, mark it filled and closed
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
//      Creates the geometry that will be used to render this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEllipse::UpdateRenderGeometry()
{
    HRESULT hr = S_OK;
    CEllipseGeometry *pNewGeometry = nullptr;
    CEllipseGeometry *pGeometryNoRef = nullptr;
    XRECTF rectBounds = GetOutlineRect(FALSE /* fCheckDegenerateStroke */);

    if (m_pRender == NULL)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC(CEllipseGeometry::Create(
            reinterpret_cast<CDependencyObject **>(&pNewGeometry),
            &cp));
        pNewGeometry->m_fillMode = XcpFillModeAlternate;
        pNewGeometry->m_fNeedsStretching = FALSE;

        ReplaceRenderGeometry(pNewGeometry);
    }

    pGeometryNoRef = static_cast<CEllipseGeometry *>(m_pRender);
    pGeometryNoRef->m_eRadiusX = (rectBounds.Width) * 0.5f;
    pGeometryNoRef->m_eRadiusY = (rectBounds.Height) * 0.5f;
    pGeometryNoRef->m_ptCenter.x = rectBounds.X + (pGeometryNoRef->m_eRadiusX);
    pGeometryNoRef->m_ptCenter.y = rectBounds.Y + (pGeometryNoRef->m_eRadiusY);

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
_Check_return_ HRESULT CEllipse::MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
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
CEllipse::ArrangeOverride(_In_ XSIZEF finalSize, _Out_ XSIZEF& newFinalSize)
{
    newFinalSize = finalSize;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the stretch mode of the shape will stretch the
//      geometry inside. CEllipse will generate its geometry at the
//      stretched size, so the geometry does not need to be stretched
//      again.
//
//------------------------------------------------------------------------
bool CEllipse::CanStretchGeometry() const
{
    return false;
}

//------------------------------------------------------------------------
//
//  CEllipseGeometry::GetPrintGeometryVirtual
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      It does not account for any transforms, stretching or clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEllipseGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;
    IPALGeometrySink *pPALGeometrySink = NULL;
    CPathFigure* pFigure = NULL;

    IFC(CreateEllipse(GetContext(), m_ptCenter, m_eRadiusX, m_eRadiusY, &pFigure));
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
CEllipseGeometry::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    CMILMatrix geometryTransform(TRUE);

    if (m_pTransform != NULL)
    {
        m_pTransform->GetTransform(&geometryTransform);
    }

    //
    // If the geometry transform is axis aligned, we can take the bounds of the ellipse and
    // transform the bounds. If the transform contains rotational components, we'll have to
    // call the base implementation to bound the transformed ellipse.
    //
    if (geometryTransform.IsScaleOrTranslationOnly())
    {
        pBounds->left = m_ptCenter.x - m_eRadiusX;
        pBounds->top = m_ptCenter.y - m_eRadiusY;
        pBounds->right = m_ptCenter.x + m_eRadiusX;
        pBounds->bottom = m_ptCenter.y + m_eRadiusY;

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
//      Solve ellipse equation to test for in/out of bounds.
//
//------------------------------------------------------------------------
XFLOAT SolveEllipse(
    _In_ const XPOINTF& point,
    XFLOAT radiusX,
    XFLOAT radiusY
    )
{
    return ((point.x * point.x) / (radiusX * radiusX)) + ((point.y * point.y) / (radiusY * radiusY));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry contains the specified point.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEllipseGeometry::HitTestFill(
    _In_ const XPOINTF& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    bool containsPoint = false;

    if (m_eRadiusX == 0 || m_eRadiusY == 0)
    {
        containsPoint = FALSE;
    }
    else
    {
        //
        // Fast path for axis aligned ellipse.
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
                XPOINTF transformedPoint = {0};

                transform.Transform(&target, &transformedPoint, 1);

                XFLOAT val = SolveEllipse(transformedPoint - m_ptCenter, m_eRadiusX, m_eRadiusY);

                containsPoint = val <= 1.0f;
            }
            else
            {
                containsPoint = FALSE;
            }
        }
        else
        {
            XFLOAT val = SolveEllipse(target - m_ptCenter, m_eRadiusX, m_eRadiusY);

            containsPoint = val <= 1.0f;
        }
    }

    *pHit = containsPoint;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if the fill of this geometry intersects the specified polygon.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEllipseGeometry::HitTestFill(
    _In_ const HitTestPolygon& target,
    _In_opt_ const CMILMatrix* pTransform,
    _Out_ bool* pHit
    )
{
    // Slow path for polygon intersection.
    IFC_RETURN(CGeometry::HitTestFill(target, pTransform, pHit));

    return S_OK;

}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CEllipseGeometry::VisitSinkInternal(
    _In_ IPALGeometrySink* pSink
    )
{
    HRESULT hr = S_OK;
    XFLOAT radiusX = 0;
    XFLOAT radiusY = 0;
    XFLOAT middle = 0;
    XPOINTF ellipsePoints[13];

    radiusX = m_eRadiusX;
    radiusY = m_eRadiusY;

    if (radiusX < 0.0f)
    {
        radiusX = -radiusX;
    }

    if (radiusY < 0.0f)
    {
        radiusY = -radiusY;
    }

    //
    // Compute the X coordinates.
    //
    middle = radiusX * ARC_AS_BEZIER;

    ellipsePoints[0].x = ellipsePoints[1].x = ellipsePoints[11].x = ellipsePoints[12].x = m_ptCenter.x + radiusX;
    ellipsePoints[2].x = ellipsePoints[10].x = m_ptCenter.x + middle;
    ellipsePoints[3].x = ellipsePoints[9].x = m_ptCenter.x;
    ellipsePoints[4].x = ellipsePoints[8].x = m_ptCenter.x - middle;
    ellipsePoints[5].x = ellipsePoints[6].x = ellipsePoints[7].x  = m_ptCenter.x - radiusX;

    //
    // Compute the Y coordinates.
    //
    middle = radiusY * ARC_AS_BEZIER;

    ellipsePoints[2].y = ellipsePoints[3].y = ellipsePoints[4].y = m_ptCenter.y + radiusY;
    ellipsePoints[1].y = ellipsePoints[5].y = m_ptCenter.y + middle;
    ellipsePoints[0].y = ellipsePoints[6].y = ellipsePoints[12].y = m_ptCenter.y;
    ellipsePoints[7].y = ellipsePoints[11].y = m_ptCenter.y - middle;
    ellipsePoints[8].y = ellipsePoints[9].y = ellipsePoints[10].y = m_ptCenter.y - radiusY;

    pSink->SetFillMode(GeometryFillMode::Alternate);

    pSink->BeginFigure(ellipsePoints[0], FALSE);

    pSink->AddBeziers(&ellipsePoints[1], ARRAY_SIZE(ellipsePoints) - 1);

    pSink->EndFigure(TRUE);

    return hr;
}

WUComp::ICompositionGeometry* CEllipseGeometry::GetCompositionGeometry(_In_ VisualContentRenderer* renderer)
{
    if (IsGeometryDirty() || m_wucGeometry == nullptr)
    {
        // Create shape-specific geometry
        wrl::ComPtr<WUComp::ICompositionEllipseGeometry> ellipseGeo;

        if (m_wucGeometry == nullptr)
        {
            IFCFAILFAST(renderer->GetCompositor5()->CreateEllipseGeometry(&ellipseGeo));
            IFCFAILFAST(ellipseGeo.As(&m_wucGeometry));
        }
        else
        {
            IFCFAILFAST(m_wucGeometry.As(&ellipseGeo));
        }

        // Set Radius
        IFCFAILFAST(ellipseGeo->put_Radius({ m_eRadiusX, m_eRadiusY }));
        // Set Center
        IFCFAILFAST(ellipseGeo->put_Center({ m_ptCenter.x, m_ptCenter.y }));

        SetWUCGeometryDirty(false);
    }

    return m_wucGeometry.Get();
}

