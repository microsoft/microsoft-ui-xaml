// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// EventArgs used for printing.
// Used by the PrintPage event to enable app authors to print a
// UIElement by assigning it to PageVisual and whether there are
// more pages to print or not by setting HasMorePages among other
// things.

#include "precomp.h"

_Check_return_ HRESULT CPrintPageEventArgs::get_PageVisual(_Outptr_ CUIElement** ppPageVisual)
{
    ReplaceInterface(*ppPageVisual, m_pPageVisual);
    RRETURN(S_OK);
}


//-------------------------------------------------------------------------
//
//  Function:   CPrintPageEventArgs::SetPageVisual()
//
//  Synopsis:
//      Sets a new PageVisual.
//      First, clears the existing PageVisual, if any. Then, sets the new
//      one. Lastly, Attach it to the Print Root, if it's not in the Live
//      tree to allow for proper rendering to WriteableBitmap.
//
//  Parameters:
//      pPageVisual : The PageVisual to set.
//
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintPageEventArgs::put_PageVisual(_In_ CUIElement* pPageVisual)
{
    CPrintRoot *pPrintRoot = NULL;

    IFC_RETURN(ClearPageVisual());

    SetInterface(m_pPageVisual, pPageVisual);

    if (!m_pPageVisual->IsActive())
    {
        // Make sure that the incoming object is not currently associated.
        if (pPageVisual->IsAssociated())
        {
            // We cannot set a new value that is already associated to another element
            IFC_RETURN(pPageVisual->SetAndOriginateError(E_NER_INVALID_OPERATION, ManagedError, AG_E_MANAGED_ELEMENT_ASSOCIATED));
        }

        m_pPageVisual->PegManagedPeerNoRef();

        pPrintRoot = m_pCore->GetPrintRoot();

        IFCPTR_RETURN(pPrintRoot);

        IFC_RETURN(pPrintRoot->AddChild(m_pPageVisual));

        IFC_RETURN(m_pPageVisual->UpdateLayout());
    }


    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   CPrintPageEventArgs::ClearPageVisual()
//
//  Synopsis:
//      Clears the PageVisual, if any.
//      First, detach the existing PageVisual from the Print Root (if it's
//      attached), and then clears the existing PageVisual.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT CPrintPageEventArgs::ClearPageVisual()
{
    HRESULT hr = S_OK;
    CUIElement  *pPageVisual = NULL;
    CPrintRoot  *pPrintRoot  = m_pCore->GetPrintRoot();

    IFCPTR(pPrintRoot);

    pPageVisual = pPrintRoot->GetFirstChildNoAddRef();

    // Remove the PageVisual (unless it was already cleared or never set).
    if(m_pPageVisual && pPageVisual == m_pPageVisual)
    {
        IFC(pPrintRoot->RemoveChild(pPageVisual));

        LeaveParams leaveParams(
            /*fIsLive*/               TRUE,
            /*fSkipNameRegistration*/ FALSE,
            /*fCoercedIsEnabled*/     TRUE,
            /*fVisualTreeBeingReset*/ TRUE
        );

        pPageVisual->UnpegManagedPeerNoRef();
    }


Cleanup:
    ReleaseInterface(m_pPageVisual);
    m_pPageVisual = NULL;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CPrintPageEventArgs::SetPrintableArea()
//
//  Synopsis:
//      Sets the printable area size of the current print job.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintPageEventArgs::SetPrintableArea(
                                      _In_ XFLOAT fWidth,
                                      _In_ XFLOAT fHeight)
{
    CPrintRoot  *pPrintRoot  = m_pCore->GetPrintRoot();

    IFCPTR_RETURN(pPrintRoot);
    if (fWidth <= 0 || fHeight <= 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    m_sPrintableArea.width  = (XFLOAT)((XINT32)fWidth);
    m_sPrintableArea.height = (XFLOAT)((XINT32)fHeight);

    IFC_RETURN(pPrintRoot->SetSize(m_sPrintableArea));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CPrintPageEventArgs::SetPageMargins()
//
//  Synopsis:
//      Sets the page margins of the current print job.
//
//-------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintPageEventArgs::SetPageMargins(_In_ XTHICKNESS pageMargins)
{
    HRESULT hr = S_OK;

    m_tPageMargins.left     = (XFLOAT) XcpRound(pageMargins.left);
    m_tPageMargins.top      = (XFLOAT) XcpRound(pageMargins.top);
    m_tPageMargins.right    = (XFLOAT) XcpRound(pageMargins.right);
    m_tPageMargins.bottom   = (XFLOAT) XcpRound(pageMargins.bottom);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CPrintPageEventArgs::~CPrintPageEventArgs()
//
//  Synopsis:
//      Class Destructor.
//
//-------------------------------------------------------------------------
CPrintPageEventArgs::~CPrintPageEventArgs()
{
    IGNOREHR( ClearPageVisual() );
}
