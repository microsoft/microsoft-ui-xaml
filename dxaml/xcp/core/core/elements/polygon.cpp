// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Synopsis:
//      Destructor for polygon object
//
//------------------------------------------------------------------------
CPolygon::~CPolygon()
{
    ReleaseInterface(m_pPoints);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures the geometry exists. Sets out param to indicate whether
//      the geometry needs to be updated.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPolygon::EnsureGeometry()
{
    HRESULT hr = S_OK;
    CPathGeometry *pNewGeometry = NULL;

    if (m_pPoints && (m_pPoints->GetCount() > 1))
    {
        const xvector<XPOINTF>& pPoints = m_pPoints->GetCollection();

        if (!pPoints.empty())
        {
            CREATEPARAMETERS cp(GetContext());

            if (m_pRender == NULL)
            {
                IFC(CPathGeometry::Create(
                    reinterpret_cast<CDependencyObject **>(&pNewGeometry),
                    &cp));
                pNewGeometry->m_fillMode = static_cast<XcpFillMode>(m_nFillRule);

                ReplaceRenderGeometry(pNewGeometry);
            }
        }
        else
        {
            // No pPoints; not enough data to render
            ReplaceRenderGeometry(NULL);
        }
    }
    else
    {
        // No m_pPoints or count is 0; not enough data to render
        ReplaceRenderGeometry(NULL);
    }

Cleanup:
    ReleaseInterface(pNewGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates the geometry that will be used to render this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPolygon::UpdateRenderGeometry()
{
    IFC_RETURN(EnsureGeometry());

    if (m_pRender != NULL)
    {
        CPathGeometry *pGeometryNoRef = static_cast<CPathGeometry *>(m_pRender);
        const xvector<XPOINTF>& points = m_pPoints->GetCollection();
        IFC_RETURN(pGeometryNoRef->DrawLines(
            points.data(),
            points.size(),
            TRUE /* fIsClosed */));
    }

    InvalidateGeometryBounds();

    return S_OK;
}
