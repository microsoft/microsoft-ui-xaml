// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PrintDocument.g.h"
#include "PaginateEventArgs.g.h"
#include "AddPagesEventArgs.g.h"
#include "GetPreviewPageEventArgs.g.h"

using namespace DirectUI;

PrintDocument::PrintDocument()
{
    m_pDocumentSource = NULL;
}

PrintDocument::~PrintDocument()
{
    ctl::release_interface<PrintDocumentSource>(m_pDocumentSource);
}

_Check_return_ HRESULT PrintDocumentSource::SetPrintDocument(
    _In_ PrintDocument* pPrintDocument
    )
{
    RRETURN(ctl::as_weakref(m_pPrintDocument, ctl::as_iinspectable(pPrintDocument)));
}

_Check_return_ HRESULT PrintDocument::get_DocumentSourceImpl(
    _Outptr_ wgr::Printing::IPrintDocumentSource** pValue
    )
{
    HRESULT hr = S_OK;

    IFCPTR(pValue);
    *pValue = NULL;

    if (!m_pDocumentSource)
    {
        IFC(ctl::ComObject<PrintDocumentSource>::CreateInstance(&m_pDocumentSource));
        IFC(m_pDocumentSource->SetPrintDocument(this));
    }
    ctl::addref_interface(m_pDocumentSource);
    *pValue = static_cast<wgr::Printing::IPrintDocumentSource*>(m_pDocumentSource);

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by PrintManager when the MPD comes up.
//
//------------------------------------------------------------------------
HRESULT PrintDocument::GetPreviewPageCollectionImpl(
    _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IPrintPreviewDxgiPackageTarget> spPreviewPackageTarget;

    IFCPTR(spContext);

    // Retrieve the preview document package target
    IFC(spContext->m_pPrintPackageTarget->GetPackageTarget(ID_PREVIEWPACKAGETARGET_DXGI, IID_PPV_ARGS(spPreviewPackageTarget.GetAddressOf())));
    IFC(hr = spPreviewPackageTarget.Get() ? S_OK : E_NOINTERFACE);

    // This callback marks the beginning of the preview phase.
    IFC(CoreImports::PrintDocument_BeginPreview(
        static_cast<CPrintDocument*>(GetHandle()),
        spPreviewPackageTarget.Get()));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Callback to indicate that a preview setting has changed that requires the application to
//     repaginate its printing content.
//
//------------------------------------------------------------------------
HRESULT PrintDocument::PaginateImpl(
    _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PaginateEventArgs> spPaginateEventArgs;
    PrintDocument::PaginateEventSourceType* pEventSource = nullptr;

    IFCPTR(spContext);

    IFC(CoreImports::PrintDocument_Paginate(
        static_cast<CPrintDocument*>(GetHandle()),
        spContext->m_pPrintTaskOptions));

    // Create and initialize PaginateEventArgs
    IFC(ctl::make(&spPaginateEventArgs));
    IFC(spPaginateEventArgs->put_PrintTaskOptions(spContext->m_pPrintTaskOptions));
    IFC(spPaginateEventArgs->put_CurrentPreviewPageNumber(spContext->m_currentJobPage));

    // Raise PrintDocument.Paginate event.
    IFC(GetPaginateEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spPaginateEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Callback to indicate that a preview setting has changed that requires the application to
//     repaginate its printing content.
//
//------------------------------------------------------------------------
HRESULT PrintDocument::MakePageImpl(
    _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<GetPreviewPageEventArgs> spGetPreviewPageEventArgs;
    PrintDocument::GetPreviewPageEventSourceType* pEventSource = nullptr;

    IFCPTR(spContext);

    // PrintManager requests the current preview page to display in MPD by sending
    // 0xFFFFFFFF as the desiredJobPage. This typically follows a Paginate or InvalidatePreview
    // as MPD does not know what page to show following a re-pagination. For WWA and Jupiter,
    // it has been decided to just switch the preview to page 1 so as to not over-complicate
    // the API. Advnaced printing clients such as Word can handle 0xFFFFFFFF more correctly.
    if (spContext->m_makePageDesiredJobPage == 0xFFFFFFFF)
    {
        spContext->m_makePageDesiredJobPage = 1;
    }
    IFC(CoreImports::PrintDocument_GetPreviewPage(
        static_cast<CPrintDocument*>(GetHandle()),
        spContext->m_makePageDesiredJobPage,
        spContext->m_makePageWidth,
        spContext->m_makePageHeight));

    IFC(ctl::make(&spGetPreviewPageEventArgs));
    IFC(spGetPreviewPageEventArgs->put_PageNumber(spContext->m_makePageDesiredJobPage));

    // Raise the PrintDocument.GetPreviewPage event to request print visuals from the application.
    IFC(GetGetPreviewPageEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spGetPreviewPageEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by PrintManager when the Print button on Modern Print Dialog is clicked.
//
//------------------------------------------------------------------------
HRESULT PrintDocument::MakeDocumentImpl(
    _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<AddPagesEventArgs> spAddPagesEventArgs;
    PrintDocument::AddPagesEventSourceType* pEventSource = nullptr;

    IFCPTR(spContext);

    IFC(CoreImports::PrintDocument_MakeDocument(
        static_cast<CPrintDocument*>(GetHandle()),
        spContext->m_pPrintPackageTarget,
        spContext->m_pPrintTaskOptions));

    IFC(ctl::make(&spAddPagesEventArgs));
    IFC(spAddPagesEventArgs->put_PrintTaskOptions(spContext->m_pPrintTaskOptions));

    // Raise the PrintDocument.AddPages event to tell the application that it is time to print.
    IFC(GetAddPagesEventSourceNoRef(&pEventSource));
    IFC(pEventSource->Raise(ctl::as_iinspectable(this), spAddPagesEventArgs.Get()));

Cleanup:
    RRETURN(hr);
}


PrintDocumentSource::PrintDocumentSource()
{
    m_pPrintDocument = NULL;
}

PrintDocumentSource::~PrintDocumentSource()
{
    ReleaseInterface(m_pPrintDocument);
}

HRESULT PrintDocumentSource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(IPrintDocumentPageSource)))
    {
        *ppObject = static_cast<IPrintDocumentPageSource*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(wgr::Printing::IPrintDocumentSource)))
    {
        *ppObject = static_cast<wgr::Printing::IPrintDocumentSource*>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(IPrintPreviewPageCollection)))
    {
        *ppObject = static_cast<IPrintPreviewPageCollection*>(this);
    }
    else
    {
        return ComBase::QueryInterfaceImpl(iid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by PrintManager when the MPD comes up.
//
//------------------------------------------------------------------------
IFACEMETHODIMP PrintDocumentSource::GetPreviewPageCollection(
    __RPC__in_opt IPrintDocumentPackageTarget *docPackageTarget,
    __RPC__deref_out_opt IPrintPreviewPageCollection **docPageCollection)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PrintDocumentInvokeContext> spContext = NULL;
    xaml::Printing::IPrintDocument* pPrintDocument = NULL;
    ctl::ComPtr<PrintDocument> spPrintDocumentDO = NULL;

    // This call make come in on a worker thread, post it on the UI-thread since we need to access our objects.

    IFC(ctl::resolve_weakref(m_pPrintDocument, pPrintDocument));
    spPrintDocumentDO = static_cast<PrintDocument*>(pPrintDocument);

    IFC(ctl::make(&spContext));
    SetInterface(spContext->m_pPrintPackageTarget, docPackageTarget);

    IFC(spPrintDocumentDO->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            ctl::WeakRefPtr(m_pPrintDocument),
            &PrintDocument::GetPreviewPageCollectionImpl,
            spContext)));

    ctl::addref_interface(this);
    *docPageCollection = static_cast<IPrintPreviewPageCollection*>(this);

Cleanup:
    ReleaseInterface(pPrintDocument);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//     Callback to indicate that a preview setting has changed that requires the application to
//     repaginate its printing content.
//
//------------------------------------------------------------------------
IFACEMETHODIMP PrintDocumentSource::Paginate(
    UINT32 currentJobPage,
    __RPC__in_opt IInspectable *docSettings)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PrintDocumentInvokeContext> spContext = NULL;
    xaml::Printing::IPrintDocument* pPrintDocument = NULL;
    ctl::ComPtr<PrintDocument> spPrintDocumentDO = NULL;
    ctl::ComPtr<wgr::Printing::IPrintTaskOptionsCore> spPrintTaskOptionsCore = NULL;

    IFC(ctl::resolve_weakref(m_pPrintDocument, pPrintDocument));
    spPrintDocumentDO = static_cast<PrintDocument*>(pPrintDocument);
    if (docSettings)
    {
        spPrintTaskOptionsCore = ctl::query_interface_cast<wgr::Printing::IPrintTaskOptionsCore>(docSettings);
        IFCEXPECT(spPrintTaskOptionsCore != nullptr);
    }

    // This call make come in on a worker thread, post it on the UI-thread since we need to access our objects.

    IFC(ctl::ComObject<PrintDocumentInvokeContext>::CreateInstance(spContext.GetAddressOf()));
    SetInterface(spContext->m_pPrintTaskOptions, spPrintTaskOptionsCore.Get());
    spContext->m_currentJobPage = currentJobPage;

    IFC(spPrintDocumentDO->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            ctl::WeakRefPtr(m_pPrintDocument),
            &PrintDocument::PaginateImpl,
            spContext)));

