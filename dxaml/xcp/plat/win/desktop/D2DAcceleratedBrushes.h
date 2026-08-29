// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <d2d1_1.h>

_Check_return_ HRESULT UnwrapD2DBrush(
    _In_ IPALAcceleratedBrush *pBrush,
    _Outptr_ ID2D1Brush **ppD2DBrush
    );

template<typename T>
class CD2DBrush : public CXcpObjectBase<T>
{
public:
    _Ret_notnull_ ID2D1Brush *GetD2DBrush()
    {
        return m_pBrush;
    }

    _Check_return_ HRESULT SetTransform(_In_ const CMILMatrix *pMatrix) override;
    _Check_return_ HRESULT SetOpacity(XFLOAT rOpacity) override;

protected:
    CD2DBrush() : m_pBrush(NULL)
    {
    }

    ~CD2DBrush() override
    {
        ReleaseInterface(m_pBrush);
    }

    _Check_return_ HRESULT Initialize(_In_ ID2D1Brush *pBrush);

    ID2D1Brush *m_pBrush;
};



class CD2DSolidColorBrush : public CD2DBrush<IPALAcceleratedSolidColorBrush>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1SolidColorBrush *pD2DBrush,
        _Outptr_ CD2DSolidColorBrush **ppPALBrush
        );

    BrushType GetType() override;
    XUINT32 GetColor() override;

protected:
    CD2DSolidColorBrush();
    ~CD2DSolidColorBrush() override;
};



class CD2DBitmapBrush : public CD2DBrush<IPALAcceleratedBitmapBrush>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1BitmapBrush *pD2DBrush,
        _Outptr_ CD2DBitmapBrush **ppPALBrush
        );

    BrushType GetType() override;

protected:
    CD2DBitmapBrush();
    ~CD2DBitmapBrush() override;
};



class CD2DLinearGradientBrush : public CD2DBrush<IPALAcceleratedLinearGradientBrush>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1LinearGradientBrush *pD2DBrush,
        _Outptr_ CD2DLinearGradientBrush **ppPALBrush
        );

    BrushType GetType() override;

protected:
    CD2DLinearGradientBrush();
    ~CD2DLinearGradientBrush() override;
};



class CD2DRadialGradientBrush : public CD2DBrush<IPALAcceleratedLinearGradientBrush>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ID2D1RadialGradientBrush *pD2DBrush,
        _Outptr_ CD2DRadialGradientBrush **ppPALBrush
        );

    BrushType GetType() override;

protected:
    CD2DRadialGradientBrush();
    ~CD2DRadialGradientBrush() override;
};
