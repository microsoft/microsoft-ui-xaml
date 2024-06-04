// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Used as a root for an element to be printed to allow for proper 
// Measure and Arrange

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method: MeasureOverride
//
//  Synopsis:
//      Implementation for MeasureOverride virtual.
//      Print Root must have only one Child at a time, we get the first
//      child and measure it.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CPrintRoot::MeasureOverride(_In_ XSIZEF availableSize, _Out_ XSIZEF& desiredSize)
{
    CUIElement* pChild = nullptr;

    // Get the first - and only - child of the print root to measure it.
    IFC_RETURN(DoPointerCast(pChild, GetFirstChildNoAddRef()));

    if (pChild)
    {
        IFC_RETURN(pChild->Measure(m_sPageSize));
        IFC_RETURN(pChild->EnsureLayoutStorage());
        desiredSize = pChild->DesiredSize;
    }
    else
    {
        desiredSize.width = 0;
        desiredSize.height = 0;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: ArrangeOverride
//
//  Synopsis:
//      Implementation for ArrangeOverride virtual.
//      Print Root must have only one Child at a time, we get the first
//      child and arrange it.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT 
CPrintRoot::ArrangeOverride(
                            _In_ XSIZEF finalSize, 
                            _Out_ XSIZEF& newFinalSize)
{
    HRESULT hr = S_OK;
    CUIElement* pChild = NULL;

    // Get the first - and only - child of the print root to arrange it.
    IFC(DoPointerCast(pChild, GetFirstChildNoAddRef()));

    if (pChild)
    {
        XRECTF arrangeRect = {0, 0, m_sPageSize.width, m_sPageSize.height};
        IFC( pChild->Arrange(arrangeRect) );
    }

Cleanup:
    newFinalSize = finalSize;
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method: SetSize
//
//  Synopsis:
//      Sets the page size of the current print job.
//      Used to properly measure the content of the page.
//
//------------------------------------------------------------------------
_Check_return_ 
HRESULT 
CPrintRoot::SetSize(
                    _In_ XSIZEF pageSize)
{
    IFC_RETURN(pageSize.width  <= 0 ? E_INVALIDARG : S_OK);
    IFC_RETURN(pageSize.height <= 0 ? E_INVALIDARG : S_OK); 

    m_sPageSize = pageSize;

    return S_OK;
}