Cleanup:
    ReleaseInterface(pPrintDocument);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Callback to request the application to draw a preview page
//
//------------------------------------------------------------------------
IFACEMETHODIMP PrintDocumentSource::MakePage(
    UINT32 desiredJobPage,
    FLOAT  width,
    FLOAT  height)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PrintDocumentInvokeContext> spContext = NULL;
    xaml::Printing::IPrintDocument* pPrintDocument = NULL;
    ctl::ComPtr<PrintDocument> spPrintDocumentDO = NULL;

    // This call make come in on a worker thread, post it on the UI-thread since we need to access our objects.

    IFC(ctl::resolve_weakref(m_pPrintDocument, pPrintDocument));
    spPrintDocumentDO = static_cast<PrintDocument*>(pPrintDocument);

    IFC(ctl::ComObject<PrintDocumentInvokeContext>::CreateInstance(spContext.GetAddressOf()));
    spContext->m_makePageDesiredJobPage = desiredJobPage;
    spContext->m_makePageWidth = width;
    spContext->m_makePageHeight = height;

    IFC(spPrintDocumentDO->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            ctl::WeakRefPtr(m_pPrintDocument),
            &PrintDocument::MakePageImpl,
            spContext)));

Cleanup:
    ReleaseInterface(pPrintDocument);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by PrintManager when the Print button on Modern Print Dialog is clicked.
