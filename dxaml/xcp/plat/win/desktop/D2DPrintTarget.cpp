// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "d2dacceleratedbrushes.h"
#include "d2dacceleratedprimitives.h"

#define XCPPRINT_CLASS L"XCPPrintClass"

const UINT CD2DPrintTarget::WM_D2DPRINT_INITIALIZE = WM_USER + 1;
const UINT CD2DPrintTarget::WM_D2DPRINT_BEGINPRINT = WM_USER + 2;
const UINT CD2DPrintTarget::WM_D2DPRINT_PRINTPAGE  = WM_USER + 3;
const UINT CD2DPrintTarget::WM_D2DPRINT_ENDPRINT   = WM_USER + 4;
const UINT CD2DPrintTarget::WM_D2DPRINT_DESTROY    = WM_USER + 5;


//------------------------------------------------------------------------
//
//  Synopsis:
//      Static to create a CD2DPrintTarget.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::Create(
    _In_ CD2DFactory* pFactory,
    _Outptr_ CD2DPrintTarget** ppPALPrintTarget
    )
{
    HRESULT hr = S_OK;
    CD2DPrintTarget* pNewTarget = NULL;

    pNewTarget = new CD2DPrintTarget();
    IFC(pNewTarget->Initialize(pFactory));
    SetInterface(*ppPALPrintTarget, pNewTarget);

Cleanup:
    ReleaseInterface(pNewTarget);
    RRETURN(hr);
}

