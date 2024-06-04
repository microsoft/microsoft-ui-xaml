// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SurfaceImageSource.g.h"
#include "DCompSurfaceFactoryManager.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

SurfaceImageSource::SurfaceImageSource()
: m_SISWithD2DImpl(this)
{
}

SurfaceImageSource::~SurfaceImageSource()
{

}

HRESULT SurfaceImageSource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    HRESULT hr = S_OK;

    if (InlineIsEqualGUID(iid, __uuidof(ISurfaceImageSourceNative)))
    {
        IFC_NOTRACE(CheckThread());
        *ppObject = static_cast<ISurfaceImageSourceNative *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(ISurfaceImageSourceNativeWithD2D)))
    {
        *ppObject = static_cast<ISurfaceImageSourceNativeWithD2D *>(&m_SISWithD2DImpl);
    }
    else
    {
        RRETURN(DirectUI::SurfaceImageSourceGenerated::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Changes the DXGI device used to render updates into this SurfaceImageSource
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::SetDevice(_In_ IDXGIDevice* pDevice)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::SurfaceImageSource_SetDevice(static_cast<CSurfaceImageSource*>(GetHandle()), pDevice));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Begins rendering an update into this SurfaceImageSource
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::BeginDraw(_In_ RECT updateRect, _Outptr_ IDXGISurface** ppSurface, _Out_ POINT* pOffset)
{
    HRESULT hr = S_OK;

    IUnknown *pUnknown = NULL;
    XPOINT xpoint;
    XRECT xrect = {};

    IFC(CheckThread());

    xrect.X = updateRect.left;
    xrect.Y = updateRect.top;
    xrect.Width = updateRect.right - updateRect.left;
    xrect.Height = updateRect.bottom - updateRect.top;

    IFC(CoreImports::SurfaceImageSource_BeginDraw(
        static_cast<CSurfaceImageSource*>(GetHandle()),
        &xrect,
        __uuidof(IDXGISurface),
        &pUnknown,
        &xpoint
        ));

    IFC(pUnknown->QueryInterface(ppSurface));

    pOffset->x = xpoint.x;
    pOffset->y = xpoint.y;

Cleanup:
    ReleaseInterface(pUnknown);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Completes rendering an update into a SurfaceImageSource
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::EndDraw()
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::SurfaceImageSource_EndDraw(static_cast<CSurfaceImageSource*>(GetHandle())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Changes the DXGI device or D2D Device used to render updates into this
//     SurfaceImageSource.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::SetDeviceWithD2D(
    _In_ IUnknown*  pDevice)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::SurfaceImageSource_SetDeviceWithD2D(static_cast<CSurfaceImageSource*>(GetHandle()), pDevice));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Begins rendering an update into this SurfaceImageSource
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::BeginDrawWithD2D(
    _In_ RECT updateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown** ppSurface,
    _Out_ POINT* pOffset)
{
    HRESULT hr = S_OK;

    XPOINT xpoint;
    XRECT xrect = {};

    bool calledFromUIThread = SUCCEEDED(CheckThread());

    xrect.X = updateRect.left;
    xrect.Y = updateRect.top;
    xrect.Width = updateRect.right - updateRect.left;
    xrect.Height = updateRect.bottom - updateRect.top;

    IFC(CoreImports::SurfaceImageSource_BeginDrawWithD2D(
        static_cast<CSurfaceImageSource*>(GetHandle()),
        &xrect,
        iid,
        calledFromUIThread,
        ppSurface,
        &xpoint
        ));

    pOffset->x = xpoint.x;
    pOffset->y = xpoint.y;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Completes rendering an update into a SurfaceImageSource
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::EndDrawWithD2D()
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::SurfaceImageSource_EndDrawWithD2D(static_cast<CSurfaceImageSource*>(GetHandle())));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Puts this SurfaceImagesource in "Suspended-Drawing" state.
//     In this state, drawing cannot be performed.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::SuspendDraw()
{
    HRESULT hr = S_OK;

    bool calledFromUIThread = SUCCEEDED(CheckThread());

    IFC(CoreImports::SurfaceImageSource_SuspendDraw(static_cast<CSurfaceImageSource*>(GetHandle()), calledFromUIThread));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Puts this SurfaceImagesource back in "Drawing" state.
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
SurfaceImageSource::ResumeDraw()
{
    HRESULT hr = S_OK;

    bool calledFromUIThread = SUCCEEDED(CheckThread());

    IFC(CoreImports::SurfaceImageSource_ResumeDraw(static_cast<CSurfaceImageSource*>(GetHandle()), calledFromUIThread));

Cleanup:
    RRETURN(hr);
}