//
//------------------------------------------------------------------------
HRESULT PrintDocumentSource::MakeDocument(
    __RPC__in_opt IInspectable *docSettings,
    __RPC__in_opt IPrintDocumentPackageTarget *docPackageTarget)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<PrintDocumentInvokeContext> spContext = NULL;
    xaml::Printing::IPrintDocument* pPrintDocument = NULL;
    ctl::ComPtr<PrintDocument> spPrintDocumentDO = NULL;
    ctl::ComPtr<wgr::Printing::IPrintTaskOptionsCore> spPrintTaskOptionsCore = NULL;

    IFC(ctl::resolve_weakref(m_pPrintDocument, pPrintDocument));
    spPrintDocumentDO = static_cast<PrintDocument*>(pPrintDocument);

    // MakeDocument may be called on a worker therad. We post a dispatcher callback to do most of our
    // work on the UI-thread. However, on this thread, let the core object know that preview stage is over
    // so that it can immediately stop processing pending preview calls from the application.
    IFC(CoreImports::PrintDocument_EndPreview( static_cast<CPrintDocument*>(spPrintDocumentDO->GetHandle())));

    if (docSettings)
    {
        spPrintTaskOptionsCore = ctl::query_interface_cast<wgr::Printing::IPrintTaskOptionsCore>(docSettings);
        IFCEXPECT(spPrintTaskOptionsCore != nullptr);
    }
    IFC(ctl::ComObject<PrintDocumentInvokeContext>::CreateInstance(spContext.GetAddressOf()));
    SetInterface(spContext->m_pPrintPackageTarget, docPackageTarget);
    SetInterface(spContext->m_pPrintTaskOptions, spPrintTaskOptionsCore.Get());

    IFC(spPrintDocumentDO->GetXamlDispatcherNoRef()->RunAsync(
        MakeCallback(
            ctl::WeakRefPtr(m_pPrintDocument),
            &PrintDocument::MakeDocumentImpl,
            spContext)));

Cleanup:
    ReleaseInterface(pPrintDocument);
    RRETURN(hr);

}

