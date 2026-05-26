// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"
#include <d2d1_1.h>
#include <d2d1helper.h>

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a CD2DFactory
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CD2DFactory::Create(
    _Outptr_ CD2DFactory **ppD2DFactory
    )
{
    HRESULT hr = S_OK;
    CD2DFactory* pD2DFactory = new CD2DFactory();

    IFC(pD2DFactory->Initialize());

    *ppD2DFactory = pD2DFactory;
    pD2DFactory = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pD2DFactory);

    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------
CD2DFactory::CD2DFactory()
    : m_pFactory(NULL)
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------
CD2DFactory::~CD2DFactory()
{
    ReleaseInterface(m_pFactory);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the ID2D1Factory and IDWriteFactory.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::Initialize()
{
    HRESULT hr = S_OK;
    D2D1CreateFactoryFunc pCreateFactory = NULL;
    D2D1_FACTORY_OPTIONS FactoryOptions;
    ID2D1Factory *pD2DFactory = NULL;

    ZeroMemory(&FactoryOptions, sizeof(FactoryOptions));

    // Create D2D factory
    IFCW32(m_D2DModule = LoadLibraryEx(L"d2d1.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32));

    pCreateFactory = reinterpret_cast<D2D1CreateFactoryFunc>(GetProcAddress(m_D2DModule, "D2D1CreateFactory"));
    IFCPTR(pCreateFactory);

    // D2DPrintTarget also uses CD2DFactory and requires it to be multi-threaded.
    // If we want to make the render factory single threaded, add a parameter to Create
    // to allow D2DPrintTarget to continue creating a multi-threaded factory here.
    IFC(pCreateFactory(
        D2D1_FACTORY_TYPE_MULTI_THREADED,
        __uuidof(ID2D1Factory),
        &FactoryOptions,
        reinterpret_cast<void **>(&pD2DFactory)
        ));

    IFC(pD2DFactory->QueryInterface(
        __uuidof(ID2D1Factory1),
        reinterpret_cast<void **>(&m_pFactory)
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pD2DFactory);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a stroke style.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateStrokeStyle(
    LineCapStyle startCap,
    LineCapStyle endCap,
    LineCapStyle dashCap,
    LineJoin lineJoin,
    XFLOAT fMiterLimit,
    DashStyle dashStyle,
    XFLOAT fDashOffset,
    _In_opt_ const std::vector<float>* dashes,
    _Outptr_ IPALStrokeStyle **ppPALStrokeStyle
    )
{
    HRESULT hr = S_OK;
    ID2D1StrokeStyle *pD2DStrokeStyle = NULL;
    IPALStrokeStyle *pPALStrokeStyle = NULL;
    D2D1_STROKE_STYLE_PROPERTIES strokeStyleProperties = D2D1::StrokeStyleProperties(
        static_cast<D2D1_CAP_STYLE>(startCap),
        static_cast<D2D1_CAP_STYLE>(endCap),
        static_cast<D2D1_CAP_STYLE>(dashCap),
        static_cast<D2D1_LINE_JOIN>(lineJoin),
        fMiterLimit,
        static_cast<D2D1_DASH_STYLE>(dashStyle),
        fDashOffset
        );

#pragma warning(push)
#pragma warning(disable:4267) //'var' : conversion from 'size_t' to 'type', possible loss of data
    IFC(m_pFactory->CreateStrokeStyle(
        &strokeStyleProperties,
        dashes != nullptr ? dashes->data() : nullptr,
        dashes != nullptr ? dashes->size() : 0,
        &pD2DStrokeStyle
        ));
#pragma warning(pop)

    IFC(CD2DStrokeStyle::Create(pD2DStrokeStyle, &pPALStrokeStyle));

    *ppPALStrokeStyle = pPALStrokeStyle;
    pPALStrokeStyle = NULL;

Cleanup:
    ReleaseInterface(pD2DStrokeStyle);
    ReleaseInterface(pPALStrokeStyle);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a rectangle geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateRectangleGeometry(
    _In_ const XRECTF &rect,
    _Outptr_ IPALAcceleratedGeometry **ppRectangleGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1RectangleGeometry *pD2DGeometry = NULL;
    IPALAcceleratedGeometry *pPALGeometry = NULL;

    XRECTF_RB rect_rb =
    {
        rect.X,
        rect.Y,
        rect.X + rect.Width,
        rect.Y + rect.Height
    };

    IFC(m_pFactory->CreateRectangleGeometry(
        PALToD2DRectF(&rect_rb),
        &pD2DGeometry
        ));

    IFC(CD2DRectangleGeometry::Create(
        pD2DGeometry,
        rect_rb,
        &pPALGeometry
        ));

    *ppRectangleGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a rounded rectangle geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateRoundedRectangleGeometry(
    _In_ const XRECTF &rect,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _Outptr_ IPALAcceleratedGeometry **ppGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1RoundedRectangleGeometry *pD2DGeometry = NULL;
    IPALAcceleratedGeometry *pPALGeometry = NULL;

    XRECTF_RB rect_rb =
    {
        rect.X,
        rect.Y,
        rect.X + rect.Width,
        rect.Y + rect.Height
    };

    D2D1_ROUNDED_RECT d2dRoundedRect = D2D1::RoundedRect(
        *PALToD2DRectF(&rect_rb),
        rRadiusX,
        rRadiusY
        );

    IFC(m_pFactory->CreateRoundedRectangleGeometry(
        &d2dRoundedRect,
        &pD2DGeometry
        ));

    IFC(CD2DRoundedRectangleGeometry::Create(
        pD2DGeometry,
        rect_rb,
        rRadiusX,
        rRadiusY,
        &pPALGeometry
        ));

    *ppGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an ellipse geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateEllipseGeometry(
    _In_ const XPOINTF &center,
    XFLOAT rRadiusX,
    XFLOAT rRadiusY,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1EllipseGeometry *pD2DGeometry = NULL;
    IPALAcceleratedGeometry *pPALGeometry = NULL;

    D2D1_ELLIPSE d2dEllipse = D2D1::Ellipse(
        *PALToD2DPointF(&center),
        rRadiusX,
        rRadiusY
        );

    IFC(m_pFactory->CreateEllipseGeometry(
        &d2dEllipse,
        &pD2DGeometry
        ));

    IFC(CD2DEllipseGeometry::Create(
        pD2DGeometry,
        center,
        rRadiusX,
        rRadiusY,
        &pPALGeometry
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a path geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreatePathGeometry(
    _Outptr_ IPALAcceleratedPathGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1PathGeometry *pD2DGeometry = NULL;
    IPALAcceleratedPathGeometry *pPALGeometry = NULL;

    IFC(m_pFactory->CreatePathGeometry(&pD2DGeometry));

    IFC(CD2DPathGeometry::Create(
        pD2DGeometry,
        &pPALGeometry
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a geometry group.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateGeometryGroup(
    bool fIsWinding,
    _In_reads_(uiCount) IPALAcceleratedGeometry **ppPALGeometries,
    XUINT32 uiCount,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1GeometryGroup *pD2DGeometry = NULL;
    IPALAcceleratedGeometry *pPALGeometry = NULL;
    ID2D1Geometry **ppD2DGeometries = NULL;
    D2D1_FILL_MODE d2dFillMode = fIsWinding ? D2D1_FILL_MODE_WINDING : D2D1_FILL_MODE_ALTERNATE;

    ppD2DGeometries = new ID2D1Geometry*[uiCount];

    memset(ppD2DGeometries, 0, uiCount * sizeof(ID2D1Geometry *));

    for (XUINT32 i = 0; i < uiCount; i++)
    {
        IFC(UnwrapD2DGeometry(ppPALGeometries[i], &ppD2DGeometries[i]));
    }

    IFC(m_pFactory->CreateGeometryGroup(
        d2dFillMode,
        ppD2DGeometries,
        uiCount,
        &pD2DGeometry
        ));

    IFC(CD2DGeometryGroup::Create(
        pD2DGeometry,
        &pPALGeometry
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    for (XUINT32 i = 0; i < uiCount; i++)
    {
        ReleaseInterface(ppD2DGeometries[i]);
    }
    ReleaseInterface(pD2DGeometry);
    delete[] ppD2DGeometries;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a transformed geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CreateTransformedGeometry(
    _In_ IPALAcceleratedGeometry *pGeometry,
    _In_ const CMILMatrix* pMatrix,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1Geometry *pUnwrappedGeometry = NULL;
    ID2D1TransformedGeometry *pD2DGeometry = NULL;
    IPALAcceleratedGeometry *pPALGeometry = NULL;

    IFC(UnwrapD2DGeometry(pGeometry, &pUnwrappedGeometry));

    IFC(m_pFactory->CreateTransformedGeometry(
        pUnwrappedGeometry,
        PALToD2DMatrix(pMatrix),
        &pD2DGeometry
        ));

    IFC(CD2DTransformedGeometry::Create(
        pGeometry,
        pMatrix,
        pD2DGeometry,
        &pPALGeometry
        ));

    *ppPALGeometry = pPALGeometry;
    pPALGeometry = NULL;

Cleanup:
    ReleaseInterface(pUnwrappedGeometry);
    ReleaseInterface(pD2DGeometry);
    ReleaseInterface(pPALGeometry);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Combines two geometry into a single geometry.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CD2DFactory::CombineGeometry(
    _In_ IPALAcceleratedGeometry *pGeometry1,
    _In_ IPALAcceleratedGeometry *pGeometry2,
    GeometryCombineMode combineMode,
    _Outptr_ IPALAcceleratedGeometry **ppPALGeometry
    )
{
    HRESULT hr = S_OK;
    ID2D1Geometry *pD2DGeometry1 = NULL;
    ID2D1Geometry *pD2DGeometry2 = NULL;
    ID2D1GeometrySink *pD2DGeometrySink = NULL;
    ID2D1PathGeometry *pD2DCombinedGeometry = NULL;
    IPALAcceleratedPathGeometry *pPALCombinedGeometry = NULL;

    IFC(UnwrapD2DGeometry(pGeometry1, &pD2DGeometry1));
    IFC(UnwrapD2DGeometry(pGeometry2, &pD2DGeometry2));

    IFC(m_pFactory->CreatePathGeometry(&pD2DCombinedGeometry));
    IFC(pD2DCombinedGeometry->Open(&pD2DGeometrySink));
    IFC(pD2DGeometry1->CombineWithGeometry(
        pD2DGeometry2,
        static_cast<D2D1_COMBINE_MODE>(combineMode),
        D2D1::IdentityMatrix(),
        pD2DGeometrySink
        ));
    IFC(pD2DGeometrySink->Close());

    IFC(CD2DPathGeometry::Create(
        pD2DCombinedGeometry,
        &pPALCombinedGeometry
        ));

    *ppPALGeometry = pPALCombinedGeometry;
    pPALCombinedGeometry = NULL;

Cleanup:
    ReleaseInterface(pD2DGeometry1);
    ReleaseInterface(pD2DGeometry2);
    ReleaseInterface(pD2DGeometrySink);
    ReleaseInterface(pD2DCombinedGeometry);
    ReleaseInterface(pPALCombinedGeometry);

    RRETURN(hr);
}
