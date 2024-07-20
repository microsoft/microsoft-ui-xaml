// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "VirtualSurfaceImageSource.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

VirtualSurfaceImageSource::VirtualSurfaceImageSource()
{
    m_pUpdatesCallback = NULL;
}

VirtualSurfaceImageSource::~VirtualSurfaceImageSource()
{
    // Clear out the pointer before calling ReleaseInterface in case the user does something
    // in Release that re-enters our callback.
    IVirtualSurfaceUpdatesCallbackNative *pTemp = m_pUpdatesCallback;
    m_pUpdatesCallback = NULL;
    ReleaseInterface(pTemp);
}

HRESULT VirtualSurfaceImageSource::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    HRESULT hr = S_OK;

    if (InlineIsEqualGUID(iid, __uuidof(ISurfaceImageSourceNative)))
    {
        IFC_NOTRACE(CheckThread());
        *ppObject = static_cast<IVirtualSurfaceImageSourceNative *>(this);
    }
    else if (InlineIsEqualGUID(iid, __uuidof(IVirtualSurfaceImageSourceNative)))
    {
        IFC_NOTRACE(CheckThread());
        *ppObject = static_cast<IVirtualSurfaceImageSourceNative *>(this);
    }
    else
    {
        RRETURN(DirectUI::VirtualSurfaceImageSourceGenerated::QueryInterfaceImpl(iid, ppObject));
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
VirtualSurfaceImageSource::SetDevice(_In_ IDXGIDevice* pDevice)
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
VirtualSurfaceImageSource::BeginDraw(_In_ RECT updateRect, _Outptr_ IDXGISurface** ppSurface, _Out_ POINT* pOffset)
{
    HRESULT hr = S_OK;

    IUnknown *pUnknown = NULL;
    XPOINT xpoint = {};
    XRECT xrect = {};

    IFC(CheckThread());

    xrect.X = updateRect.left;
    xrect.Y = updateRect.top;
    xrect.Width = updateRect.right - updateRect.left;
    xrect.Height = updateRect.bottom - updateRect.top;

    // Calls the same C function as SurfaceImageSource.  The implementation at that side will handle the virtual pointer call
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
VirtualSurfaceImageSource::EndDraw()
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    // Calls the same C function as SurfaceImageSource.  The implementation at that side will handle the virtual pointer call
    IFC(CoreImports::SurfaceImageSource_EndDraw(static_cast<CSurfaceImageSource*>(GetHandle())));


Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//     Invalidates the given rectangle
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::Invalidate(_In_ RECT updateRect)
{
    HRESULT hr = S_OK;
    XRECT xrect = {};

    IFC(CheckThread());

    xrect.X = updateRect.left;
    xrect.Y = updateRect.top;
    xrect.Width = updateRect.right - updateRect.left;
    xrect.Height = updateRect.bottom - updateRect.top;

    IFC(CoreImports::VirtualSurfaceImageSource_Invalidate(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), xrect));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Gets the regions that need to be redrawn
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::GetUpdateRectCount(_Out_ DWORD *pCount)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    IFC(CoreImports::VirtualSurfaceImageSource_GetUpdateRectCount(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), pCount));

Cleanup:

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Gets the regions that need to be redrawn
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::GetUpdateRects(_Out_writes_(count) RECT *pRects,_In_ DWORD count)
{
    HRESULT hr = S_OK;
    XRECT *pXRects = NULL;

    IFC(CheckThread());

    pXRects = new XRECT[count];


    // This call will fill in no more that count pXRects.  If fewer are available, the call
    // should return empty XRECTs.
    IFC(CoreImports::VirtualSurfaceImageSource_GetUpdateRects(static_cast<CVirtualSurfaceImageSource*>(GetHandle()),pXRects, count));

    // Copy the internal representation into the external representation
    for(DWORD i = 0; i < count; i++)
    {
        pRects[i].left = pXRects[i].X;
        pRects[i].top = pXRects[i].Y;
        pRects[i].right = pRects[i].left + pXRects[i].Width;
        pRects[i].bottom = pRects[i].top + pXRects[i].Height;
    }


Cleanup:
    delete[] pXRects;

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Gets the rectangular region currently visible from the virtual space
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::GetVisibleBounds(_Out_ RECT *pBounds)
{
    HRESULT hr = S_OK;
    XRECT_RB xBounds = {};

    IFC(CheckThread());

    IFC(CoreImports::VirtualSurfaceImageSource_GetVisibleBounds(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), &xBounds));

    pBounds->left = xBounds.left;
    pBounds->top = xBounds.top;
    pBounds->right = xBounds.right;
    pBounds->bottom = xBounds.bottom;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Change the virtual size
//
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::Resize(_In_ INT newWidth, _In_ INT newHeight)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    if (newWidth < 0 || newHeight < 0)
    {
        IFC(E_INVALIDARG);
    }

    IFC(CoreImports::VirtualSurfaceImageSource_Resize(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), newWidth, newHeight));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Registers to get notification when the VirtualSurface needs more regions to be drawn
//  Also unregisters.  If the application code is holding a strong ref to this VirtualSurfaceImageSource, then
// we may be introducing a ref-counting loop because we AddRef the interface they pass in.
//-------------------------------------------------------------------------
IFACEMETHODIMP
VirtualSurfaceImageSource::RegisterForUpdatesNeeded(_In_opt_ IVirtualSurfaceUpdatesCallbackNative *pNewCallback)
{
    HRESULT hr = S_OK;
    IVirtualSurfaceUpdatesCallbackNative *pOldCallback = m_pUpdatesCallback;

    IFC(CheckThread());

    // Let's avoid calling Release and then Add again just because the user passed the same
    // value in
    if (m_pUpdatesCallback == pNewCallback)
        return S_OK;

    // Clear this out before we ReleaseInterface the current callback so that if during Release() the user
    // code does something, they don't get called back on the interface they are getting rid of.
    m_pUpdatesCallback = NULL;
    // Now release the old one
    ReleaseInterface(pOldCallback);
    // Addref the new one and set it
    AddRefInterface(pNewCallback);
    m_pUpdatesCallback = pNewCallback;

    if (m_pUpdatesCallback != NULL)
    {
        // We pass "this" as the last parameter because we implement IVirtualSurfaceImageSourceCallbacks on ourselves
        IFC(CoreImports::VirtualSurfaceImageSource_RegisterCallbacks(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), this));
    }
    else
    {
        IFC(CoreImports::VirtualSurfaceImageSource_RegisterCallbacks(static_cast<CVirtualSurfaceImageSource*>(GetHandle()), NULL));
    }

Cleanup:
    RRETURN(hr);
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//     Overrides method from IVirtualSurfaceImageSourceCallbacks and forwards
// to public interface ISurfaceUpdatesCallbackNative::UpdatesNeeded
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
VirtualSurfaceImageSource::UpdatesNeeded()
{
    HRESULT hr = S_OK;

    IFC(CheckThread()); // This is only called internally, but we should make sure we're on the right thread

    if (m_pUpdatesCallback != NULL)
    {
        // Don't fall over just because user code returned an error
        // TODO:  Think about whether this is correct.  Are we just eating errors?
        IGNOREHR(m_pUpdatesCallback->UpdatesNeeded());
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Creates a new VirtualSurfaceImageSource with the specified dimensions
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT VirtualSurfaceImageSourceFactory::CreateInstanceWithDimensionsAndOpacityImpl(
    _In_ INT pixelWidth,
    _In_ INT pixelHeight,
    _In_ BOOLEAN isOpaque,
    _Outptr_ xaml_imaging::IVirtualSurfaceImageSource** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<VirtualSurfaceImageSource> spVirtualSurfaceImageSource;

    IFCPTR(ppInstance);

    IFC(CheckActivationAllowed());

    if (pixelWidth < 0 || pixelHeight < 0)
    {
        IFC(E_INVALIDARG);
    }

    // Create the VirtualSurfaceImageSource.
    IFC(ctl::make(&spVirtualSurfaceImageSource));

    // Call into core for initialization.
    IFC(CoreImports::VirtualSurfaceImageSource_Initialize(
        static_cast<CVirtualSurfaceImageSource*>(spVirtualSurfaceImageSource->GetHandle()),
        pixelWidth,
        pixelHeight,
        !!isOpaque));

    *ppInstance = spVirtualSurfaceImageSource.Detach();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Creates a new VirtualSurfaceImageSource with the specified dimensions.   Assumed to
// contain transparent content
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT VirtualSurfaceImageSourceFactory::CreateInstanceWithDimensionsImpl(
    _In_ INT pixelWidth,
    _In_ INT pixelHeight,
    _Outptr_ xaml_imaging::IVirtualSurfaceImageSource** ppInstance)
{
    RRETURN(CreateInstanceWithDimensionsAndOpacity(pixelWidth, pixelHeight, FALSE, ppInstance));
}


