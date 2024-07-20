// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.media.h>

typedef HRESULT (WINAPI *LPFNMFSTARTUP) (ULONG Version, DWORD dwFlags);
typedef HRESULT (WINAPI *LPFNMFSHUTDOWN) ( );
typedef HRESULT (WINAPI *LPFNMFCREATEATTRIBUTES) (_Out_ IMFAttributes** ppMFAttributes, _In_  UINT32 cInitialSize );

//-----------------------------------------------------------------------------
//
// Class ImageSharingEngineHost:
//
//    Embed IMFImageSharingEngine and implements IXcpImageSharingEngineService
//    to work with Image in UICore.dll
//
//-----------------------------------------------------------------------------
class ImageSharingEngineHost final     : public CXcpObjectBase< >,
                                  public IXcpImageSharingEngine,
                                  public IMediaQueueClient
{
public:

    static _Check_return_ HRESULT Create( _In_ IMediaServices* pMediaServices,
                                         _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify,
                                         _Outptr_ IXcpImageSharingEngine** ppXcpImageSharingEngine );

public:

    FORWARD_ADDREF_RELEASE( CXcpObjectBase< > );

    // IXcpImageSharingEngine
    virtual void Shutdown( );
    virtual _Check_return_ HRESULT SetSource( _In_ IPALSurface *pSoftwareSurface );
    virtual _Check_return_ HRESULT InitializeCasting(_In_ IInspectable *pCastingEngine, _In_ IUnknown *pMediaEngineCallback, _In_ wm::Casting::ICastingSource *pSource);
    virtual _Check_return_ HRESULT Connect(_In_opt_ IMFSharingEngineClassFactory *pFactory, bool createSharingEngine);
    virtual _Check_return_ HRESULT Disconnect();

    // IMediaQueueClient interface
    virtual _Check_return_ HRESULT HandleMediaEvent( _In_ IObject *pState, _In_ XINT32 bIsShuttingDown );

protected:

    ImageSharingEngineHost();
    virtual ~ImageSharingEngineHost();

    _Check_return_ HRESULT Initialize( _In_ IMediaServices* pMediaServices,  _In_ IXcpImageSharingEngineNotify *pXcpImageSharingEngineNotify );

    void Deinitialize();

    _Check_return_ HRESULT QueueEvent( _In_ bool onConnected );

    // -------------------------------------------------------------------
    //  Class:      ImageSharingState
    //
    //  Synopsis:
    //      An embedded class for ImageSharingState that can be used to
    //      queue event and disptach it to UI thread for execution.
    //
    // -------------------------------------------------------------------
    class ImageSharingState final : public CXcpObjectBase<IObject>
    {
    public:
        ImageSharingState(_In_ bool onConnected)
        {
            m_OnConnected = onConnected;
        }

        ~ImageSharingState() { }

        FORWARD_ADDREF_RELEASE( CXcpObjectBase<IObject> );
        bool  IsOnConnected( ) { return m_OnConnected; }

    private:
        bool     m_OnConnected;
    };


private:
    IMFImageSharingEngine         *m_pImageSharingEngine;
    wil::critical_section          m_MECritSection;
    HMODULE                        m_moduleMFPlat;
    LPFNMFCREATEATTRIBUTES         m_pfnMFCreateAttributes;
    LPFNMFSTARTUP                  m_pfnMFStartup;
    LPFNMFSHUTDOWN                 m_pfnMFShutdown;
    bool                           m_shouldShutdownMF;
    IWICBitmap                    *m_pBitmapSource;
    IMediaServices                *m_pCoreMediaServices;
    IXcpImageSharingEngineNotify *m_pImageSharingEngineNotify ;
    IMediaQueue                   *m_pMediaQueue;
};
