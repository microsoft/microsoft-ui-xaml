// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "documenttarget.h"
#include "documentsource.h"
#include "windows.graphics.printing.h"
//$TODO- Remove when preview is convered to DXGI surface
#include "printpreview.h"
#include "windows.graphics.printing.optiondetails.h"
#include "PrintDocument.g.h"
#include "Callback.h"

namespace DirectUI
{
    class PrintDocumentSource;
    class PrintDocumentPreviewPageCollection;
    class PrintDocumentInvokeContext;

    PARTIAL_CLASS(PrintDocument)
    {
        protected:
            PrintDocument();
            ~PrintDocument() override;

        public:
            _Check_return_ HRESULT get_DocumentSourceImpl(_Outptr_ wgr::Printing::IPrintDocumentSource** pValue);

            HRESULT GetPreviewPageCollectionImpl(
                _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext);

            HRESULT PaginateImpl(
                _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext);

            HRESULT MakePageImpl(
                _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext);

            HRESULT MakeDocumentImpl(
                _In_ ctl::ComPtr<PrintDocumentInvokeContext> spContext);

        private:
            PrintDocumentSource* m_pDocumentSource;
    };

    class PrintDocumentSource:
        public IPrintDocumentPageSource,
        public IPrintPreviewPageCollection,
        public wgr::Printing::IPrintDocumentSource,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(PrintDocumentSource, ComBase)
            INTERFACE_ENTRY(PrintDocumentSource, IPrintDocumentPageSource )
            INTERFACE_ENTRY(PrintDocumentSource, IPrintPreviewPageCollection )
            INTERFACE_ENTRY(PrintDocumentSource, wgr::Printing::IPrintDocumentSource )
        END_INTERFACE_MAP(PrintDocumentSource, ComBase)

        protected:
            PrintDocumentSource();
            ~PrintDocumentSource() override;
            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

        public:
            // IPrintDocumentPageSource methods
            HRESULT STDMETHODCALLTYPE GetPreviewPageCollection(
               /* [in] */ __RPC__in_opt IPrintDocumentPackageTarget *docPackageTarget,
               /* [out] */ __RPC__deref_out_opt IPrintPreviewPageCollection **docPageCollection) override;

            HRESULT STDMETHODCALLTYPE MakeDocument(
                /* [in] */ __RPC__in_opt IInspectable *docSettings,
                /* [in] */ __RPC__in_opt IPrintDocumentPackageTarget *docPackageTarget) override;

            // IPrintPreviewPageCollection methods
            IFACEMETHOD(Paginate)(
                UINT32 currentJobPage,
                __RPC__in_opt IInspectable *docSettings);

            IFACEMETHOD(MakePage)(
                UINT32 desiredJobPage,
                FLOAT  width,
                FLOAT  height);

            _Check_return_ HRESULT SetPrintDocument(_In_ PrintDocument* pPrintDocument);

        private:
            IWeakReference* m_pPrintDocument;
    };

    class PrintDocumentInvokeContext:
        public ctl::ComBase
    {
        friend class PrintDocumentSource;
        friend class PrintDocument;

    protected:
        PrintDocumentInvokeContext()
        {
            m_pPrintPackageTarget = NULL;
            m_pPrintTaskOptions = NULL;
            m_currentJobPage = 0;
            m_makePageDesiredJobPage = 0;
            m_makePageWidth = m_makePageHeight = 0;
        }

        ~PrintDocumentInvokeContext() override
        {
            ReleaseInterface(m_pPrintPackageTarget);
            ReleaseInterface(m_pPrintTaskOptions);
        }

    private:
        IPrintDocumentPackageTarget* m_pPrintPackageTarget;
        wgr::Printing::IPrintTaskOptionsCore* m_pPrintTaskOptions;
        UINT32 m_currentJobPage;
        UINT32 m_makePageDesiredJobPage;
        FLOAT  m_makePageWidth;
        FLOAT  m_makePageHeight;
    };

    template <>
    struct WeakRefTypeTraits<PrintDocument>
    {
        typedef xaml::Printing::IPrintDocument ResolveType;
    };
}
