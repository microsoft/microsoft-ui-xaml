// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      CPathFigure destructor
//
//------------------------------------------------------------------------

CPathFigure::~CPathFigure()
{
    ReleaseInterface(m_pSegments);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries for this figure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathFigure::AddAcceleratedFigure(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    if (m_pSegments)
    {
        pPALGeometrySink->BeginFigure(
            m_ptStart,
            !m_bFilled
            );

        IFC_RETURN(m_pSegments->AddAcceleratedSegments(
            pPALGeometrySink
            ));

        pPALGeometrySink->EndFigure(
            m_bClosed
            );
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Builds D2D geometries for this figure collection.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPathFigureCollection::AddAcceleratedFigures(
    _In_ IPALGeometrySink *pPALGeometrySink
    )
{
    for (auto item : (*this))
    {
        IFC_RETURN(static_cast<CPathFigure*>(item)->AddAcceleratedFigure(pPALGeometrySink));
    }

    return S_OK;
}


