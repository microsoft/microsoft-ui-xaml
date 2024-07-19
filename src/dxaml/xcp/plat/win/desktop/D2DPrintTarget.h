// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <d2d1_1.h>
#include "documenttarget.h"
#include "wincodec.h"
#include "d2daccelerated.h"
#include "d2dacceleratedrt.h"

class CD2DPrintTarget : public CD2DRenderTarget<IPALPrintTarget>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ CD2DFactory* pFactory,
        _Outptr_ CD2DPrintTarget** ppPALPrintTarget
        );

    //
    // IPALPrintTarget
    //

    _Check_return_ HRESULT BeginPreview(_In_ IPALPrintingData* pPreviewData) override;
    _Check_return_ HRESULT InvalidatePreview() override;
    _Check_return_ HRESULT EndPreview() override;
    _Check_return_ HRESULT SetPreviewPageCount(PALPreviewPageCountType pageCountType, XINT32 pageCount) override;
    _Check_return_ HRESULT BeginPrint(_In_ IPALPrintingData* pPD) override;
    _Check_return_ HRESULT EndPrint() override;
    _Check_return_ HRESULT BeginPage(XINT32 pageNumber) override;
    _Check_return_ HRESULT EndPage() override;
    _Check_return_ HRESULT CancelPage() override;


protected:
    CD2DPrintTarget()
    {
        m_pCommandList = NULL;
        m_pD2D1Device = NULL;
        m_pPrintControl = NULL;
        m_fPreviewStage = FALSE;
        m_fPrintStage = FALSE;
        m_pPreviewData = NULL;
        m_iPageNumber = 0;
        m_pPrintQueue = NULL;
        m_pD3DDevice = NULL;
    }

    ~CD2DPrintTarget() override
    {
        // All printing resources except for command list are created on print thread.
        // Send a message to destroy those resources.
        IGNOREHR(SendPrintMessage(WM_D2DPRINT_DESTROY, (WPARAM)this, 0));
        ReleaseInterface(m_pCommandList);
        ReleaseInterface(m_pPreviewData);
        if (m_pPrintQueue)
        {
            IGNOREHR(m_pPrintQueue->Close());
        }
    }

private:
    _Check_return_ HRESULT Initialize(
        _In_ CD2DFactory* pFactory);

    HRESULT PostPrintMessage(
        _In_ UINT msg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);

    HRESULT SendPrintMessage(
        _In_ UINT msg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);

    static XINT32 PrintThreadFn(
        _In_reads_bytes_(sizeof(PrintThreadData)) XUINT8* pData);

    static void ProcessPrintMessage(
        _In_ UINT   msg,
        _In_ WPARAM wParam,
        _In_ LPARAM lParam);


    HRESULT InitializeImpl();
    HRESULT DestroyImpl();
    HRESULT BeginPrintImpl(LPARAM lParam);
    HRESULT PrintPageImpl(LPARAM lParam);
    HRESULT EndPrintImpl();

    _Check_return_ HRESULT CommandListToDXGISurface(
        _In_ ID2D1CommandList* pCommandList,
        _In_ ID3D11Device* pD3D11Device,
        _In_ ID2D1DeviceContext* pD2DDeviceContext,
        _In_ XSIZEF commandListSize,
        _In_ XSIZEF surfaceSize,
        _Outptr_ IDXGISurface** ppSurface
        );

    struct PrintMessage
    {
        PrintMessage()
        {
            msg = 0;
            wParam = 0;
            lParam = 0;
            pSendMessageEvent = NULL;
        }

        PrintMessage(
            UINT msgArg,
            WPARAM wParamArg,
            LPARAM lParamArg,
            IPALEvent* pSendMessageEventArg)
        {
            msg = msgArg;
            wParam = wParamArg;
            lParam = lParamArg;
            pSendMessageEvent = pSendMessageEventArg;
        }

        UINT msg;
        WPARAM wParam;
        LPARAM lParam;
        IPALEvent* pSendMessageEvent;
    };

    static const UINT WM_D2DPRINT_INITIALIZE;
    static const UINT WM_D2DPRINT_BEGINPRINT;
    static const UINT WM_D2DPRINT_PRINTPAGE;
    static const UINT WM_D2DPRINT_ENDPRINT;
    static const UINT WM_D2DPRINT_DESTROY;

public:
    ID2D1Device* m_pD2D1Device;
    ID3D11Device* m_pD3DDevice;
    ID2D1CommandList* m_pCommandList;
    ID2D1PrintControl* m_pPrintControl;
    D2D_SIZE_F m_pageSize;
    IPALD2DPrintingData* m_pPreviewData;

    bool m_fPreviewStage;
    bool m_fPrintStage;
    XINT32 m_iPageNumber;
    IPALQueue* m_pPrintQueue;
};

