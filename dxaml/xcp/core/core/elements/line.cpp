// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VisualContentRenderer.h"

extern void UpdateBounds(
    _Inout_ XRECTF_RB *prcLocalSpaceBounds,          // The local space bounds
    _In_reads_(cPoints) const XPOINTF *pPtsSource,  // Source points
    _In_ XINT32 cPoints                              // Count of points
    );

extern void StartBounds(
    _Out_ XRECTF_RB* pBounds,          // The local space bounds
    _In_ const XPOINTF* pStartPoint    // Source points
    );

// Helper function to create a linear figure
_Check_return_ HRESULT
CreateLine(
    _In_ CCoreServices *pCore,
    _In_ const XPOINTF& ptStart,
    _In_ const XPOINTF& ptEnd,
    _Outptr_ CPathFigure **ppFigure
    )
{
    HRESULT hr;
    CLineSegment *pCurves = nullptr;
    CPathSegmentCollection *pSegments = nullptr;
    CPathFigure *pFigure = nullptr;
    CValue value;

    CREATEPARAMETERS cp(pCore);

    IFC(CLineSegment::Create(reinterpret_cast<CDependencyObject**>(&pCurves), &cp));
    pCurves->m_pt = ptEnd;

    IFC(CPathSegmentCollection::Create(reinterpret_cast<CDependencyObject**>(&pSegments), &cp));
    value.WrapObjectNoRef(pCurves);
    IFC(pSegments->SetValue(pSegments->GetContentProperty(), value));

    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject**>(&pFigure), &cp));
    value.WrapObjectNoRef(pSegments);
    IFC(pFigure->SetValue(pFigure->GetContentProperty(), value));

    // Set its start point, mark it unfilled and open
    pFigure->m_ptStart = ptStart;
    pFigure->m_bClosed = false;

    *ppFigure = pFigure;
    pFigure = nullptr;

Cleanup:
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
CLine::UpdateRenderGeometry()
{
    HRESULT hr = S_OK;
    CLineGeometry *pNewGeometry = nullptr;
    CLineGeometry *pGeometryNoRef = nullptr;

    if (m_pRender == nullptr)
    {
        CREATEPARAMETERS cp(GetContext());

        IFC(CLineGeometry::Create(
            reinterpret_cast<CDependencyObject **>(&pNewGeometry),
            &cp));
        pNewGeometry->m_fillMode = XcpFillModeAlternate;

        ReplaceRenderGeometry(pNewGeometry);
    }

    pGeometryNoRef = static_cast<CLineGeometry *>(m_pRender);
    pGeometryNoRef->m_ptStart.x = m_eX1;
    pGeometryNoRef->m_ptStart.y = m_eY1;
    pGeometryNoRef->m_ptEnd.x = m_eX2;
    pGeometryNoRef->m_ptEnd.y = m_eY2;

    InvalidateGeometryBounds();

Cleanup:
    ReleaseInterface(pNewGeometry);
    return hr;
}

//------------------------------------------------------------------------
//
//  CLineGeometry::GetPrintGeometryVirtual
//
//  Synopsis:
//      Creates and returns a print geometry for this Silverlight geometry.
//      It does not account for any transforms, stretching or clipping.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLineGeometry::GetPrintGeometryVirtual(
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams,
    IPALAcceleratedGeometry** ppGeometry
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedPathGeometry *pPALGeometry = nullptr;
    IPALGeometrySink *pPALGeometrySink = nullptr;

    IFC(cp.GetFactory()->CreatePathGeometry(
        &pPALGeometry
        ));

    IFC(pPALGeometry->Open(&pPALGeometrySink));
    pPALGeometrySink->BeginFigure(
        m_ptStart,
        TRUE    // fIsHollow
        );
    pPALGeometrySink->AddLine(m_ptEnd);
    pPALGeometrySink->EndFigure(
        FALSE   // fIsClosed
        );
    pPALGeometrySink->SetFillMode(static_cast<GeometryFillMode>(m_fillMode));
    IFC(pPALGeometrySink->Close());

    SetInterface(*ppGeometry, pPALGeometry);

Cleanup:
    ReleaseInterface(pPALGeometrySink);
    ReleaseInterface(pPALGeometry);
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLineGeometry::GetBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    CMILMatrix geometryTransform(TRUE);

    if (m_pTransform != nullptr)
    {
        m_pTransform->GetTransform(&geometryTransform);
    }

    //
    // If the geometry transform is axis aligned, we can take the bounds of the line and
    // transform the bounds. If the transform contains rotational components, we'll have to
    // call the base implementation to bound the transformed line.
    //
    if (geometryTransform.IsScaleOrTranslationOnly())
    {
        StartBounds(pBounds, &m_ptStart);
        UpdateBounds(pBounds, &m_ptEnd, 1);

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
//      Add geometry to the specified sink.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CLineGeometry::VisitSinkInternal(
    _In_ IPALGeometrySink* pSink
    )
{
    pSink->SetFillMode(GeometryFillMode::Alternate);
    pSink->BeginFigure(m_ptStart, FALSE);
    pSink->AddLines(&m_ptEnd, 1);
    pSink->EndFigure(FALSE);

    return S_OK;
}

WUComp::ICompositionGeometry* CLineGeometry::GetCompositionGeometry(_In_ VisualContentRenderer* renderer)
{
    if (IsGeometryDirty() || m_wucGeometry == nullptr)
    {
        // Create shape-specific geometry
        wrl::ComPtr<WUComp::ICompositionLineGeometry> lineGeo;

        if (m_wucGeometry == nullptr)
        {
            IFCFAILFAST(renderer->GetCompositor5()->CreateLineGeometry(&lineGeo));
            IFCFAILFAST(lineGeo.As(&m_wucGeometry));
        }
        else
        {
            IFCFAILFAST(m_wucGeometry.As(&lineGeo));
        }

        // Set start
        IFCFAILFAST(lineGeo->put_Start({ m_ptStart.x, m_ptStart.y }));
        // Set end
        IFCFAILFAST(lineGeo->put_End({ m_ptEnd.x, m_ptEnd.y }));

        SetWUCGeometryDirty(false);
    }

    return m_wucGeometry.Get();
}

