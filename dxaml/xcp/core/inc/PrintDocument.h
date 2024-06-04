// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Synopsis:
//      Core printing class.
//      Represents a PrintDocument which can be used by subscribing to the
//      PrintPage event and then calling Print.
//      The user would handle the PrintPage event and set the PageVisual
//      event of the PrintPageEventArgs.
//      The user may also subscribe to the StartPrint and EndPrint events
//      to properly initialize, cleanup (respectively) the PrintDocument
//      object.

class CPrintDocument final : public CDependencyObject
{
private:
    CPrintDocument(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    ~CPrintDocument() override;

    _Check_return_ HRESULT InitializeBitmapPrinting();
    _Check_return_ HRESULT RaisePrintingEvent(
        _In_ EventHandle hEvent,
        _In_ CEventArgs **ppArgs);

    _Check_return_ HRESULT UnregisterEvents();

    _Check_return_ HRESULT EndPrinting();

    _Check_return_ HRESULT LayoutAndPrintSinglePage(
        XINT32 pageNumber,
        _In_opt_ CUIElement* pPageVisual);

    _Check_return_ HRESULT PrintSinglePage(
        XINT32 pageNumber,
        _In_ CUIElement* pPageVisual);

    _Check_return_ HRESULT PrintSinglePageVectorFallback(
        _In_ CUIElement* pPageVisual
        );
    _Check_return_ HRESULT GetPrintFactoryAndTarget(
        _In_ CCoreServices *pCore,
        _Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory,
        _Outptr_ IPALPrintTarget **ppPrintTarget
        );

public:
    DECLARE_CREATE(CPrintDocument);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPrintDocument>::Index;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        // Peer has state
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    bool AllowsHandlerWhenNotLive(XINT32 iListenerType, KnownEventIndex eventIndex) const final
    {
        return true;
    }

    void Reset();

    _Check_return_ HRESULT AddEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue,
        _In_ XINT32 iListenerType,
        _Out_opt_ CValue *pResult ,
        _In_ bool fHandledEventsToo = false) final;

    _Check_return_ HRESULT RemoveEventListener(
        _In_ EventHandle hEvent,
        _In_ CValue *pValue) override;

    _Check_return_ HRESULT BeginPreview(_In_ void* pPreviewPackageTarget);
    void EndPreview();
    _Check_return_ HRESULT Paginate(
        _In_opt_ void* pDocumentSettings);
    _Check_return_ HRESULT GetPreviewPage(
        XUINT32 desiredJobPage,
        XFLOAT width,
        XFLOAT height);
    _Check_return_ HRESULT MakeDocument(
        _In_ void* pDocPackageTarget,
        _In_opt_ void* pDocSettings);

    _Check_return_ HRESULT SetPreviewPageCount(
        _In_ XINT32 count,
        _In_ DirectUI::PreviewPageCountType type);

    _Check_return_ HRESULT SetPreviewPage(
        _In_ XINT32 pageNumber,
        _In_ CUIElement* pageVisual);

    _Check_return_ HRESULT InvalidatePreview();

    _Check_return_ HRESULT AddPage(
        _In_ CUIElement* pageVisual);

    _Check_return_ HRESULT AddPagesComplete();

private:
    // Events
    CXcpList<REQUEST>   *m_pEventList       = nullptr;
    CMatrixTransform    *m_pFinalTransform  = nullptr; // RenderTransform (on element) + ScaleTransform (printing)
    CScaleTransform     *m_pScaleTransform  = nullptr;
    CWriteableBitmap    *m_pWriteableBitmap = nullptr;
    XINT32              *m_pPixels          = nullptr;
    CRectangle          *m_pWhiteRect       = nullptr;
    CSolidColorBrush    *m_pWhiteBrush      = nullptr;
    CTranslateTransform *m_pDummyTransform  = nullptr;
    IPALPrintingData2   *m_pPD              = nullptr;
    IPALD2DPrintingData *m_pD2DPrintingData = nullptr;

    XINT32               m_iErrorCode               = 0;
    bool m_fForceVector = false;
    IPALAcceleratedGraphicsFactory *m_pPrintFactory = nullptr;
    IPALPrintTarget     *m_pPrintTarget             = nullptr;
    bool m_fPreviewStage = false;

public:
    XINT32                          m_nPrintedPageCount = 0;
    DirectUI::PrintDocumentFormat   m_eDesiredFormat    = DirectUI::PrintDocumentFormat::Bitmap;
};
