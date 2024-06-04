// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"


//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::CCommonPrintingData
//
//  Synopsis:
//      
//
//------------------------------------------------------------------------
CCommonPrintingData::CCommonPrintingData()
{
    m_fPrintingStarted      = FALSE;
    m_pszDocumentName       = NULL;
    m_fScaleX               = 1.0f;
    m_fScaleY               = 1.0f;
    m_nPageRangeFrom        = 1;
    m_nPageRangeTo          = 1;
    m_nPrintableAreaWidth   = 0;
    m_nPrintableAreaHeight  = 0;
    m_tPageMargins.left     = 0;
    m_tPageMargins.top      = 0;
    m_tPageMargins.right    = 0;
    m_tPageMargins.bottom   = 0;
    m_fSupportsPostscript   = FALSE;
    m_fIsPrintFormatPostscript = FALSE;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData Destructor
//
//  Synopsis:
//      Resets the CCommonPrintingData state, and frees the resources.
//
//------------------------------------------------------------------------
CCommonPrintingData::~CCommonPrintingData()
{
    IPlatformUtilities *pUtilities = nullptr;
    VERIFYHR(GetPALCoreServices()->GetPlatformUtilities(&pUtilities));
    if(pUtilities && m_pszDocumentName)
    {
        pUtilities->Xstrfree(m_pszDocumentName);
        m_pszDocumentName = NULL;
    }

    m_fPrintingStarted      = FALSE;
    m_fScaleX               = 1.0f;
    m_fScaleY               = 1.0f;
    m_nPageRangeFrom        = 1;
    m_nPageRangeTo          = 1;
    m_nPrintableAreaWidth   = 0;
    m_nPrintableAreaHeight  = 0;
    m_tPageMargins.left     = 0;
    m_tPageMargins.top      = 0;
    m_tPageMargins.right    = 0;
    m_tPageMargins.bottom   = 0;
    m_fSupportsPostscript   = FALSE;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetDidPrintingStart
//
//  Synopsis:
//      Sets whether printing has started or not.
//
//------------------------------------------------------------------------
void  
CCommonPrintingData::SetDidPrintingStart(bool fPrintingStarted)
{
    m_fPrintingStarted = fPrintingStarted;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::GetDidPrintingStart
//
//  Synopsis:
//      Returns whether printing has started or not.
//
//------------------------------------------------------------------------
_Check_return_ bool 
CCommonPrintingData::GetDidPrintingStart()
{
    return m_fPrintingStarted;
}


//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetScaleX
//
//  Synopsis:
//      Sets ScaleX.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetScaleX(XFLOAT fScaleX)
{
    HRESULT hr = S_OK;

    IFCEXPECT_ASSERT(fScaleX > 0.0);

    m_fScaleX = fScaleX;

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::GetScaleX
//
//  Synopsis:
//      Returns ScaleX.
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT 
CCommonPrintingData::GetScaleX()
{
    return m_fScaleX;
}


//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetScaleY
//
//  Synopsis:
//      Sets ScaleY.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetScaleY(XFLOAT fScaleY)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(fScaleY > 0.0);

    m_fScaleY = fScaleY;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::GetScaleY
//
//  Synopsis:
//      Returns ScaleY.
//
//------------------------------------------------------------------------
_Check_return_ XFLOAT 
CCommonPrintingData::GetScaleY()
{
    return m_fScaleY;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetPageRangeFrom
//
//  Synopsis:
//      Sets page range's from.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetPageRangeFrom(XINT32 nPageRangeFrom)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPageRangeFrom > 0);

    m_nPageRangeFrom = nPageRangeFrom;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::GetPageRangeFrom
//
//  Synopsis:
//      Returns page range's from.
//
//------------------------------------------------------------------------
_Check_return_ XINT32 
CCommonPrintingData::GetPageRangeFrom()
{
    return m_nPageRangeFrom;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetPageRangeTo
//
//  Synopsis:
//      Sets page range's to.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetPageRangeTo(XINT32 nPageRangeTo)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPageRangeTo > 0);

    m_nPageRangeTo = nPageRangeTo;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::GetPageRangeTo
//
//  Synopsis:
//      Returns page range's to.
//
//------------------------------------------------------------------------
_Check_return_ XINT32 
CCommonPrintingData::GetPageRangeTo()
{
    return m_nPageRangeTo;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::SetPrintableAreaWidth
//
//  Synopsis:
//      Sets printable area width.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetPrintableAreaWidth(XUINT32 nPrintableAreaWidth)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPrintableAreaWidth > 0);

    m_nPrintableAreaWidth = nPrintableAreaWidth;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Returns printable area width.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 
CCommonPrintingData::GetPrintableAreaWidth()
{
    return m_nPrintableAreaWidth;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Sets printable area height.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetPrintableAreaHeight(XUINT32 nPrintableAreaHeight)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(nPrintableAreaHeight > 0);

    m_nPrintableAreaHeight = nPrintableAreaHeight;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Returns printable area height.
//
//------------------------------------------------------------------------
_Check_return_ XUINT32 
CCommonPrintingData::GetPrintableAreaHeight()
{
    return m_nPrintableAreaHeight;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Sets page margins.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetPageMargins(XTHICKNESS tPageMargins)
{
    HRESULT hr = S_OK;
    IFCEXPECT_ASSERT(tPageMargins.left >= 0);
    IFCEXPECT_ASSERT(tPageMargins.top >= 0);
    IFCEXPECT_ASSERT(tPageMargins.right >= 0);
    IFCEXPECT_ASSERT(tPageMargins.bottom >= 0);

    m_tPageMargins = tPageMargins;
Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Returns page margins.
//
//------------------------------------------------------------------------
_Check_return_ XTHICKNESS 
CCommonPrintingData::GetPageMargins()
{
    return m_tPageMargins;
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Sets document name.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCommonPrintingData::SetDocumentName(
                                  _In_ const XINT32  cLength,
                                  _In_reads_((cLength+1)) const WCHAR *pDocumentName)
{
    HRESULT hr = S_OK;
    IPlatformUtilities *pUtilities;

    IFCEXPECT_ASSERT(!m_pszDocumentName);

    IFC(GetPALCoreServices()->GetPlatformUtilities(&pUtilities));
    
    m_pszDocumentName = pUtilities->Xstralloc((WCHAR *) pDocumentName, cLength);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CCommonPrintingData::
//
//  Synopsis:
//      Returns document name.
//
//------------------------------------------------------------------------
_Check_return_ WCHAR* 
CCommonPrintingData::GetDocumentName()
{
    return m_pszDocumentName;
}

_Check_return_ bool CCommonPrintingData::GetSupportsPostscript() 
{ 
    return m_fSupportsPostscript; 
}   

void CCommonPrintingData::SetSupportsPostscript(bool value)
{ 
    m_fSupportsPostscript = value; 
}   

bool CCommonPrintingData::GetIsPrintFormatPostscript()
{
    return m_fIsPrintFormatPostscript;
}

void CCommonPrintingData::SetPrintFormatToPostscript(bool value)
{
    m_fIsPrintFormatPostscript = value;
}

