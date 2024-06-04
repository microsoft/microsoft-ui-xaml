// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"
#include "DWriteFontFace.h"
#include <d2d1_1.h>
#include <d2d1helper.h>
#include <GraphicsUtility.h>
#include "TextDrawingContext.h"
#include "D2DTextDrawingContext.h"

template class CD2DRenderTarget<IPALPrintTarget>;
template class CD2DRenderTarget<IPALAcceleratedRenderTarget>;

//------------------------------------------------------------------------
//
//  Synopsis:
//      An implementation of IPALAcceleratedBitmap that wraps an ID2D1Bitmap
//
//------------------------------------------------------------------------
class CD2DBitmap :  public CXcpObjectBase< IPALAcceleratedBitmap >
{
private:
    CD2DBitmap(
        _In_ ID2D1Bitmap *pBitmap
        )
    {
        SetInterface(m_pD2DBitmap, pBitmap);
    }

    ~CD2DBitmap() override
    {
        ReleaseInterface(m_pD2DBitmap);
    }

public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1Bitmap *pD2DBitmap,
        _Outptr_ CD2DBitmap **ppSurface
        )
    {
        HRESULT hr = S_OK;

        CD2DBitmap *pSurface = new CD2DBitmap(pD2DBitmap);

        *ppSurface = pSurface;
        RRETURN(hr);//RRETURN_REMOVAL
    }

    ID2D1Bitmap *GetBitmap()
    {
        return m_pD2DBitmap;
    }

    //
    // IPALAcceleratedBitmap
    //
    XUINT32 GetWidth() override { return m_pD2DBitmap->GetPixelSize().width; }
    XUINT32 GetHeight() override { return m_pD2DBitmap->GetPixelSize().height; }

private:
    _Notnull_ ID2D1Bitmap *m_pD2DBitmap;
};

//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
template<typename T>
CD2DRenderTarget<T>::CD2DRenderTarget()
    : m_pD2DDeviceContext(NULL)
    , m_pFactory(NULL)
    , m_isEmpty(TRUE)
{
}

