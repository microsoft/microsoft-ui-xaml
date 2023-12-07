// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"
#include <d2d1_1.h>
#include <d2d1helper.h>

_Check_return_ HRESULT UnwrapD2DBrush(
    _In_ IPALAcceleratedBrush *pBrush,
    _Outptr_ ID2D1Brush **ppD2DBrush
    )
{
    HRESULT hr = S_OK;
    ID2D1Brush* pD2DBrush = NULL;

    switch(pBrush->GetType())
    {
        case BrushType::Solid:
        {
            pD2DBrush = (reinterpret_cast<CD2DSolidColorBrush *>(pBrush))->GetD2DBrush();
            break;
        }

        case BrushType::Bitmap:
        {
            pD2DBrush = (reinterpret_cast<CD2DBitmapBrush *>(pBrush))->GetD2DBrush();
            break;
        }

        case BrushType::LinearGradient:
        {
            pD2DBrush = (reinterpret_cast<CD2DLinearGradientBrush *>(pBrush))->GetD2DBrush();
            break;
        }

        case BrushType::RadialGradient:
        {
            pD2DBrush = (reinterpret_cast<CD2DRadialGradientBrush *>(pBrush))->GetD2DBrush();
            break;
        }

        default:
        {
            IFC(E_FAIL);
        }
    }

    SetInterface(*ppD2DBrush, pD2DBrush);

Cleanup:
    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
template<typename T>
_Check_return_ HRESULT
CD2DBrush<T>::SetTransform(
    _In_ const CMILMatrix *pMatrix
    )
{
    HRESULT hr = S_OK;

    m_pBrush->SetTransform(PALToD2DMatrix(pMatrix));

    RRETURN(hr);
}

template<typename T>
_Check_return_ HRESULT
CD2DBrush<T>::SetOpacity(
    XFLOAT rOpacity
    )
{
    HRESULT hr = S_OK;

    m_pBrush->SetOpacity(rOpacity);

    RRETURN(hr);
}

template<typename T>
_Check_return_ HRESULT
CD2DBrush<T>::Initialize(
    _In_ ID2D1Brush *pBrush
    )
{
    HRESULT hr = S_OK;

    SetInterface(m_pBrush, pBrush);

    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DSolidColorBrush::Create(
    _In_ ID2D1SolidColorBrush *pD2DBrush,
    _Outptr_ CD2DSolidColorBrush** ppPALBrush
    )
{
    HRESULT hr = S_OK;
    CD2DSolidColorBrush* pPALBrush = NULL;

    pPALBrush = new CD2DSolidColorBrush();

    IFC(pPALBrush->Initialize(pD2DBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

CD2DSolidColorBrush::CD2DSolidColorBrush()
{
}

CD2DSolidColorBrush::~CD2DSolidColorBrush()
{
}

BrushType CD2DSolidColorBrush::GetType()
{
    return BrushType::Solid;
}

XUINT32 CD2DSolidColorBrush::GetColor()
{
    D2D1_COLOR_F Col = ((ID2D1SolidColorBrush*)m_pBrush)->GetColor();

    XBYTE A = (XBYTE)(Col.a * 0xFF);
    XBYTE R = (XBYTE)(Col.r * 0xFF);
    XBYTE G = (XBYTE)(Col.g * 0xFF);
    XBYTE B = (XBYTE)(Col.b * 0xFF);

    return (((XUINT32)A) << 24) |
           (((XUINT32)R) << 16) |
           (((XUINT32)G) <<  8) |
           ((XUINT32)(B) <<  0);
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DBitmapBrush::Create(
    _In_ ID2D1BitmapBrush *pD2DBrush,
    _Outptr_ CD2DBitmapBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    CD2DBitmapBrush* pPALBrush = NULL;

    pPALBrush = new CD2DBitmapBrush();

    IFC(pPALBrush->Initialize(pD2DBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

CD2DBitmapBrush::CD2DBitmapBrush()
{
}

CD2DBitmapBrush::~CD2DBitmapBrush()
{
}

BrushType CD2DBitmapBrush::GetType()
{
    return BrushType::Bitmap;
}

//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DLinearGradientBrush::Create(
    _In_ ID2D1LinearGradientBrush *pD2DBrush,
    _Outptr_ CD2DLinearGradientBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    CD2DLinearGradientBrush* pPALBrush = NULL;

    pPALBrush = new CD2DLinearGradientBrush();

    IFC(pPALBrush->Initialize(pD2DBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

CD2DLinearGradientBrush::CD2DLinearGradientBrush()
{
}

CD2DLinearGradientBrush::~CD2DLinearGradientBrush()
{
}

BrushType CD2DLinearGradientBrush::GetType()
{
    return BrushType::LinearGradient;
}



//------------------------------------------------------------------------
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CD2DRadialGradientBrush::Create(
    _In_ ID2D1RadialGradientBrush *pD2DBrush,
    _Outptr_ CD2DRadialGradientBrush **ppPALBrush
    )
{
    HRESULT hr = S_OK;
    CD2DRadialGradientBrush* pPALBrush = NULL;

    pPALBrush = new CD2DRadialGradientBrush();

    IFC(pPALBrush->Initialize(pD2DBrush));

    *ppPALBrush = pPALBrush;
    pPALBrush = NULL;

Cleanup:
    ReleaseInterface(pPALBrush);

    RRETURN(hr);
}

CD2DRadialGradientBrush::CD2DRadialGradientBrush()
{
}

CD2DRadialGradientBrush::~CD2DRadialGradientBrush()
{
}

BrushType CD2DRadialGradientBrush::GetType()
{
    return BrushType::RadialGradient;
}
