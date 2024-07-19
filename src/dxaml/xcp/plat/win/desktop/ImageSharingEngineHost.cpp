// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <mfapi.h>

//
// A wrapper static method that can be called by DirectUI components in
// other libraries.
//
_Check_return_ HRESULT
CreateImageSharingEngine(
    _In_ IMediaServices* pMediaServices,
    _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify,
    _Outptr_ IXcpImageSharingEngine** ppXcpImageSharingEngine)
{
    return ( ImageSharingEngineHost::Create( pMediaServices,
                                             pXcpImageSharingEngineNotify,
                                             ppXcpImageSharingEngine ) );
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//
//       Create a new ImageSharingEngineHost instance.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSharingEngineHost::Create( _In_ IMediaServices* pMediaServices,
                                _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify,
                                _Outptr_ IXcpImageSharingEngine** ppXcpImageSharingEngine)
{
    HRESULT hr = S_OK;
    ImageSharingEngineHost* pImageSharingEngineHost = NULL;

    IFCPTR( ppXcpImageSharingEngine );

    *ppXcpImageSharingEngine = NULL;

    pImageSharingEngineHost = new ImageSharingEngineHost();

    IFC( pImageSharingEngineHost->Initialize( pMediaServices, pXcpImageSharingEngineNotify ) );

    *ppXcpImageSharingEngine = static_cast<IXcpImageSharingEngine *>( pImageSharingEngineHost );
    pImageSharingEngineHost = NULL;

Cleanup:

    ReleaseInterface( pImageSharingEngineHost );
    return hr;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//            Default ctor of ImageSharingEngineHost
//----------------------------------------------------------------------------
ImageSharingEngineHost::ImageSharingEngineHost()
    :    m_pImageSharingEngine( NULL ),
         m_pBitmapSource ( NULL ),
         m_pfnMFCreateAttributes ( NULL ),
         m_moduleMFPlat ( NULL ),
         m_pCoreMediaServices ( NULL ),
         m_pImageSharingEngineNotify( NULL ),
         m_pMediaQueue( NULL ),
         m_shouldShutdownMF(false),
         m_pfnMFStartup(nullptr),
         m_pfnMFShutdown(nullptr)
{
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//          Default dtor of ImageSharingEngineHost
//
//----------------------------------------------------------------------------
ImageSharingEngineHost::~ImageSharingEngineHost()
{
    Deinitialize();
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//          Initialize the ImageSharingEngineHost
//
//----------------------------------------------------------------------------
_Check_return_
HRESULT
ImageSharingEngineHost::Initialize( _In_ IMediaServices* pMediaServices,
                                    _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify )
{
    HRESULT hr = S_OK;

    IFCPTR( pMediaServices );
    IFCPTR( pXcpImageSharingEngineNotify );

    if (m_moduleMFPlat == NULL)
    {
        IFCW32(m_moduleMFPlat = LoadLibraryEx(L"mfplat.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));

        m_pfnMFCreateAttributes = (LPFNMFCREATEATTRIBUTES) GetProcAddress( m_moduleMFPlat, "MFCreateAttributes" );
        IFCEXPECT(m_pfnMFCreateAttributes);

        m_pfnMFStartup = (LPFNMFSTARTUP)GetProcAddress(m_moduleMFPlat, "MFStartup");
        IFCEXPECT(m_pfnMFStartup);

        m_pfnMFShutdown = (LPFNMFSHUTDOWN)GetProcAddress(m_moduleMFPlat, "MFShutdown");
        IFCEXPECT(m_pfnMFShutdown);
    }

    IFC(m_pfnMFStartup(MF_VERSION, MFSTARTUP_FULL));
    m_shouldShutdownMF = true;

    m_pCoreMediaServices = pMediaServices;
    m_pImageSharingEngineNotify = pXcpImageSharingEngineNotify;

    IFC(m_pCoreMediaServices->CreateMediaQueue( static_cast<IMediaQueueClient *>(this), &m_pMediaQueue));

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Tears down everything Initialize did, along with removing the
//       media item from the queue if it was inserted.  Puts the engine in
//       a 'disabled' state.
//
//----------------------------------------------------------------------------
void ImageSharingEngineHost::Deinitialize()
{
    auto Lock = m_MECritSection.lock();

    Shutdown( );

    ReleaseInterface(m_pBitmapSource);

    if (m_shouldShutdownMF)
    {
        m_pfnMFShutdown();
        m_shouldShutdownMF = false;
    }

    if (m_moduleMFPlat)
    {
        FreeLibrary(m_moduleMFPlat);
        m_moduleMFPlat = nullptr;

        m_pfnMFCreateAttributes = nullptr;
        m_pfnMFShutdown = nullptr;
        m_pfnMFStartup = nullptr;
    }
}
//----------------------------------------------------------------------------------
//
//  Synopsis:
//
//        Connect to a DLNA Renderer
//
//----------------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSharingEngineHost::Connect(_In_opt_ IMFSharingEngineClassFactory *pFactory, bool createSharingEngine)
{
    HRESULT                hr = S_OK;
    IMFImageSharingEngine *pRemoteEngine = NULL;
    IUnknown              *pUnkEngine = NULL;
    IMFAttributes         *pAttr = NULL;
    auto Lock = m_MECritSection.lock();

    if (createSharingEngine && (m_pImageSharingEngine == NULL))
    {
        ASSERT(m_pfnMFCreateAttributes != NULL);
        IFC(m_pfnMFCreateAttributes(&pAttr, 1));

        IFC( pFactory->CreateInstance( 0, pAttr, &pUnkEngine ) );
        IFC( pUnkEngine->QueryInterface( IID_PPV_ARGS(&pRemoteEngine )));
        m_pImageSharingEngine = pRemoteEngine;
        m_pImageSharingEngine->AddRef();
    }

    //
    // Pass the BitmapSource to this SharingEngine if the source is ready.
    //
    if ( m_pBitmapSource )
    {
        ASSERT( m_pImageSharingEngine != NULL );
        m_pImageSharingEngine->SetSource( m_pBitmapSource );
    }
    else
    {
        //
        // If the BitmapSource is not ready, notify the UI Element
        // and ask for it.
        if ( m_pImageSharingEngineNotify )
        {
            IFC(QueueEvent( TRUE ));
        }
    }

Cleanup:

    ReleaseInterface( pAttr );
    ReleaseInterface( pUnkEngine );
    RRETURN(hr);
}

//----------------------------------------------------------------------------------
//
//  Synopsis:
//
//        Disconnect from a DLNA Renderer
//
//----------------------------------------------------------------------------------
_Check_return_ HRESULT
ImageSharingEngineHost::Disconnect()
{
    HRESULT hr = S_OK;
    auto Lock = m_MECritSection.lock();

    if (m_pImageSharingEngine)
    {
        m_pImageSharingEngine->Shutdown();
        ReleaseInterface( m_pImageSharingEngine );
    }

    //
    // There is no need to hold the reference of BitmapSource anymore
    //
    ReleaseInterface( m_pBitmapSource );

    //
    // Notify the UI Element of the disconnection from the sharing engine.
    //
    if ( m_pImageSharingEngineNotify )
    {
        IFC( QueueEvent( FALSE ) );
    }

Cleanup:
    RRETURN(hr);
}


// Initializes the ImageSharingEngineHost for Casting
_Check_return_ HRESULT
ImageSharingEngineHost::InitializeCasting(_In_ IInspectable *pCastingEngine, _In_ IUnknown *pMediaEngineCallback, _In_ wm::Casting::ICastingSource *pSource)
{
    HRESULT hr = S_OK;
    IUnknown *pUnkEngine = nullptr;
    IMFAttributes *pAttr = nullptr;
    IMFSharingEngineClassFactory *pFactory = nullptr;
    wf::IUriRuntimeClass *pPreferredSourceUri = nullptr;
    auto Lock = m_MECritSection.lock();

    IFCPTR(pCastingEngine);
    IFCPTR(pSource);

    if (m_pImageSharingEngine == nullptr)
    {
        IFC(m_pfnMFCreateAttributes(&pAttr, 3));
        IFC(CoCreateInstance(CLSID_MFImageSharingEngineClassFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFactory)));
        IFC(pAttr->SetUnknown(MF_SHARING_ENGINE_CALLBACK, pMediaEngineCallback));
        IFC(pAttr->SetUnknown(MF_SHARING_ENGINE_SHAREDRENDERER, pCastingEngine));
        IFC(pAttr->SetUINT32(MF_SHUTDOWN_RENDERER_ON_ENGINE_SHUTDOWN, TRUE));

        // PreferredSourceUri is optional - don't fail here if it is null
        if (SUCCEEDED(pSource->get_PreferredSourceUri(&pPreferredSourceUri)) && (pPreferredSourceUri != nullptr))
        {
            wrl_wrappers::HString strPreferredSourceUri;
            IFC(pPreferredSourceUri->get_AbsoluteUri(strPreferredSourceUri.GetAddressOf()));
            IFC(pAttr->SetString(MF_PREFERRED_SOURCE_URI, strPreferredSourceUri.GetRawBuffer(nullptr)));
        }

        IFC(pFactory->CreateInstance(0, pAttr, &pUnkEngine));
        IFC(pUnkEngine->QueryInterface(IID_PPV_ARGS(&m_pImageSharingEngine)));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pAttr);
    ReleaseInterfaceNoNULL(pUnkEngine);
    ReleaseInterfaceNoNULL(pFactory);
    ReleaseInterfaceNoNULL(pPreferredSourceUri);
    return hr;
}

//----------------------------------------------------------------------------------
//
//  Synopsis:
//
//        Set the source on the ImageSharingEngine
//
//----------------------------------------------------------------------------------
_Check_return_ HRESULT ImageSharingEngineHost::SetSource(_In_ IPALSurface *pSoftwareSurface)
{
    HRESULT       hr = S_OK;
    XUINT32       width = 0;
    XUINT32       height = 0;
    XINT32        stride = 0;
    BYTE         *pDataSrc = NULL;
    bool shouldUnlock = false;
    IWICBitmap   *pWicBitmap = NULL;

    IWICImagingFactory *pWicFactory = NULL;
    XUINT32       buffSize = 0;

    auto Lock = m_MECritSection.lock();

    IFCPTR(pSoftwareSurface);

    //
    // Clean up the previous BitmapSource.
    //
    ReleaseInterface( m_pBitmapSource );

    IFC( pSoftwareSurface->Lock((void **)&pDataSrc, &stride, &width, &height));
    shouldUnlock = TRUE;

    IFC( CoCreateInstance( CLSID_WICImagingFactory,
                           nullptr,
                           CLSCTX_INPROC_SERVER,
                           IID_PPV_ARGS( &pWicFactory ) ) );

    buffSize = (((width * 4) +3 ) & ~3 ) * height;

    IFC(pWicFactory->CreateBitmapFromMemory( width,
                                             height,
                                             GUID_WICPixelFormat32bppPBGRA,
                                             stride,
                                             buffSize,
                                             pDataSrc,
                                             &pWicBitmap ) );

    m_pBitmapSource = pWicBitmap;
    pWicBitmap = NULL;

    if ( m_pImageSharingEngine )
    {
        m_pImageSharingEngine->SetSource( m_pBitmapSource );
    }

Cleanup:

    if ( shouldUnlock && pSoftwareSurface )
    {
        IGNOREHR( pSoftwareSurface->Unlock( ) );
    }

    ReleaseInterface( pWicBitmap );
    ReleaseInterface( pWicFactory );

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Shutdown the related componments and break the relationship between
//       this object and UI element.
//
//----------------------------------------------------------------------------
void
ImageSharingEngineHost::Shutdown( )
{
    auto Lock = m_MECritSection.lock();

    m_pImageSharingEngineNotify = NULL;

    if ( m_pImageSharingEngine )
    {
        m_pImageSharingEngine->Shutdown();
        ReleaseInterface( m_pImageSharingEngine );
    }

    if ( m_pMediaQueue )
    {
        IGNOREHR( m_pMediaQueue->Shutdown( ) );
        ReleaseInterface( m_pMediaQueue );
    }

    // Disconnect any casting sessions that are currently running
    Disconnect();

}

//------------------------------------------------------------------------
//
//  Method:   QueueEvent
//
//  Synopsis:
// TODO: review...
//      Queues up this CMediaElement event on the MediaQueue.
//
//------------------------------------------------------------------------
_Check_return_  HRESULT
ImageSharingEngineHost::QueueEvent( _In_ bool onConnected )
{
    HRESULT hr = S_OK;
    ImageSharingState *pState = NULL;

    pState = new ImageSharingState( onConnected );

    IFC( m_pMediaQueue->AddMediaEvent( pState, NULL ) );

Cleanup:
    ReleaseInterface( pState );
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handles an event from the MediaQueue on the UI thread.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
ImageSharingEngineHost::HandleMediaEvent( _In_ IObject *pState, _In_ XINT32 bIsShuttingDown )
{
    HRESULT hr = S_OK;
    auto Lock = m_MECritSection.lock();

    ImageSharingState *pEventState = static_cast< ImageSharingState* >( pState );

    IFCPTR(pState);

    if ( !bIsShuttingDown && m_pImageSharingEngineNotify )
    {
        // Call back to UI element on UI thread.
        if ( pEventState->IsOnConnected() )
        {
            IFC( m_pImageSharingEngineNotify->OnConnected() );
        }
        else
        {
            IFC( m_pImageSharingEngineNotify->OnDisconnected() );
        }
    }

Cleanup:
    RRETURN(hr);
}
