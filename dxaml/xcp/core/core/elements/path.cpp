// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for path object
//
//------------------------------------------------------------------------
CPath::~CPath()
{
    ReleaseInterface(m_pGeometryData);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the geometry that will be used to render this shape.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPath::UpdateRenderGeometry()
{
    HRESULT hr = S_OK;

    // Path has no fields to update its geometry with. Instead
    // there's a publicly exposed geometry that is already up to date. Just
    // set it as the geometry that CShape will render.
    ReplaceRenderGeometry(m_pGeometryData);

    InvalidateGeometryBounds();

    RRETURN(hr);
}
