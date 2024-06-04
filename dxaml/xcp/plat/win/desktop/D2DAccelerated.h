// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "palgfx.h"
#include <d2d1_1.h>
#include <DWrite_3.h>
#include "xref_ptr.h"

typedef HRESULT (WINAPI *D2D1CreateFactoryFunc)(
  _In_      D2D1_FACTORY_TYPE factoryType,
  _In_      REFIID riid,
  _In_opt_  const D2D1_FACTORY_OPTIONS *pFactoryOptions,
  _Out_     void **ppIFactory
);

typedef HRESULT (WINAPI *DWriteCreateFactoryFunc)(
  _In_   DWRITE_FACTORY_TYPE factoryType,
  _In_   REFIID iid,
  _Out_  IUnknown **factory
);

class CD2DSurfaceRenderTarget;

class CD2DFactory final : public CXcpObjectBase<IPALAcceleratedGraphicsFactory>
{
public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CD2DFactory **ppD2DFactory
        );

    //
    // IPALAcceleratedGraphicsFactory
    //
    _Check_return_ HRESULT CreateStrokeStyle(
        LineCapStyle startCap,
        LineCapStyle endCap,
        LineCapStyle dashCap,
        LineJoin lineJoin,
        XFLOAT fMiterLimit,
        DashStyle dashStyle,
        XFLOAT fDashOffset,
        _In_opt_ const std::vector<float>* dashes,
        _Outptr_ IPALStrokeStyle **ppPALStrokeStyle
        ) override;

    _Check_return_ HRESULT CreateRectangleGeometry(
        _In_ const XRECTF &rect,
        _Outptr_ IPALAcceleratedGeometry **ppGeometry
        ) override;

    _Check_return_ HRESULT CreateRoundedRectangleGeometry(
        _In_ const XRECTF &rect,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppGeometry
        ) override;

    _Check_return_ HRESULT CreateEllipseGeometry(
        _In_ const XPOINTF &center,
        XFLOAT rRadiusX,
        XFLOAT rRadiusY,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) override;

    _Check_return_ HRESULT CreatePathGeometry(
        _Outptr_ IPALAcceleratedPathGeometry **ppPALGeometry
        ) override;

    _Check_return_ HRESULT CreateGeometryGroup(
        bool fIsWinding,
        _In_reads_(uiCount) IPALAcceleratedGeometry **ppPALGeometries,
        XUINT32 uiCount,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) override;

    _Check_return_ HRESULT CreateTransformedGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry,
        _In_ const CMILMatrix* pMatrix,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) override;

    _Check_return_ HRESULT CombineGeometry(
        _In_ IPALAcceleratedGeometry *pGeometry1,
        _In_ IPALAcceleratedGeometry *pGeometry2,
        GeometryCombineMode combineMode,
        _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
        ) override;

    //
    // CD2DFactory
    //
    ID2D1Factory1* GetFactory() { return m_pFactory; }

private:
    CD2DFactory();
    ~CD2DFactory() override;

    _Check_return_ HRESULT Initialize();

    ID2D1Factory1 *m_pFactory{};

    HMODULE m_D2DModule{};
};