template<typename T>
CD2DRenderTarget<T>::CD2DRenderTarget(
    ID2D1DeviceContext* pD2DDeviceContext
    )
    : m_pFactory(nullptr)
    , m_isEmpty(TRUE)
{
    SetInterface(m_pD2DDeviceContext, pD2DDeviceContext);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
template<typename T>
CD2DRenderTarget<T>::~CD2DRenderTarget()
{
    ReleaseInterface(m_pD2DDeviceContext);
    ReleaseInterface(m_pFactory);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls BeginDraw on the D2D render target.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::BeginDraw()
{
    // There should be no clip layers pushed at BeginDraw
    ASSERT(0 == m_clipLayerStack.Size());

    m_pD2DDeviceContext->BeginDraw();

    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls EndDraw on the D2D render target. Waits until the surface
//      has been updated.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::EndDraw()
{
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(
        m_pD2DDeviceContext->EndDraw());

    // Clear any pending clip layers
    m_clipLayerStack.Clear();

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the width of the render target.
//
//------------------------------------------------------------------------
template<typename T>
XUINT32
CD2DRenderTarget<T>::GetWidth()
{
    return 0;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the height of the render target.
//
//------------------------------------------------------------------------
template<typename T>
XUINT32
CD2DRenderTarget<T>::GetHeight()
{
    return 0;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a bitmap that D2D can use.  Pixels are copied from the source surface
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateBitmap(
    _In_ IPALSurface *pBitmapSurface,
    _Outptr_ IPALAcceleratedBitmap **ppPALBitmap
    )
{
    HRESULT hr = S_OK;
    D2D1_SIZE_U bitmapSize;
    void *pSWBits = NULL;
    XINT32 stride;
    ID2D1Bitmap *pD2DBitmap = NULL;
    bool bitmapIsLocked = false;
    CD2DBitmap *pBitmapWrapper = NULL;

    D2D1_BITMAP_PROPERTIES bitmapProperties = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
        96.0f,
        96.0f
        );

    IFC(pBitmapSurface->Lock(
        &pSWBits,
        &stride,
        &bitmapSize.width,
        &bitmapSize.height
        ));
    bitmapIsLocked = TRUE;

    IFC(m_pD2DDeviceContext->CreateBitmap(
        bitmapSize,
        pSWBits,
        static_cast<XUINT32>(stride),
        &bitmapProperties,
        &pD2DBitmap
        ));

    IFC(pBitmapSurface->Unlock());
    bitmapIsLocked = FALSE;

    IFC(CD2DBitmap::Create(pD2DBitmap, &pBitmapWrapper));

    *ppPALBitmap = pBitmapWrapper;
    pBitmapWrapper = NULL;

Cleanup:
    if (bitmapIsLocked)
    {
        VERIFYHR(pBitmapSurface->Unlock());
    }

    ReleaseInterface(pD2DBitmap);

    ReleaseInterface(pBitmapWrapper);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a solid color brush.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateSolidColorBrush(
    XUINT32 uiColor,
    XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    ID2D1SolidColorBrush *pD2DBrush = NULL;
    CD2DSolidColorBrush *pPALBrush = NULL;

    // uiColor may include alpha, but D2D1::ColorF will just read the rgb. So combine the alpha in uiColor with rOpacity.
    rOpacity *= static_cast<XFLOAT>((uiColor >> 24) & 0xff) / 255.0f;

    IFC(m_pD2DDeviceContext->CreateSolidColorBrush(
        D2D1::ColorF(uiColor),
        D2D1::BrushProperties(rOpacity),
        &pD2DBrush
        ));

    IFC(CD2DSolidColorBrush::Create(pD2DBrush, &pPALBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a bitmap brush.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateBitmapBrush(
    _In_ IPALSurface *pBitmapSurface,
    XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    IPALAcceleratedBitmap *pD2DBitmap = NULL;
    IPALAcceleratedBrush *pPALBrush = NULL;

    //
    // Create a D2D-accesible surface (copying pixels from the pBitmapSurface)
    //
    IFC(CreateBitmap(
        pBitmapSurface,
        &pD2DBitmap
        ));

    //
    // Create a bitmap brush that points to the surface
    //
    IFC(CreateSharedBitmapBrush(
        pD2DBitmap,
        rOpacity,
        &pPALBrush
        ));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pD2DBitmap);
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateBitmapBrush(
    _In_ ID2D1SvgDocument* d2dSvgDocument,
    XFLOAT opacity,
    uint32_t width,
    uint32_t height,
    _Outptr_ IPALAcceleratedBrush** palBrushOut
    )
{
    xref_ptr<IPALAcceleratedBrush> palBrush;
    wrl::ComPtr<ID2D1Bitmap1> d2d1Bitmap;
    wrl::ComPtr<ID2D1Image> oldTarget;
    xref_ptr<CD2DBitmap> bitmapWrapper;

    D2D1_SIZE_U bitmapSize;
    bitmapSize.width = width;
    bitmapSize.height = height;

    D2D1_BITMAP_PROPERTIES1 bitmapProperties =
        D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    IFC_RETURN(m_pD2DDeviceContext->CreateBitmap(
        bitmapSize,
        0,
        0,
        &bitmapProperties,
        d2d1Bitmap.ReleaseAndGetAddressOf()
        ));

    m_pD2DDeviceContext->GetTarget(oldTarget.ReleaseAndGetAddressOf());
    m_pD2DDeviceContext->SetTarget(d2d1Bitmap.Get());
    m_pD2DDeviceContext->Clear();

    ctl::ComPtr<ID2D1DeviceContext5> d2dDeviceContext5;
    IFCFAILFAST(ctl::do_query_interface(d2dDeviceContext5, m_pD2DDeviceContext));

    d2dDeviceContext5->DrawSvgDocument(d2dSvgDocument);

    m_pD2DDeviceContext->SetTarget(oldTarget.Get());
    IFC_RETURN(CD2DBitmap::Create(d2d1Bitmap.Get(), bitmapWrapper.ReleaseAndGetAddressOf()));

    IFC_RETURN(CreateSharedBitmapBrush(
        bitmapWrapper.get(),
        opacity,
        palBrush.ReleaseAndGetAddressOf()
        ));

    *palBrushOut = palBrush.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a bitmap brush using the same bitmap as another bitmap
//      brush but with a different opacity. Not implemented for Jupiter.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateBitmapBrush(
    _In_ IPALAcceleratedBitmapBrush *pPALBitmapBrush,
        XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    // This method is used by CVideoBrush, which doesn't exist in Jupiter.
    ASSERT(FALSE);

    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a bitmap brush pointing to a pre-existing D2D bitmap
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateSharedBitmapBrush(
    _In_ IPALAcceleratedBitmap *pInputBitmap,
    XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;

    CD2DBitmapBrush *pPALBrush = NULL;
    CD2DBitmap *pD2DBitmap = static_cast<CD2DBitmap *>(pInputBitmap);
    ID2D1BitmapBrush *pD2DBrush = NULL;

    IFC(m_pD2DDeviceContext->CreateBitmapBrush(
        pD2DBitmap->GetBitmap(),
        D2D1::BitmapBrushProperties(),
        D2D1::BrushProperties(rOpacity),
        &pD2DBrush
        ));

    IFC(CD2DBitmapBrush::Create(pD2DBrush, &pPALBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pPALBrush);
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a linear gradient brush.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateLinearGradientBrush(
    XPOINTF startPoint,
    XPOINTF endPoint,
    _In_reads_(gradientStopsCount) XcpGradientStop *pGradientStopsArray,
    XUINT32 gradientStopsCount,
    InterpolationMode interpolationMode,
    GradientWrapMode gradientWrapMode,
    XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    ID2D1LinearGradientBrush *pD2DBrush = NULL;
    CD2DLinearGradientBrush *pPALBrush = NULL;

    D2D1_LINEAR_GRADIENT_BRUSH_PROPERTIES linearGradientBrushProperties = D2D1::LinearGradientBrushProperties(
        *PALToD2DPointF(&startPoint),
        *PALToD2DPointF(&endPoint)
        );
    ID2D1GradientStopCollection *pGradientStopCollection = NULL;
    D2D1_BRUSH_PROPERTIES brushProperties = D2D1::BrushProperties(rOpacity);

    IFC(m_pD2DDeviceContext->CreateGradientStopCollection(
        PALToD2DGradientStops(pGradientStopsArray),
        gradientStopsCount,
        static_cast<D2D1_GAMMA>(interpolationMode),
        static_cast<D2D1_EXTEND_MODE>(gradientWrapMode),
        &pGradientStopCollection
        ));

    IFC(m_pD2DDeviceContext->CreateLinearGradientBrush(
        &linearGradientBrushProperties,
        &brushProperties,
        pGradientStopCollection,
        &pD2DBrush
        ));

    IFC(CD2DLinearGradientBrush::Create(pD2DBrush, &pPALBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a radial gradient brush.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::CreateRadialGradientBrush(
    XPOINTF center,
    XPOINTF gradientOriginOffset,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _In_reads_(gradientStopsCount) XcpGradientStop *pGradientStopsArray,
    XUINT32 gradientStopsCount,
    InterpolationMode interpolationMode,
    GradientWrapMode gradientWrapMode,
    XFLOAT rOpacity,
    _Outptr_ IPALAcceleratedBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    ID2D1RadialGradientBrush *pD2DBrush = NULL;
    CD2DRadialGradientBrush *pPALBrush = NULL;
    D2D1_BRUSH_PROPERTIES brushProperties = D2D1::BrushProperties(rOpacity);

    D2D1_RADIAL_GRADIENT_BRUSH_PROPERTIES radialGradientBrushProperties = D2D1::RadialGradientBrushProperties(
        *PALToD2DPointF(&center),
        *PALToD2DPointF(&gradientOriginOffset),
        rRadiusX,
        rRadiusY
        );
    ID2D1GradientStopCollection *pGradientStopCollection = NULL;

    IFC(m_pD2DDeviceContext->CreateGradientStopCollection(
        PALToD2DGradientStops(pGradientStopsArray),
        gradientStopsCount,
        static_cast<D2D1_GAMMA>(interpolationMode),
        static_cast<D2D1_EXTEND_MODE>(gradientWrapMode),
        &pGradientStopCollection
        ));

    IFC(m_pD2DDeviceContext->CreateRadialGradientBrush(
        &radialGradientBrushProperties,
        &brushProperties,
        pGradientStopCollection,
        &pD2DBrush
        ));

    IFC(CD2DRadialGradientBrush::Create(pD2DBrush, &pPALBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the transform.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::SetTransform(
    _In_ const CMILMatrix *pMatrix
    )
{
    HRESULT hr = S_OK;

    // The clip layer stack only handles cases
    // where the clip layer was pushed with the same world
    // transform as the shapes that are drawn
    // If the transform changes, then a real D2D layer needs to be used
    // A future potential optimization would be to save the transform and not resolve the clip stack
    // if the new transform preserves axis-alignment
    ResolveClipStack();

    m_pD2DDeviceContext->SetTransform(PALToD2DMatrix(pMatrix));

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a geometry.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::FillGeometry(
    _In_ IPALAcceleratedGeometry *pGeometry,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    ID2D1Geometry *pD2DGeometry = NULL;
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    IFC(UnwrapD2DGeometry(pGeometry, &pD2DGeometry));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->FillGeometry(
        pD2DGeometry,
        pD2DBrush
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Strokes a geometry.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::DrawGeometry(
    _In_ IPALAcceleratedGeometry *pGeometry,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    ID2D1Geometry *pD2DGeometry = NULL;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    IFC(UnwrapD2DGeometry(pGeometry, &pD2DGeometry));

    IFC(UnwrapD2DStrokeStyle(pStrokeStyle, &pD2DStrokeStyle));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->DrawGeometry(
        pD2DGeometry,
        pD2DBrush,
        rStrokeWidth,
        pD2DStrokeStyle
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pD2DStrokeStyle);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a rectangle.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::FillRectangle(
    _In_ const XRECTF_RB &rect,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush* pD2DBrush = NULL;
    XFLOAT brushOpacity = 1.0f;

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    //
    // Clip the rect on the CPU if there are any clips pushed on the clip stack
    //
    if (m_clipLayerStack.Size() > 0)
    {
        XRECTF_RB rectToFill = rect;

        if (m_clipLayerStack.Intersect(&rectToFill))
        {
             m_pD2DDeviceContext->FillRectangle(
                PALToD2DRectF(&rectToFill),
                pD2DBrush
                );
        }
    }
    else
    {
        m_pD2DDeviceContext->FillRectangle(
            PALToD2DRectF(&rect),
            pD2DBrush
            );
    }

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Strokes a rectangle.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::DrawRectangle(
    _In_ const XRECTF_RB &rect,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush* pD2DBrush = NULL;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    IFC(UnwrapD2DStrokeStyle(pStrokeStyle, &pD2DStrokeStyle));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->DrawRectangle(
        PALToD2DRectF(&rect),
        pD2DBrush,
        rStrokeWidth,
        pD2DStrokeStyle
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pD2DStrokeStyle);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills a rounded rectangle.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::FillRoundedRectangle(
    _In_ const XRECTF_RB &rect,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
        *PALToD2DRectF(&rect),
        rRadiusX,
        rRadiusY
        );
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->FillRoundedRectangle(
        roundedRect,
        pD2DBrush
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Strokes a rounded rectangle.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::DrawRoundedRectangle(
    _In_ const XRECTF_RB &rect,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;
    D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
        *PALToD2DRectF(&rect),
        rRadiusX,
        rRadiusY
        );
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    IFC(UnwrapD2DStrokeStyle(pStrokeStyle, &pD2DStrokeStyle));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->DrawRoundedRectangle(
        roundedRect,
        pD2DBrush,
        rStrokeWidth,
        pD2DStrokeStyle
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pD2DStrokeStyle);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Fills an ellipse.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::FillEllipse(
    _In_ const XPOINTF &center,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        *PALToD2DPointF(&center),
        rRadiusX,
        rRadiusY
        );
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->FillEllipse(
        ellipse,
        pD2DBrush
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Strokes an ellipse.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::DrawEllipse(
    _In_ const XPOINTF &center,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT rStrokeWidth,
    _In_opt_ IPALStrokeStyle *pStrokeStyle,
    XFLOAT opacity
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush *pD2DBrush = NULL;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;
    D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        *PALToD2DPointF(&center),
        rRadiusX,
        rRadiusY
        );
    XFLOAT brushOpacity = 1.0f;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    IFC(UnwrapD2DStrokeStyle(pStrokeStyle, &pD2DStrokeStyle));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    m_pD2DDeviceContext->DrawEllipse(
        ellipse,
        pD2DBrush,
        rStrokeWidth,
        pD2DStrokeStyle
        );

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);
    ReleaseInterface(pD2DStrokeStyle);

    RRETURN(hr);
}

template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::DrawGlyphRun(
    _In_ const PALText::GlyphRun *pGlyphRun,
    _In_ IPALAcceleratedBrush *pBrush,
    XFLOAT opacity,
    UINT32 fontPaletteIndex,
    _In_ DWRITE_GLYPH_IMAGE_FORMATS glyphImageFormat
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush* pD2DBrush = NULL;
    DWRITE_GLYPH_RUN dwriteGlyphRun;
    DWriteFontFace *pDWriteFontFace = reinterpret_cast<DWriteFontFace*>(pGlyphRun->FontFace);
    XFLOAT brushOpacity = 1.0f;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext4> textureAtlasDeviceContext4;

    //
    // CPU-side clipping not supported
    //
    ResolveClipStack();

    dwriteGlyphRun.fontFace      = pDWriteFontFace->GetFontFace();
    dwriteGlyphRun.fontEmSize    = pGlyphRun->FontEmSize;
    dwriteGlyphRun.glyphCount    = pGlyphRun->GlyphCount;
    dwriteGlyphRun.glyphIndices  = pGlyphRun->GlyphIndices;
    dwriteGlyphRun.glyphAdvances = pGlyphRun->GlyphAdvances;
    dwriteGlyphRun.glyphOffsets  = reinterpret_cast<DWRITE_GLYPH_OFFSET const*>(pGlyphRun->GlyphOffsets);
    dwriteGlyphRun.isSideways    = pGlyphRun->IsSideways;
    dwriteGlyphRun.bidiLevel     = pGlyphRun->BidiLevel;

    IFC(UnwrapD2DBrush(pBrush, &pD2DBrush));

    if (opacity < 1.0f)
    {
        brushOpacity = pD2DBrush->GetOpacity();
        pD2DBrush->SetOpacity(brushOpacity * opacity);
    }

    IFC(m_pD2DDeviceContext->QueryInterface(IID_PPV_ARGS(&textureAtlasDeviceContext4)));

    D2DTextDrawingContext::RasterizeImageFormatGlyphRun(
        textureAtlasDeviceContext4.Get(),
        dwriteGlyphRun,
        glyphImageFormat,
        fontPaletteIndex,
        pD2DBrush);

    if (opacity < 1.0f)
    {
        pD2DBrush->SetOpacity(brushOpacity);
    }

    m_isEmpty = FALSE;

Cleanup:
    ReleaseInterface(pD2DBrush);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes a layer for applying opacity.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::PushOpacityLayer(
    XFLOAT rOpacity,
    _In_opt_ const XRECTF_RB *pContentBounds
    )
{
    ResolveClipStack();

    D2D1_RECT_F contentBounds = pContentBounds ? *PALToD2DRectF(pContentBounds) : D2D1::InfiniteRect();
    D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
        contentBounds,
        NULL,
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
        D2D1::IdentityMatrix(),
        rOpacity,
        NULL,
        D2D1_LAYER_OPTIONS_NONE
        );

    m_pD2DDeviceContext->PushLayer(
        layerParams,
        NULL /* layer - let D2D manage the layer pool */
        );

    RRETURN(S_OK);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes a layer for applying a clip.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::PushClipLayer(
    _In_ IPALAcceleratedGeometry *pPALClipGeometry,
    _In_opt_ const XRECTF_RB *pContentBounds
    )
{
    HRESULT hr = S_OK;
    ID2D1Geometry *pD2DClipGeometry = NULL;

    if (pPALClipGeometry->IsAxisAlignedRectangle())
    {
        // Defer pushing the clip layer
        // just remember that a clip layer should be pushed
        // This optimizes the case where clipping can occur trivially on the CPU
        XRECTF_RB clipRect;
        IFC(pPALClipGeometry->GetBounds(&clipRect));

        if (pContentBounds)
        {
            IntersectRect(&clipRect, pContentBounds);
        }

        IFC(m_clipLayerStack.Push(
            &clipRect
            ));
    }
    else
    {
        D2D1_LAYER_PARAMETERS layerParams;
        D2D1_RECT_F contentBounds = D2D1::InfiniteRect();

        //
        // Push any pending clip layers
        //
        ResolveClipStack();

        if (pContentBounds != NULL)
        {
            contentBounds = *PALToD2DRectF(pContentBounds);
        }

        IFC(UnwrapD2DGeometry(pPALClipGeometry, &pD2DClipGeometry));

        layerParams = D2D1::LayerParameters(
            contentBounds,
            pD2DClipGeometry,
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::IdentityMatrix(),
            1.0f,
            NULL,
            D2D1_LAYER_OPTIONS_NONE
            );

        m_pD2DDeviceContext->PushLayer(
            layerParams,
            NULL /* layer - let D2D manage the layer pool */
            );
    }

Cleanup:
    ReleaseInterface(pD2DClipGeometry);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops the most recently pushed layer.
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DRenderTarget<T>::PopLayer()
{
    HRESULT hr = S_OK;

    //
    // Pop the layer from either the pending clip stack or the D2D device context
    //
    if (m_clipLayerStack.Size() > 0)
    {
        IFC(m_clipLayerStack.Pop());
    }
    else
    {
        m_pD2DDeviceContext->PopLayer();
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pushes an axis aligned clip.
//
//------------------------------------------------------------------------
template<typename T>
void
CD2DRenderTarget<T>::PushAxisAlignedClip(
    _In_ const XRECTF_RB *pClip
    )
{
    //
    // Push any pending clip layers to ensure proper ordering
    // because layers and axis-aligned clips go onto the same stack in D2D
    //
    ResolveClipStack();

    m_pD2DDeviceContext->PushAxisAlignedClip(
        PALToD2DRectF(pClip),
        D2D1_ANTIALIAS_MODE_PER_PRIMITIVE   // The AA mode applies to the clip rather than the contents inside
        );
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops an axis aligned clip.
//
//------------------------------------------------------------------------
template<typename T>
void
CD2DRenderTarget<T>::PopAxisAlignedClip()
{
    //
    // Caller should have popped clip layers before calling this
    //
    ASSERT(0 == m_clipLayerStack.Size());

    m_pD2DDeviceContext->PopAxisAlignedClip();
}

template<typename T>
_Check_return_ HRESULT CD2DRenderTarget<T>::SetDxgiTarget(
    _In_ IDXGISurface* pDXGISurface
    )
{
    wrl::ComPtr<ID2D1Bitmap1> spD2DBitmap;
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(
        m_pD2DDeviceContext->CreateBitmapFromDxgiSurface(
            pDXGISurface,
            nullptr,
            spD2DBitmap.ReleaseAndGetAddressOf()
            )
        );

    m_pD2DDeviceContext->SetTarget(spD2DBitmap.Get());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the deferred clip layer stack by pushing
//      1 D2D clip layer for each element in the clip stack
//      A future potential optimization would be to combine multiple elements of the clip stack
//      into a single D2D layer.
//
//------------------------------------------------------------------------
template<typename T>
void
CD2DRenderTarget<T>::ResolveClipStack()
{
    if (m_clipLayerStack.Size() > 0)
    {
        for (XUINT32 i = 0; i < m_clipLayerStack.Size(); i++)
        {
            const XRECTF_RB& clipRect = m_clipLayerStack.Rect(i);

            D2D1_LAYER_PARAMETERS layerParams = D2D1::LayerParameters(
                *PALToD2DRectF(&clipRect),
                NULL, // pGeometricMask
                D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                D2D1::IdentityMatrix(),
                1.0f,
                NULL, // pOpacityBrush
                D2D1_LAYER_OPTIONS_NONE
                );

            m_pD2DDeviceContext->PushLayer(
                layerParams,
                NULL /* layer - let D2D manage the layer pool */
                );
        }

        m_clipLayerStack.Clear();
    }
}

XUINT32
CClipLayerStack::Size()
{
    return m_stack.size();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds another rect to the stack.  The resulting clip rect
//      is the intersection of the input with all currently pushed clip rects
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CClipLayerStack::Push(
    _In_ const XRECTF_RB* pInputRect
    )
{
    //
    // Intersect the clip rect with all of the clips that are currently pushed
    // This will set intersected = { 0,0,0,0 } if there is no intersection
    // The code that does this last part is inside of IntersectRect()
    //
    XRECTF_RB intersected = *pInputRect;

    Intersect(&intersected);

    RRETURN(m_stack.push_back(intersected));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops the top element from the stack.  Fails if the stack is empty
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CClipLayerStack::Pop()
{
    RRETURN(m_stack.pop_back());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pops all elements from the stack
//
//------------------------------------------------------------------------
void
CClipLayerStack::Clear()
{
    m_stack.clear();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//     Computes the intersection of the input rectangle with all rects on the stack
//     returns FALSE if the intersection has no area
//
//------------------------------------------------------------------------
bool
CClipLayerStack::Intersect(
    _Inout_ XRECTF_RB* pRect
    )
{
    if (m_stack.size() > 0)
    {
        const XRECTF_RB& topmost = Rect(m_stack.size() - 1);

        return IntersectRect(pRect, &topmost);
    }
    else
    {
        return true;
    }
}

const XRECTF_RB&
CClipLayerStack::Rect(
    XUINT32 index
    )
{
    ASSERT(index < m_stack.size());

    return m_stack[index];
}
