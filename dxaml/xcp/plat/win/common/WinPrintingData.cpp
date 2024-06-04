// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::CWinPrintingData
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------
CWinPrintingData::CWinPrintingData()
{
    m_hPrinter                  = NULL;
    m_fStretch                  = FALSE;
    m_nPrinterWidthInPixels     = 0;
    m_nPrinterHeightInPixels    = 0;
    m_passthroughMode           = PASSTHROUGH;
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData Destructor
//
//  Synopsis:
//      Resets the state of the object and releases the handles.
//
//------------------------------------------------------------------------
CWinPrintingData::~CWinPrintingData()
{
    if(m_hPrinter)
    {
        DeleteDC(m_hPrinter);
        m_hPrinter = NULL;
    }

    m_fStretch                  = FALSE;
    m_nPrinterWidthInPixels     = 0;
    m_nPrinterHeightInPixels    = 0;
}


//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::SetPrinterHandle
//
//  Synopsis:
//      Sets the printer handle.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CWinPrintingData::SetPrinterHandle(HDC hPrinter)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(hPrinter);
    IFCEXPECT_ASSERT(!m_hPrinter);

    m_hPrinter = hPrinter;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::
//
//  Synopsis:
//      Returns the printer handle.
//
//------------------------------------------------------------------------
_Check_return_ HDC 
CWinPrintingData::GetPrinterHandle()
{
    return m_hPrinter;
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::SetShouldStretch
//
//  Synopsis:
//      Sets whether the image should be stretched to device or not.
//
//------------------------------------------------------------------------
void  
CWinPrintingData::SetShouldStretch(bool fStretch)
{
    m_fStretch = fStretch;
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::GetShouldStretch
//
//  Synopsis:
//      Returns whether the image should be stretched to device or not.
//
//------------------------------------------------------------------------
_Check_return_ bool 
CWinPrintingData::GetShouldStretch()
{
    return m_fStretch;
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::SetPrinterWidthInPixels
//
//  Synopsis:
//      Sets printer width in pixels.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CWinPrintingData::SetPrinterWidthInPixels(XUINT32 nPrinterWidthInPixels)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPrinterWidthInPixels > 0);

    m_nPrinterWidthInPixels = nPrinterWidthInPixels;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::GetPrinterWidthInPixels
//
//  Synopsis:
//      Returns printer width in pixels.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 
CWinPrintingData::GetPrinterWidthInPixels()
{
    return m_nPrinterWidthInPixels;
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::SetPrinterHeightInPixels
//
//  Synopsis:
//      Sets printer height in pixels.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CWinPrintingData::SetPrinterHeightInPixels(XUINT32 nPrinterHeightInPixels)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPrinterHeightInPixels > 0);

    m_nPrinterHeightInPixels = nPrinterHeightInPixels;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CWinPrintingData::GetPrinterHeightInPixels
//
//  Synopsis:
//      Returns printer height in pixels.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 
CWinPrintingData::GetPrinterHeightInPixels()
{
    return m_nPrinterHeightInPixels;
}

void CWinPrintingData::SetSupportedExtEscapePassthroughMode(int passthroughMode)
{
    // this is one of the following 3 Windows constants -
    // PASSTHROUGH, POSTSCRIPT_PASSTHROUGH, PASSTHROUGH_DATA
    m_passthroughMode = passthroughMode;
}

int CWinPrintingData::GetSupportedExtEscapePassthroughMode()
{
    // this is one of the following 3 Windows constants -
    // PASSTHROUGH, POSTSCRIPT_PASSTHROUGH, PASSTHROUGH_DATA
    return m_passthroughMode;
}

