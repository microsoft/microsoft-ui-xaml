// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlBehaviorMode.h>
#include "ContentRoot.h"
#include "FrameworkInputPaneOneCore.h"

using namespace ATL;
using namespace Microsoft::WRL;

CInputPaneInteractionHelper::CInputPaneInteractionHelper(
    _In_ IXcpInputPaneHandler* pInputPaneHandler)
{
    m_pInputPaneHandler = pInputPaneHandler;
    m_pFrameworkInputPaneHandler = NULL;
    m_dwCookie = 0;
}

CInputPaneInteractionHelper::~CInputPaneInteractionHelper()
{
    ReleaseInterface(m_pFrameworkInputPaneHandler);
}

//-------------------------------------------------------------------------
//
//  Function:   CInputPaneInteractionHelper::Create
//
//  Synopsis:   Create CInputPaneInteractionHelper instance.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneInteractionHelper::Create(
    _In_ IXcpInputPaneHandler* pInputPaneHandler,
    _Outptr_ CInputPaneInteractionHelper** ppInputPaneInteractionHelper)
{
    HRESULT hr = S_OK;
    CInputPaneInteractionHelper* pInputPaneInteractionHelper = NULL;

    ASSERT(pInputPaneHandler);
    ASSERT(ppInputPaneInteractionHelper);
    *ppInputPaneInteractionHelper = NULL;

    pInputPaneInteractionHelper = new CInputPaneInteractionHelper(pInputPaneHandler);

    *ppInputPaneInteractionHelper = pInputPaneInteractionHelper;

    RRETURN(hr);//RRETURN_REMOVAL
}

//-------------------------------------------------------------------------
//
//  Function:   CInputPaneInteractionHelper::RegisterInputPaneHandler
//
//  Synopsis:   Register the input pane handler to receive the notification
//              for input pane windows showing and hiding state change.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneInteractionHelper::RegisterInputPaneHandler(_In_ XHANDLE hWindow, _In_ CContentRoot* contentRoot)
{
    HRESULT hr = S_OK;
    CComObject<CFrameworkInputPaneHandler>* pFrameworkInputPaneHandler = NULL;

    if (m_pInputPane == nullptr)
    {
        if (contentRoot->ShouldUseVisualRelativePixels() || IsXamlBehaviorEnabledForCurrentSku(FrameworkInputPaneOneCore_Enable))
        {
            Microsoft::WRL::ComPtr<FrameworkInputPaneOneCore> spFrameworkInputPane;
            spFrameworkInputPane = wrl::Make<FrameworkInputPaneOneCore>();

            IFC(CComObject<CFrameworkInputPaneHandler>::CreateInstance(&pFrameworkInputPaneHandler));
            ASSERT(m_pInputPaneHandler);

            // As per http://msdn.microsoft.com/en-us/library/vstudio/9e31say1.aspx,
            // CComObject::CreateInstance doesn't AddRef, so AddRef here.
            pFrameworkInputPaneHandler->AddRef();
            IFC(pFrameworkInputPaneHandler->SetInputPaneHandler(m_pInputPaneHandler));

            IFC(spFrameworkInputPane->AdviseInternal(
                hWindow,
                pFrameworkInputPaneHandler,
                &m_dwCookie));

            IFC(spFrameworkInputPane.As(&m_pInputPane));
            IFCEXPECT(m_pInputPane);
        }
        else
        {
            IFC(::CoCreateInstance(CLSID_FrameworkInputPane, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pInputPane)));

            IFCEXPECT(m_pInputPane);
            IFC(CComObject<CFrameworkInputPaneHandler>::CreateInstance(&pFrameworkInputPaneHandler));
            auto frameworkHandlerAsArg  = static_cast<IFrameworkInputPaneHandler*>(pFrameworkInputPaneHandler);
            ASSERT(m_pInputPaneHandler);
            // As per http://msdn.microsoft.com/en-us/library/vstudio/9e31say1.aspx,
            // CComObject::CreateInstance doesn't AddRef, so AddRef here.
            pFrameworkInputPaneHandler->AddRef();
            IFC(pFrameworkInputPaneHandler->SetInputPaneHandler(m_pInputPaneHandler));
            IFC(InputPaneFramework_Advise(m_pInputPane.Get(), static_cast<wuc::ICoreWindow*>(hWindow), frameworkHandlerAsArg, &m_dwCookie));
        }

        m_pFrameworkInputPaneHandler = pFrameworkInputPaneHandler;
        m_pFrameworkInputPaneHandler->AddRef();
    }

Cleanup:
    ReleaseInterface(pFrameworkInputPaneHandler);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CInputPaneInteractionHelper::UnregisterInputPaneHandler
//
//  Synopsis:   Unregister the input pane handler.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputPaneInteractionHelper::UnregisterInputPaneHandler()
{
    HRESULT hr = S_OK;

    if (m_pInputPane && m_dwCookie != 0)
    {
        hr = m_pInputPane->Unadvise(m_dwCookie);
        if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE))
        {
            // Ignore. This will be returned if the window was destroyed before calling this function,
            // which is OK.
            hr = S_OK;
        }
        else
        {
            IFC(hr);
        }
    }

Cleanup:
    ReleaseInterface(m_pFrameworkInputPaneHandler);
    m_pInputPane = nullptr;
    m_dwCookie = 0;

    RRETURN(hr);
}
