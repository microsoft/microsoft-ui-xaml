// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "palfontandscriptservices.h"
#include <RootScale.h>

_Check_return_ HRESULT CPrintDocument::GetPrintFactoryAndTarget(
    _In_ CCoreServices *pCore,
    _Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory,
    _Outptr_ IPALPrintTarget **ppPrintTarget
    )
{
    wrl::ComPtr<IPALPrintTarget> pPrintTarget;

    // Create D2D print target
    wrl::ComPtr<IPALAcceleratedGraphicsFactory> pPrintFactory;
    IFC_RETURN(gps->CreateD2DPrintFactoryAndTarget(
        &pPrintFactory,
        &pPrintTarget));

    *ppPrintFactory = pPrintFactory.Detach();
    *ppPrintTarget = pPrintTarget.Detach();

    return S_OK;
}


_Check_return_ HRESULT CPrintDocument::PrintSinglePageVectorFallback(
    _In_ CUIElement* pPageVisual
    )
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the print target for the preview phase.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::BeginPreview(_In_ void* pPreviewPackageTarget)
{
    // Recreate the printing data for the new preview phase.
    ReleaseInterface(m_pD2DPrintingData);
    IFC_RETURN(gps->CreateD2DPrintingData(
        &m_pD2DPrintingData));
    IFC_RETURN(m_pD2DPrintingData->SetPreviewPackageTarget(pPreviewPackageTarget));
    m_fForceVector = TRUE;
    m_fPreviewStage = TRUE;

    if (!m_pPrintTarget)
    {
        // Create D2D print target and factory
        IFC_RETURN(GetPrintFactoryAndTarget(
            GetContext(),
            &m_pPrintFactory,
            &m_pPrintTarget));
    }
    IFC_RETURN(m_pPrintTarget->BeginPreview(m_pD2DPrintingData));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called when the user presses the print button and the preview pane is dismissed.
//
//------------------------------------------------------------------------
void CPrintDocument::EndPreview()
{
    m_fPreviewStage = FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raises PrintDocument.Paginate event to inform the application that a change in
//      document settings requires it to re-paginate its printing content.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::Paginate(
    _In_opt_ void* pDocumentSettings)
{
    // Do not process if we are not in the preview state. Due to asynchronous communication
    // with PrintManager and the application, there could be pending preview events/calls
    // even after the user has hit the preview button and dismissed the preview pane.
    if (m_fPreviewStage)
    {
        IFCEXPECT_RETURN(m_pD2DPrintingData);
        IFC_RETURN(m_pD2DPrintingData->SetDocumentSettings(pDocumentSettings));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Raises PrintDocument.GetPreviewPage event to request a preview content for
//      a page from the application.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::GetPreviewPage(
    XUINT32 desiredJobPage,
    XFLOAT width,
    XFLOAT height)
{
    XSIZEF previewPaneSize = { width, height };

    // Do not process if we are not in the preview state. Due to asynchronous communication
    // with PrintManager and the application, there could be pending preview events/calls
    // even after the user has hit the preview button and dismissed the preview pane.
    if (m_fPreviewStage)
    {
        IFCEXPECT_RETURN(m_pD2DPrintingData);
        IFC_RETURN(m_pD2DPrintingData->SetPreviewPaneSize(desiredJobPage, previewPaneSize));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by application to update the preview page count.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::SetPreviewPageCount(
    _In_ XINT32 count,
    _In_ DirectUI::PreviewPageCountType type)
{
    PALPreviewPageCountType palPageCountType;

    // Do not process if we are not in the preview state. Due to asynchronous communication
    // with PrintManager and the application, there could be pending preview events/calls
    // even after the user has hit the preview button and dismissed the preview pane.
    if (m_fPreviewStage)
    {
        ASSERT(type == DirectUI::PreviewPageCountType::Final || type == DirectUI::PreviewPageCountType::Intermediate);
        palPageCountType = type == DirectUI::PreviewPageCountType::Final ? PALPreviewPageCountType::Final : PALPreviewPageCountType::Intermediate;

        IFCEXPECT_RETURN(m_pPrintTarget);
        IFC_RETURN(m_pPrintTarget->SetPreviewPageCount(palPageCountType, count));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by application to provide visual for print preview for a page.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::SetPreviewPage(
    _In_ XINT32 pageNumber,
    _In_ CUIElement* pageVisual)
{
    // Do not process if we are not in the preview state. Due to asynchronous communication
    // with PrintManager and the application, there could be pending preview events/calls
    // even after the user has hit the preview button and dismissed the preview pane.
    if (m_fPreviewStage)
    {
        IFCEXPECT_RETURN(m_pD2DPrintingData);
        m_pD2DPrintingData->SetDPIScale(RootScale::GetRasterizationScaleForElement(pageVisual));
        IFC_RETURN(LayoutAndPrintSinglePage(pageNumber, pageVisual));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls InvalidatePreview on the print target to invalidate the preview cache.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPrintDocument::InvalidatePreview()
{
    // Do not process if we are not in the preview state. Due to asynchronous communication
    // with PrintManager and the application, there could be pending preview events/calls
    // even after the user has hit the preview button and dismissed the preview pane.
    if (m_fPreviewStage)
    {
        IFCEXPECT_RETURN(m_pPrintTarget);
        IFC_RETURN(m_pPrintTarget->InvalidatePreview());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Raises the PrintDocument.AddPages event to inform the application that it is now
//      time to print the document to the printer. In response to this event, the application
//      should call PrintDocument.AddPages method for each page.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::MakeDocument(
    _In_ void* pDocPackageTarget,
    _In_opt_ void* pDocSettings)
{
    if (!m_pD2DPrintingData)
    {
        // Create printing data
        IFC_RETURN(gps->CreateD2DPrintingData(
            &m_pD2DPrintingData));
    }
    IFC_RETURN(m_pD2DPrintingData->SetDocumentPackageTarget(pDocPackageTarget));
    IFC_RETURN(m_pD2DPrintingData->SetDocumentSettings(pDocSettings));
    m_fForceVector = TRUE;

    if (m_pPrintTarget)
    {
        // We have an existing print target created for preview. End the preview phase.
        IFC_RETURN(m_pPrintTarget->EndPreview());
    }
    else
    {
        // Create D2D print target and factory
        IFC_RETURN(GetPrintFactoryAndTarget(
            GetContext(),
            &m_pPrintFactory,
            &m_pPrintTarget));
    }
    IFC_RETURN(m_pPrintTarget->BeginPrint(m_pD2DPrintingData));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by application to add PageVisual for a page into the PrintDocument for printing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::AddPage(
    _In_ CUIElement* pageVisual)
{
    IFCEXPECT_RETURN(!m_fPreviewStage);
    // We do not need valid page numbers for final print.
    IFC_RETURN(LayoutAndPrintSinglePage(1, pageVisual));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by application to indicate it is done adding pages to the PrintDocument.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::AddPagesComplete()
{
    IFCEXPECT_RETURN(!m_fPreviewStage);
    IFC_RETURN(EndPrinting());

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: EndPrinting
//
//  Synopsis:
//      Ends a printing job and resets the state of the PrintDocument
//      object.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintDocument::EndPrinting()
{
    HRESULT hr = S_OK;

    if (m_pPrintTarget)
    {
        IFC(m_pPrintTarget->EndPrint());
    }

Cleanup:
    ReleaseInterface(m_pD2DPrintingData);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepares a page visual for printing by adding it to the print root and performing
//      layout. Hands it over to PrintSinglePage method for printing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CPrintDocument::LayoutAndPrintSinglePage(
    XINT32 pageNumber,
    _In_opt_ CUIElement* pPageVisual)
{
    HRESULT hr = S_OK;
    CPrintPageEventArgs *pPrintPageEventArgs = NULL;
    CPrintRoot *pPrintRoot = NULL;
    XSIZEF pageSize = {0,0};

    // Printing Data could be NULL if application calls AddPage when it is not supposed to.
    ASSERT(m_pD2DPrintingData);
    if (m_pD2DPrintingData && pPageVisual)
    {
        auto core = GetContext();

        // We do not need valid page numbers for final print.
        IFC(m_pD2DPrintingData->GetPageSize(pageNumber, pageSize));

        IFCPTR(pPrintRoot = core->GetPrintRoot());
        IFC(pPrintRoot->SetSize(pageSize));

        // Setting as PageVisual for PrintPageEventArgs adds it to the print root,
        // enters it in the live tree and does layout.
        pPrintPageEventArgs = new CPrintPageEventArgs(core);

        IFC(pPrintPageEventArgs->put_PageVisual(pPageVisual));

        hr = PrintSinglePage(pageNumber, pPageVisual);
        // Recreate the print target if we get a device lost.
        if (hr == D2DERR_RECREATE_TARGET)
        {
            // A valid document package target indicates we were
            // in the middle of print operation when device lost happened.
            bool fIsPrint = (m_pD2DPrintingData->GetDocumentPackageTargetNoRef() != NULL);

            // Device lost in the middle of printing is a non-recoverable operation.
            if (fIsPrint)
            {
                IFC(m_pPrintTarget->EndPrint());
                IFC(E_FAIL);
            }

            // We can recover from a device lost in preview stage. Re-generate the
            // D2D render target and factory and resume the last preview operation.
            IFC(m_pPrintTarget->EndPreview());

            ReleaseInterface(m_pPrintFactory);
            ReleaseInterface(m_pPrintTarget);

            IFC(GetPrintFactoryAndTarget(
                GetContext(),
                &m_pPrintFactory,
                &m_pPrintTarget));

            IFC(m_pPrintTarget->BeginPreview(m_pD2DPrintingData));
            // Print the page with the new render target.
            IFC(PrintSinglePage(pageNumber, pPageVisual));
        }
        IFC(hr);
    }

Cleanup:
    if (pPrintPageEventArgs)
    {
        IGNOREHR(pPrintPageEventArgs->ClearPageVisual());
    }
    ReleaseInterface(pPrintPageEventArgs);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: PrintSinglePage
//
//  Synopsis:
//      This is where the actual printing is done.
//      The method first makes sure that the element to be printed
//      is ready. If not, it kicks off the retrying mechanism, resuing
//      the same PrintPageEventArgs (that's holding onto the PageVisual)
//      If the element is ready, it prints it, and informs the caller
//      by setting pfPagePrinted to TRUE.
//
//  Parameters:
//      pPrintPageEventArgs: The PrintPageEventArgs object that holds a
//                           reference to PageVisual and whether the job
///                          HasMorePages or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPrintDocument::PrintSinglePage(
    XINT32 pageNumber,
    _In_ CUIElement* pPageVisual
    )
{
    HRESULT hr          = S_OK;
    CPopupRoot* pPopupRoot = NULL;

    IFCEXPECT_ASSERT(pPageVisual);

    if (m_pPrintTarget)
    {
        auto core = GetContext();
        D2DPrecomputeParams cp(m_pPrintFactory, NULL);
        D2DRenderParams printParams(m_pPrintTarget, NULL, TRUE);
        printParams.m_fForceVector = m_fForceVector;
        SharedRenderParams sharedParams;
        CMILMatrix rootOffsetTransform;

        IFC(core->GetAdjustedPopupRootForElement(pPageVisual, &pPopupRoot));

        core->SetIsPrinting(TRUE);
        IFC(m_pPrintTarget->BeginPage(pageNumber));

        // The print root should not take into consideration its offset with the parent layout container
        // while printing. The root print visual, thus, always prints at top-left of the page.
        rootOffsetTransform.SetToIdentity();
        rootOffsetTransform.SetDx(-1*pPageVisual->GetActualOffsetX());
        rootOffsetTransform.SetDy(-1*pPageVisual->GetActualOffsetY());
        IFC(m_pPrintTarget->SetTransform(&rootOffsetTransform));
        sharedParams.pCurrentTransform = &rootOffsetTransform;

        // Print the print visual.
        hr = pPageVisual->Print(sharedParams, cp, printParams);
        if (hr != AgError(AG_E_NO_VECTOR_PRINT))
        {
            IFC(hr);

            if (pPopupRoot)
            {
                // Print any popups that are part of the print visual tree.
                hr = pPopupRoot->Print(sharedParams, cp, printParams);
            }
        }

        if (hr == AgError(AG_E_NO_VECTOR_PRINT))
        {
            // One of the UI primitives on the page could not be printed with vector printing.
            // Attempt a fallback approach.
            IFC(m_pPrintTarget->CancelPage());
            IFC(m_pPrintTarget->BeginPage(pageNumber));
            IFC(PrintSinglePageVectorFallback(pPageVisual));
        }
        IFC(hr);

        // Revert the global transform to page top-left. If we don't do this, the
        // subsequent page will pick up the residual transform from here.
        rootOffsetTransform.SetToIdentity();
        IFC(m_pPrintTarget->SetTransform(&rootOffsetTransform));

        IFC(m_pPrintTarget->EndPage());
    }
    else
    {
        ASSERT(FALSE);
    }

    m_nPrintedPageCount++;

Cleanup:
    if (FAILED(hr) && pPopupRoot)
    {
        // Clear out all the print dirty flags on popup so that the popups
        // don't print as part of the next page.
        IGNOREHR(pPopupRoot->ClearPrintDirtyFlagOnOpenPopups());
    }
    GetContext()->SetIsPrinting(FALSE);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: InitializeBitmapPrinting
//
//  Synopsis:
//      Creates the WriteableBitmap object used for printing, a white
//      background rectangle (by default, WB has a black background).
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintDocument::InitializeBitmapPrinting()
{
    XUINT32 nLength = 0;
    auto core = GetContext();
    CREATEPARAMETERS cp(core);
    DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strWhite, L"White");
    CREATEPARAMETERS cpBrush(core, c_strWhite);
    CValue val;

    IFC_RETURN(UInt32Mult(m_pPD->GetPrintableAreaWidth(), m_pPD->GetPrintableAreaHeight(), &nLength));

    m_pPixels = new XINT32[nLength];

    // Create the WriteableBitmap to be used for printing.
    IFC_RETURN(CreateDO(&m_pWriteableBitmap, &cp));
    IFC_RETURN(m_pWriteableBitmap->Create((void *)m_pPixels, m_pPD->GetPrintableAreaWidth(), m_pPD->GetPrintableAreaHeight()));

    // Scale transform to be used for rendering the PageVisual
    IFC_RETURN(CreateDO(&m_pScaleTransform, &cp));

    // Properly initialize the scale transform
    m_pScaleTransform->m_eScaleX = m_pPD->GetScaleX();
    m_pScaleTransform->m_eScaleY = m_pPD->GetScaleY();

    // Final transform = RenderTransform (on element) + ScaleTransform (printing)
    IFC_RETURN(CreateDO(&m_pFinalTransform, &cp));
    IFC_RETURN(CreateDO(&(m_pFinalTransform->m_pMatrix), &cp));

    // Create the White brush.
    IFC_RETURN(CreateDO(&m_pWhiteBrush, &cpBrush));

    // Create the White rect. By default, WriteableBitmap has a black background
    // which is not desired while printing.
    // This Rectangle will be used as a white background for printing.
    IFC_RETURN(CreateDO(&m_pWhiteRect, &cp));

    // Create a dummy transform to be used for rendering the background rectangle.
    IFC_RETURN(CreateDO(&m_pDummyTransform, &cp));

    // Properly initialize the rectangle
    IFC_RETURN(m_pWhiteRect->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, m_pWhiteBrush));
    val.SetFloat(static_cast<XFLOAT>(m_pPD->GetPrintableAreaWidth()));
    IFC_RETURN(m_pWhiteRect->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
    val.SetFloat(static_cast<XFLOAT>(m_pPD->GetPrintableAreaHeight()));
    IFC_RETURN(m_pWhiteRect->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, val));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: UnregisterEvents
//
//  Synopsis:
//      Usually, Events get removed when an element "Leave"s the Live tree.
//      Since PrintDocument doesn't Enter the live tree, events has to be
//      removed from the Event Manager manually. That's what this method does.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintDocument::UnregisterEvents()
{
    CEventManager *pEventManager = GetContext()->GetEventManager();

    IFCPTR_RETURN(pEventManager);

    if (m_pEventList)
    {
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            IFC_RETURN( pEventManager->RemoveRequest(this, pRequest));
            pTemp = pTemp->m_pNext;
        }

        m_pEventList->Clean();
        delete m_pEventList;
        m_pEventList = NULL;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: AddEventListener
//
//  Synopsis:
//      Override to base AddEventListener
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintDocument::AddEventListener(
                                 _In_ EventHandle hEvent,
                                 _In_ CValue *pValue,
                                 _In_ XINT32 iListenerType,
                                 _Out_opt_ CValue *pResult,
                                 _In_ bool fHandledEventsToo)
{
    RRETURN(CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo));
}

//------------------------------------------------------------------------
//
//  Method: RemoveEventListener
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CPrintDocument::RemoveEventListener(
                                    _In_ EventHandle hEvent,
                                    _In_ CValue *pValue)
{
    RRETURN(CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      A helper method to raise various printing events.
//
//  Parameters:
//      nIndex      : The id of the event to fire.
//      ppArgs      : Pointer-to-Pointer EventArgs to be sent to the event handler.
//      fTimeBased  : Whether the event is time based or not.
//      rDuration   : If it's time based, that's the duration after which
//                    the event should be fired.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CPrintDocument::RaisePrintingEvent(
    _In_ EventHandle hEvent,
    _In_ CEventArgs **ppArgs
    )
{
    CEventManager       *pEventManager  = NULL;

    // All printing events use EventArgs
    IFCEXPECT_ASSERT_RETURN(ppArgs && *ppArgs);

    // Get the event manager.
    pEventManager = GetContext()->GetEventManager();
    IFCPTR_RETURN(pEventManager);

    // Raise the event.
    pEventManager->Raise(hEvent, TRUE, this, *ppArgs);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: ~CPrintDocument
//
//  Synopsis:
//      Class destructor.
//
//------------------------------------------------------------------------
CPrintDocument::~CPrintDocument()
{
    Reset();
    IGNOREHR(UnregisterEvents());
}

//------------------------------------------------------------------------
//
//  Method: Reset
//
//  Synopsis:
//      A helper method to reset the state of the PrintDocument object.
//
//------------------------------------------------------------------------
void CPrintDocument::Reset()
{
    ReleaseInterface(m_pPD);
    ReleaseInterface(m_pD2DPrintingData);
    ReleaseInterface(m_pPrintFactory);
    ReleaseInterface(m_pPrintTarget);
    if(m_pPixels)
    {
        delete[] m_pPixels;
        m_pPixels = NULL;
    }

    ReleaseInterface(m_pFinalTransform);
    ReleaseInterface(m_pScaleTransform);
    ReleaseInterface(m_pWriteableBitmap);

    ReleaseInterface(m_pDummyTransform);
    ReleaseInterface(m_pWhiteRect);
    ReleaseInterface(m_pWhiteBrush);

    m_nPrintedPageCount = 0;
    m_iErrorCode        = 0;
}

