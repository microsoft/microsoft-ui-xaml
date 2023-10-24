// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Builds the earmark path.
_Check_return_ HRESULT
CListViewBaseItemSecondaryChrome::PrepareEarmarkPath()
{
    HRESULT hr = S_OK;

    auto core = GetContext();
    CREATEPARAMETERS cp(core);
    CValue cVal;

    CPathGeometry *pGeo = NULL;
    CPathFigure *pFigure = NULL;
    CPathSegmentCollection *pSegmentsNoRef = NULL;
    CPathFigureCollection *pFiguresNoRef = NULL;

    XPOINTF startPoint = {0.0f, 0.0f};
    XPOINTF point1 = {40.0f, 0.0f};
    XPOINTF point2 = {40.0f, 40.0f};

    if (m_pEarmarkGeometryData)
    {
        goto Cleanup;
    }

    IFC(CPathGeometry::Create(reinterpret_cast<CDependencyObject **>(&pGeo), &cp));
    IFC(CPathFigure::Create(reinterpret_cast<CDependencyObject **>(&pFigure), &cp));

    // Set start point.
    cVal.WrapPoint(&startPoint);
    IFC(pFigure->SetValueByIndex(KnownPropertyIndex::PathFigure_StartPoint, cVal));

    // Get and populate the line segments.
    IFC(pFigure->GetValueByIndex(KnownPropertyIndex::PathFigure_Segments, &cVal));
    pSegmentsNoRef = do_pointer_cast<CPathSegmentCollection>(cVal.AsObject());
    IFCEXPECT(pSegmentsNoRef);

    IFC(CListViewBaseItemChrome::AddLineSegmentToSegmentCollection(core, pSegmentsNoRef, point1));
    IFC(CListViewBaseItemChrome::AddLineSegmentToSegmentCollection(core, pSegmentsNoRef, point2));

    // Populate the figure collection.
    IFC(pGeo->GetValueByIndex(KnownPropertyIndex::PathGeometry_Figures, &cVal));
    pFiguresNoRef = do_pointer_cast<CPathFigureCollection>(cVal.AsObject());

    cVal.WrapObjectNoRef(pFigure);
    IFC(CCollection::Add(pFiguresNoRef, 1, &cVal, NULL));

    // Get the bounds of the geometry.
    IFC(pGeo->GetBounds(&m_earmarkGeometryBounds));

    // Save the geometry for use in rendering.
    m_pEarmarkGeometryData = pGeo;
    pGeo = NULL;

Cleanup:
    ReleaseInterface(pGeo);
    ReleaseInterface(pFigure);

    RRETURN(hr);
}

// Provides the geometry of the earmark, so the renderer knows how to size the brush.
_Check_return_ HRESULT
CListViewBaseItemSecondaryChrome::GetEarmarkBounds(_Out_ XRECTF_RB* pBounds)
{
    *pBounds = m_earmarkGeometryBounds;
    RRETURN(S_OK);
}