_Check_return_ HRESULT CD2DPrintTarget::Initialize(
    _In_ CD2DFactory* pFactory)
{
    HRESULT hr = S_OK;
    IPALWaitable* pPrintThread = NULL;
    CD2DPrintTarget* pThis = NULL;

    // Set the D2D factory
    SetInterface(m_pFactory, pFactory);
    // Create the internal message queue
    IFC(gps->QueueCreate(&m_pPrintQueue));
    // Create the print thread
    IFC(gps->ThreadCreate(
        &pPrintThread,
        PrintThreadFn,
        sizeof(IPALQueue*),
        reinterpret_cast<XUINT8*>(m_pPrintQueue)));

    // Initialize D2D resources.
    SetInterface(pThis, this);
    IFC(SendPrintMessage(WM_D2DPRINT_INITIALIZE, (WPARAM)pThis, NULL));
    // Ownership of pThis has been transferred to the printer thread
    // processing the message.
    pThis = NULL;

Cleanup:
    ReleaseInterface(pThis);
    if (pPrintThread)
    {
        IGNOREHR(pPrintThread->Close());
    }
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Print thread function. Listens for print messages on thread-safe queue.
//
//------------------------------------------------------------------------
XINT32 CD2DPrintTarget::PrintThreadFn(_In_reads_bytes_(sizeof(IPALQueue*)) XUINT8* pData)
{
    HRESULT hr = S_OK;
    IPALQueue* pPrintQueue = reinterpret_cast<IPALQueue*>(pData);
    PrintMessage* pMessage = NULL;
    bool fShouldCallCoUninitialize = false;

    IFC(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
    fShouldCallCoUninitialize = TRUE;

    while (TRUE)
    {
        IFC(pPrintQueue->Get((void**)&pMessage, INFINITE));
        // Dispatch the print message for processing
        ProcessPrintMessage(
            pMessage->msg,
            pMessage->wParam,
            pMessage->lParam);
        // Signal the notification event if any.
        if (pMessage->pSendMessageEvent)
        {
            pMessage->pSendMessageEvent->Set();
        }
        // Exit from the message loop on WM_D2DPRINT_DESTROY message.
        if (pMessage->msg == WM_D2DPRINT_DESTROY)
        {
            SAFE_DELETE(pMessage);
            break;
        }
        SAFE_DELETE(pMessage);
    }

Cleanup:
    if (fShouldCallCoUninitialize)
    {
        CoUninitialize();
    }
    return 0;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Processes print messages queued by the UI-thread.
//
//------------------------------------------------------------------------
void CD2DPrintTarget::ProcessPrintMessage(
    _In_ UINT   msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    CD2DPrintTarget* pPrintTarget = NULL;

    switch (msg)
    {
        case WM_D2DPRINT_INITIALIZE:
            pPrintTarget = reinterpret_cast<CD2DPrintTarget*>(wParam);
            IGNOREHR(pPrintTarget->InitializeImpl());
            break;
        case WM_D2DPRINT_BEGINPRINT:
            pPrintTarget = reinterpret_cast<CD2DPrintTarget*>(wParam);
            IGNOREHR(pPrintTarget->BeginPrintImpl(lParam));
            break;
        case WM_D2DPRINT_PRINTPAGE:
            pPrintTarget = reinterpret_cast<CD2DPrintTarget*>(wParam);
            IGNOREHR(pPrintTarget->PrintPageImpl(lParam));
            break;
        case WM_D2DPRINT_ENDPRINT:
            pPrintTarget = reinterpret_cast<CD2DPrintTarget*>(wParam);
            IGNOREHR(pPrintTarget->EndPrintImpl());
            break;
        case WM_D2DPRINT_DESTROY:
            IGNOREHR(reinterpret_cast<CD2DPrintTarget*>(wParam)->DestroyImpl());
            break;
        default:
            ASSERT(FALSE);
    }

    ReleaseInterface(pPrintTarget);
    return;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Simulates PostMessage for the print queue - method returns immediately
//      after the posting the message.
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::PostPrintMessage(
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HRESULT hr = S_OK;
    PrintMessage* pMessage = NULL;

    IFCPTR(m_pPrintQueue);
    pMessage = new PrintMessage(msg, wParam, lParam, NULL);
    m_pPrintQueue->Post(static_cast<void*>(pMessage));
    pMessage = NULL;

Cleanup:
    SAFE_DELETE(pMessage);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Simulates SendMessage for the print queue - method returns after the print
//      message has been processed by the printer thread
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::SendPrintMessage(
    _In_ UINT msg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam)
{
    HRESULT hr = S_OK;
    PrintMessage* pMessage = NULL;
    IPALEvent* pEvent = NULL;

    IFCPTR(m_pPrintQueue);
    IFC(gps->EventCreate(&pEvent, FALSE, FALSE));

    pMessage = new PrintMessage(msg, wParam, lParam, pEvent);
    m_pPrintQueue->Post(static_cast<void*>(pMessage));
    pMessage = NULL;

    // Wait for the event to signal that the printer thread has processed this message.
    IFCW32(gps->WaitForObjects(
        1,
        reinterpret_cast<IPALWaitable**>(&pEvent),
        TRUE,
        INFINITE) != WAIT_FAILED);

Cleanup:
    SAFE_DELETE(pMessage);
    if (pEvent)
    {
        IGNOREHR(pEvent->Close());
    }
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes D2D resources required for preview and printing.
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::InitializeImpl()
{
    HRESULT hr = S_OK;
    D3D_FEATURE_LEVEL featureLevelUnused;
    ID3D11DeviceContext* pD3D11DeviceContextUnused = NULL;
    ID3D11Device* pD3D11Device = NULL;
    ID3D10Multithread * pMultithread = NULL;
    IDXGIDevice* pDXGIDevice = NULL;
    ID2D1Device* pD2D1Device = NULL;
    ID2D1Factory1 *pD2D1Factory1 = NULL;
    ID2D1DeviceContext* pD2D1DeviceContext = NULL;

    // Create D3D11 device
    IFC(D3D11CreateDevice(
                NULL,
                D3D_DRIVER_TYPE_WARP,
                NULL,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                NULL,
                0,
                D3D11_SDK_VERSION,
                &pD3D11Device,
                &featureLevelUnused,
                &pD3D11DeviceContextUnused
                ));
    // Mark the device for multithreading.
    IFC(pD3D11Device->QueryInterface(__uuidof(ID3D10Multithread), (void**)&pMultithread))
    pMultithread->SetMultithreadProtected(TRUE);

    // Create a D2D device and a D2DDeviceContext
    IFC(pD3D11Device->QueryInterface(&pDXGIDevice));
    IFC(m_pFactory->GetFactory()->QueryInterface(__uuidof(ID2D1Factory1), reinterpret_cast<void**>(&pD2D1Factory1)));
    IFC(pD2D1Factory1->CreateDevice(pDXGIDevice, &pD2D1Device));
    IFC(pD2D1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &pD2D1DeviceContext));

    SetInterface(m_pD2DDeviceContext, pD2D1DeviceContext);
    SetInterface(m_pD2D1Device, pD2D1Device);
    SetInterface(m_pD3DDevice, pD3D11Device);

Cleanup:
    ReleaseInterface(pD3D11Device);
    ReleaseInterface(pMultithread);
    ReleaseInterface(pDXGIDevice);
    ReleaseInterface(pD2D1Factory1);
    ReleaseInterface(pD2D1Device);
    ReleaseInterface(pD2D1DeviceContext);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Destroys the resources associated with printing - D2D resources, print control
//      and the hidden window.
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::DestroyImpl()
{
    // A valid print control means we are destroying the print target
    // without completing print operation. We should close and
    // destroy the print control properly.
    if (m_pPrintControl)
    {
        IGNOREHR(m_pPrintControl->Close());
        ReleaseInterface(m_pPrintControl);
    }

    ReleaseInterface(m_pD2D1Device);
    ReleaseInterface(m_pFactory);
    ReleaseInterface(m_pD3DDevice);
    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//    Sets up resources for printing to physical printer or file on disk.
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::BeginPrintImpl(LPARAM lParam)
{
    HRESULT hr = S_OK;
    IWICImagingFactory* pWicFactory = NULL;
    ID2D1PrintControl* pPrintControl = NULL;
    IPALD2DPrintingData* pPD = reinterpret_cast<IPALD2DPrintingData*>(lParam);

    IFCEXPECT(pPD);
    IFC(CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        __uuidof(IWICImagingFactory),
        (void**)&pWicFactory));
    IFC(m_pD2D1Device->CreatePrintControl(
        pWicFactory,                            // WIC imaging factory
        static_cast<IPrintDocumentPackageTarget*>(pPD->GetDocumentPackageTargetNoRef()),   // IDocumentPackageTarget representing the printer
        NULL,                                   // Optional print control properties
        &pPrintControl));                       // [Out] Print Control
    SetInterface(m_pPrintControl, pPrintControl);

Cleanup:
    ReleaseInterface(pPrintControl);
    ReleaseInterface(pWicFactory);
    ReleaseInterface(pPD);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//    Sends the current page data to the print control
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::PrintPageImpl(LPARAM lParam)
{
    HRESULT hr = S_OK;
    ID2D1CommandList* pCommandList = reinterpret_cast<ID2D1CommandList*>(lParam);

    IFCEXPECT(pCommandList && m_pPrintControl);
    IFC(m_pPrintControl->AddPage(
        pCommandList,       // ID2D1CommandList D2D command list to print
        m_pageSize,         // D2D_SIZE_F page Size
        NULL                // _In_opt_ IStream *pagePrintTicketStream
        ));

Cleanup:
    ReleaseInterface(pCommandList);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//    Closes the print control to mark end of print job
//
//------------------------------------------------------------------------
HRESULT CD2DPrintTarget::EndPrintImpl()
{
    HRESULT hr = S_OK;

    if (m_pPrintControl)
    {
        IFC(m_pPrintControl->Close());
    }

Cleanup:
    ReleaseInterface(m_pPrintControl);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the start of preview stage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::BeginPreview(_In_ IPALPrintingData* pPreviewData)
{
    if (m_fPrintStage)
    {
        // If there was a failure on a previous attempt, end printing.
        EndPrint();
    }
    ReplaceInterface(m_pPreviewData, reinterpret_cast<IPALD2DPrintingData*>(pPreviewData));
    m_fPreviewStage = TRUE;

    return S_OK;
}

_Check_return_ HRESULT CD2DPrintTarget::InvalidatePreview()
{
    HRESULT hr = S_OK;
    IPrintPreviewDxgiPackageTarget* pPreviewTarget = NULL;

    if (m_fPreviewStage)
    {
        IFCPTR(m_pPreviewData);
        IFCPTR(pPreviewTarget = static_cast<IPrintPreviewDxgiPackageTarget*>(m_pPreviewData->GetPreviewPackageTargetNoRef()));
        hr = pPreviewTarget->InvalidatePreview();
        // IPrintPreviewDxgiPackageTarget::InvalidatePreview will return RPC_E_DISCONNECTED after preview stage is closed.
        // This will happen, for example, if the user pressed Print before the Preview finished loading. If this happens
        // we want to simply ignore the error to avoid a needless app crash.
        if (hr == RPC_E_DISCONNECTED)
        {
            hr = S_OK;
        }
        else
        {
            IFC(hr);
        }
    }

Cleanup:
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates page count in preview UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::SetPreviewPageCount(PALPreviewPageCountType pageCountType, XINT32 pageCount)
{
    HRESULT hr = S_OK;
    IPrintPreviewDxgiPackageTarget* pPreviewTarget = NULL;
    PageCountType type;

    if (m_fPreviewStage)
    {
        IFCPTR(m_pPreviewData);
        IFCPTR(pPreviewTarget = static_cast<IPrintPreviewDxgiPackageTarget*>(m_pPreviewData->GetPreviewPackageTargetNoRef()));

        ASSERT(pageCountType == PALPreviewPageCountType::Final || pageCountType == PALPreviewPageCountType::Intermediate);
        type = pageCountType == PALPreviewPageCountType::Final ? PageCountType::FinalPageCount : PageCountType::IntermediatePageCount;

        hr = pPreviewTarget->SetJobPageCount(type, pageCount);
        // IPrintPreviewDxgiPackageTarget::InvalidatePreview will return RPC_E_DISCONNECTED after preview stage is closed.
        // This will happen, for example, if the user pressed Print before the Preview finished loading. If this happens
        // we want to simply ignore the error to avoid a needless app crash.
        if (hr == RPC_E_DISCONNECTED)
        {
            hr = S_OK;
        }
        else
        {
            IFC(hr);
        }
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the end of preview stage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::EndPreview()
{
    HRESULT hr = S_OK;

    IFCEXPECT(!m_fPrintStage);

Cleanup:
    // When it is in preview stage, end the preview stage
    if (m_fPreviewStage)
    {
        m_fPreviewStage = FALSE;
        ReleaseInterface(m_pPreviewData); // Release the preview data since we don't need it anymore
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the start of print stage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::BeginPrint(_In_ IPALPrintingData* pPD)
{
    HRESULT hr = S_OK;
    IPALD2DPrintingData* pD2DPD = NULL;
    CD2DPrintTarget* pThis = NULL;
    XSIZEF pageSize;

    IFCPTR(pPD);
    if (m_fPreviewStage && !m_fPrintStage)
    {
        // End preview in case this wasn't done by the caller.
        EndPreview();
    }
    IFCEXPECT(!m_fPreviewStage && !m_fPrintStage);
    m_fPrintStage = TRUE;

    // Set the page size for the print job. Just use the size for page 1 since all pages in a job have the same size.
    pD2DPD = reinterpret_cast<IPALD2DPrintingData*>(pPD);
    AddRefInterface(pD2DPD);
    IFC(pD2DPD->GetPageSize(1, pageSize));
    m_pageSize = *PALToD2DSizeF(&pageSize);

    SetInterface(pThis, this);
    IFC(PostPrintMessage(WM_D2DPRINT_BEGINPRINT, (WPARAM)pThis, (LPARAM)pD2DPD));
    // Ownership of pThis and pD2DPD has been transferred to the printer thread
    // processing the message.
    pThis = NULL;
    pD2DPD = NULL;

Cleanup:
    if (FAILED(hr))
    {
        m_fPrintStage = FALSE;
    }
    ReleaseInterface(pD2DPD);
    ReleaseInterface(pThis);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Marks the end of print stage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::EndPrint()
{
    HRESULT hr = S_OK;
    CD2DPrintTarget* pThis = NULL;

    IFCEXPECT(!m_fPreviewStage && m_fPrintStage);

    SetInterface(pThis, this);
    IFC(PostPrintMessage(WM_D2DPRINT_ENDPRINT, (WPARAM)pThis, NULL));
    // Ownership of pThis has been transferred to the printer thread
    // processing the message.
    pThis = NULL;

Cleanup:
    m_fPrintStage = FALSE;
    ReleaseInterface(pThis);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      New page within the print or preview workflow
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::BeginPage(XINT32 pageNumber)
{
    HRESULT hr = S_OK;
    IPALAcceleratedBrush* pBackgroundBrush = NULL;
    IPALAcceleratedGeometry* pBackgroundGeometry = NULL;
    XRECTF pageRect;
    XSIZEF pageSize;
    CMILMatrix identityMat(TRUE);

    IFCEXPECT(m_fPrintStage || m_fPreviewStage);
    if (m_fPreviewStage)
    {
        // Get the page size from print task options. We do not use the cached
        // value as page size can change while the MPD is up.
        IFCEXPECT(m_pPreviewData);
        IFC(m_pPreviewData->GetPageSize(pageNumber, pageSize));
        m_pageSize = *PALToD2DSizeF(&pageSize);
    }
    m_iPageNumber = pageNumber;

    // Create a new commandlist and set as the target of the device context.
    // We use command list for both preview and print stages. In preview,
    // the command list is drawn to a DXGI surface and passed to MPD. In print phase,
    // the command list is passed to the print control.
    //
    // Note we take reference for commandlist here, which could indirectly hold
    // references to other, possibly large (eg ID2D1Bitmap with pixel data for decoded image)
    // D2D resources. Make sure we release this here before creating new list, as well as in EndPage().
    ReleaseInterface(m_pCommandList);
    IFC(m_pD2DDeviceContext->CreateCommandList(&m_pCommandList));
    m_pD2DDeviceContext->SetTarget(m_pCommandList);

    // Clear out any existing transform and start drawing.
    m_pD2DDeviceContext->SetTransform(*PALToD2DMatrix(&identityMat)); // Method does not return a value
    m_pD2DDeviceContext->BeginDraw(); // Method does not return a value

    // Set a white background for page on which content will be drawn.
    pageRect.X = pageRect.Y = 0;
    pageRect.Width = m_pageSize.width;
    pageRect.Height = m_pageSize.height;
    IFC(m_pFactory->CreateRectangleGeometry(pageRect, &pBackgroundGeometry));
    IFC(CreateSolidColorBrush(0xFFFFFFFF, 1, &pBackgroundBrush));
    IFC(FillGeometry(pBackgroundGeometry, pBackgroundBrush, 1));

Cleanup:
    ReleaseInterface(pBackgroundGeometry);
    ReleaseInterface(pBackgroundBrush);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      End of current page. Send the data to printer if in printing stage.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::EndPage()
{
    HRESULT hr = S_OK;
    CD2DPrintTarget* pThis = NULL;
    IDXGISurface* pPreviewSurface = NULL;

    IFCEXPECT(m_fPrintStage || m_fPreviewStage);

    IFC(m_pD2DDeviceContext->EndDraw());
    m_pCommandList->Close();
    m_pD2DDeviceContext->SetTarget(NULL);

    if (m_fPreviewStage)
    {
        IPrintPreviewDxgiPackageTarget* pPreviewTarget = NULL;
        XSIZEF previewPaneSize;
        XSIZEF pageSize = { m_pageSize.width, m_pageSize.height };
        XFLOAT dpi = 96;

        IFCEXPECT(m_pPreviewData);
        // Get the size of the preview pane. This will be the size of the surface we create, command list
        // contents will be scaled to fit the preview pane. Use page size if preview panel size is not
        // available.
        hr = m_pPreviewData->GetPreviewPaneSize(m_iPageNumber, previewPaneSize);
        if (hr == S_FALSE)
        {
            // Do not scale up by the DPI scale. In most cases, pageSize is already bigger than
            // the preview pane and thus, we wouldn't gain anything by blowing it up even
            // further, rather impact perf by using significantly more memory.
            previewPaneSize.width = m_pageSize.width;
            previewPaneSize.height = m_pageSize.height;
        }
        else
        {
            // In immersive high dpi mode, m_pPreviewData->GetDPIScale() returns a scale of 1.4.
            // If we calculate the DPI using this scale, it comes out to 96*1.4 = 134.4. We need to pass
            // this DPI to the PrintManager via DrawPage call.
            XFLOAT dpiScale = m_pPreviewData->GetDPIScale();
            dpi *= dpiScale;
            previewPaneSize.width *= dpiScale;
            previewPaneSize.height *= dpiScale;
        }
        IFC(hr);

        IFC(CommandListToDXGISurface(
            m_pCommandList,
            m_pD3DDevice,
            m_pD2DDeviceContext,
            pageSize,
            previewPaneSize,
            &pPreviewSurface));
        IFCPTR(pPreviewTarget = static_cast<IPrintPreviewDxgiPackageTarget*>(m_pPreviewData->GetPreviewPackageTargetNoRef()));
        hr = pPreviewTarget->DrawPage(m_iPageNumber, pPreviewSurface, dpi, dpi);

        // IPrintPreviewDxgiPackageTarget::DrawPage will return RPC_E_DISCONNECTED
        // if an attempt is being made to provide preview images or set a page count after preview is closing down.
        // This will happen, for example, if the user pressed Print before the Preview finished loading. If this happens
        // we want to simply ignore the error to avoid a needless app crash.
        if (hr == RPC_E_DISCONNECTED)
        {
            hr = S_OK;
        }
        else
        {
            IFC(hr);
        }
    }
    else
    {
        SetInterface(pThis, this);
        IFC(PostPrintMessage(WM_D2DPRINT_PRINTPAGE, (WPARAM)pThis, (LPARAM)m_pCommandList));
        m_pCommandList = NULL;
        pThis = NULL;
    }

Cleanup:
    ReleaseInterface(m_pCommandList);
    ReleaseInterface(pThis);
    ReleaseInterface(pPreviewSurface);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Aborts the current page.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DPrintTarget::CancelPage()
{
    RRETURN(E_NOTIMPL);
}


_Check_return_ HRESULT CD2DPrintTarget::CommandListToDXGISurface(
    _In_ ID2D1CommandList* pCommandList,
    _In_ ID3D11Device* pD3D11Device,
    _In_ ID2D1DeviceContext* pD2DDeviceContext,
    _In_ XSIZEF commandListSize,
    _In_ XSIZEF surfaceSize,
    _Outptr_ IDXGISurface** ppSurface
    )
{
    HRESULT hr = S_OK;
    D3D11_TEXTURE2D_DESC texDesc;
    ID3D11Texture2D* pTexture = NULL;
    IDXGISurface* pSurface = NULL;
    ID2D1Bitmap1* pBitmap = NULL;
    ID2D1Effect* pD2D1ScaleEffect = NULL;
    ID2D1Image* pD2D1Image = NULL;
    D2D1_POINT_2F targetOffset = { 0, 0 };
    D2D1_VECTOR_2F scale = D2D1::Vector2F(surfaceSize.width/commandListSize.width, surfaceSize.height/commandListSize.height);
    D2D1_SCALE_INTERPOLATION_MODE interpolationMode = D2D1_SCALE_INTERPOLATION_MODE_HIGH_QUALITY_CUBIC;
    CMILMatrix identityMat(TRUE);

    texDesc.Width = static_cast<UINT>(surfaceSize.width);
    texDesc.Height = static_cast<UINT>(surfaceSize.height);
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    // Create scale effect to scale down the command list to preview pane size using
    // cubic interpolation method.
    IFC(pD2DDeviceContext->CreateEffect(CLSID_D2D1Scale, &pD2D1ScaleEffect));
    pD2D1ScaleEffect->SetInput(0, pCommandList);    // Set the input bitmap as the input to this effect
    IFC(pD2D1ScaleEffect->SetValue(D2D1_SCALE_PROP_SCALE, reinterpret_cast<const BYTE *>(&scale), sizeof(D2D1_VECTOR_2F))); // Set the scale factor.
    IFC(pD2D1ScaleEffect->SetValue(D2D1_SCALE_PROP_INTERPOLATION_MODE, reinterpret_cast<const BYTE *>(&interpolationMode), sizeof(D2D1_SCALE_INTERPOLATION_MODE))); // Set the interpolation mode.
    pD2D1ScaleEffect->GetOutput(&pD2D1Image); // get the scaled down image from the command list.

    IFC(pD3D11Device->CreateTexture2D(&texDesc, NULL, &pTexture));
    IFC(pTexture->QueryInterface(__uuidof(IDXGISurface), reinterpret_cast<void**>(&pSurface)));
    IFC(pD2DDeviceContext->CreateBitmapFromDxgiSurface(pSurface, NULL, &pBitmap));
    pD2DDeviceContext->SetTarget(pBitmap);

    // Draw the image into the device context. Output surface is set as the target of the device context.
    pD2DDeviceContext->BeginDraw();
    pD2DDeviceContext->SetTransform(*PALToD2DMatrix(&identityMat));     // Clear out any existing transform before drawing.
    pD2DDeviceContext->DrawImage(pD2D1Image, targetOffset);
    IFC(pD2DDeviceContext->EndDraw());

    SetInterface(*ppSurface, pSurface);

Cleanup:
    ReleaseInterface(pTexture);
    ReleaseInterface(pSurface);
    ReleaseInterface(pBitmap);
    ReleaseInterface(pD2D1ScaleEffect);
    ReleaseInterface(pD2D1Image);
    RRETURN(hr);
}

