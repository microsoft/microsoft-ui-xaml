// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CWindowsServices::CreateD2DPrintFactoryAndTarget(
    _Outptr_ IPALAcceleratedGraphicsFactory **ppPrintFactory,
    _Outptr_ IPALPrintTarget** ppPALPrintTarget)
{
    HRESULT hr = S_OK;
    CD2DPrintTarget* pPrintTarget = NULL;
    CD2DFactory* pFactory = NULL;

    IFC(CD2DFactory::Create(&pFactory));

    IFC(CD2DPrintTarget::Create(
        pFactory,
        &pPrintTarget));

    SetInterface(*ppPrintFactory, pFactory);
    SetInterface(*ppPALPrintTarget, pPrintTarget);

Cleanup:
    ReleaseInterface(pFactory);
    ReleaseInterface(pPrintTarget);
    RRETURN(hr);
}


_Check_return_ HRESULT
CWindowsServices::CreateD2DPrintingData(
    _Outptr_ IPALD2DPrintingData** ppPrintingData)
{
    HRESULT hr = S_OK;
    CD2DPrintingData* pPrintingData = NULL;

    pPrintingData = new CD2DPrintingData();
    SetInterface(*ppPrintingData, pPrintingData);

    ReleaseInterface(pPrintingData);
    RRETURN(hr);//RRETURN_REMOVAL
}

_Check_return_
HRESULT CWindowsServices::CreateResourceProvider(_In_ CCoreServices *pCore, _Outptr_ IPALResourceProvider **ppResourceProvider)
{
    HRESULT hr = S_OK;
    ModernResourceProvider* pResourceProvider = NULL;
    CommonResourceProvider* pCommonResourceProvider = nullptr;

    // Try to use MRT if possible.
    IFC(ModernResourceProvider::TryCreate(pCore, &pResourceProvider));
    if (pResourceProvider)
    {
        *ppResourceProvider = pResourceProvider;
        pResourceProvider = NULL;
        goto Cleanup;
    }

    // If we can't use MRT, fallback to the non-MRT resource provider.
    IFC(CommonResourceProvider::Create(&pCommonResourceProvider));

    *ppResourceProvider = pCommonResourceProvider;
    pCommonResourceProvider = nullptr;

Cleanup:
    ReleaseInterface(pResourceProvider);
    ReleaseInterface(pCommonResourceProvider);

    RRETURN(hr);
}

_Check_return_
HRESULT CWindowsServices::CreateApplicationDataProvider(_Outptr_ IPALApplicationDataProvider **ppAppDataProvider)
{
    HRESULT hr = S_OK;
    ApplicationDataProvider *pAppDataProvider = NULL;

    IFC(ApplicationDataProvider::Create(&pAppDataProvider));

    *ppAppDataProvider = pAppDataProvider;
    pAppDataProvider = NULL;

Cleanup:
    ReleaseInterface(pAppDataProvider);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get automation provider for an element
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CWindowsServices::GetAutomationProvider(
    _In_ IXcpHostSite *pSite,
    _In_ CDependencyObject *pElement,
    _Outptr_ IUnknown** ppProvider)
{
    HRESULT hr = S_OK;
    CAutomationPeer *pAutomationPeer = NULL;
    CUIAWindow *pWindow = NULL;
    CUIAWrapper* pWrapper = NULL;

    IFC(pSite->GetUIAWindow(pElement, pSite->GetXcpControlWindow(), false /*onlyGet*/, &pWindow));
    pAutomationPeer = pElement->OnCreateAutomationPeer();

    IFCPTR(pAutomationPeer);

    IFC(pWindow->CreateProviderForAP(pAutomationPeer, &pWrapper));
    IFC(pWrapper->QueryInterface(IID_IUnknown, reinterpret_cast<void**>(ppProvider)));

Cleanup:
    ReleaseInterface(pWrapper);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      We need a piece of memory that outlives the actual CCoreServices object.
//      This Thread Local Storage, TLS, and accessor functions are used to prevent
//      callers from attempting to access the core after it has already been destroyed.
//
//------------------------------------------------------------------------

XUINT32 g_nTlsIsCoreServicesReady = TLS_OUT_OF_INDEXES;

bool CCoreServices::GetIsCoreServicesReady()
{
    bool ready = false;

    if (g_nTlsIsCoreServicesReady != TLS_OUT_OF_INDEXES)
    {
        ready = !!TlsGetValue(g_nTlsIsCoreServicesReady);
    }

    return ready;
}

void CCoreServices::SetIsCoreServicesReady(bool value)
{
    if (g_nTlsIsCoreServicesReady != TLS_OUT_OF_INDEXES)
    {
        uintptr_t valueTls = value;
        TlsSetValue(g_nTlsIsCoreServicesReady, reinterpret_cast<LPVOID>(valueTls));
    }
}
