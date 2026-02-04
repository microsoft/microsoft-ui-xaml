// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "d2D1_2.h"
#include "MockD2D1.h"
#include "MockComObjectBase.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    //  MockD2D1ResourceBase - Base class for all mock D2D Resource objects.  It adds common methods to the MockComObjectBase

    template<class T>
    class MockD2D1ResourceBase : public MockComObjectBase<T>
    {
    public:
        MockD2D1ResourceBase(_In_ ID2D1Factory* factory) : m_factory(factory)
        {
        }

        MockD2D1ResourceBase()
        {
        }

        virtual ~MockD2D1ResourceBase()
        {
        }

        #pragma warning(suppress: 6387)
        STDMETHOD_(void, GetFactory)(_Outptr_ ID2D1Factory **factory) CONST
        {
            m_factory.CopyTo(factory);
        }

    private:
        Microsoft::WRL::ComPtr<ID2D1Factory> m_factory;
    };

    //  D2D1Device Mock

    class MockD2D1Device :
        public MockD2D1ResourceBase<IMockD2D1Device>
    {
    public:
        MockD2D1Device(ID2D1Factory* factory) : MockD2D1ResourceBase<IMockD2D1Device>(factory)
        {
        }

        virtual ~MockD2D1Device()
        {
        }

        //  ID2D1Device
        STDMETHOD(CreateDeviceContext)(D2D1_DEVICE_CONTEXT_OPTIONS options, _Outptr_ ID2D1DeviceContext **deviceContext)
        {
            UNREFERENCED_PARAMETER(options);
            UNREFERENCED_PARAMETER(deviceContext);
            return E_NOTIMPL;
        }

        STDMETHOD(CreatePrintControl)(_In_ IWICImagingFactory *wicFactory, _In_ IPrintDocumentPackageTarget *documentTarget, _In_opt_ CONST D2D1_PRINT_CONTROL_PROPERTIES *printControlProperties, _Outptr_ ID2D1PrintControl **printControl)
        {
            UNREFERENCED_PARAMETER(wicFactory);
            UNREFERENCED_PARAMETER(documentTarget);
            UNREFERENCED_PARAMETER(printControlProperties);
            UNREFERENCED_PARAMETER(printControl);
            return E_NOTIMPL;
        }

        STDMETHOD_(void, SetMaximumTextureMemory)(UINT64 maximumInBytes)
        {
            UNREFERENCED_PARAMETER(maximumInBytes);
            VERIFY_FAIL();
        }

        STDMETHOD_(UINT64, GetMaximumTextureMemory)() CONST
        {
            VERIFY_FAIL();
            return 0;
        }

            STDMETHOD_(void, ClearResources)(UINT32 millisecondsSinceUse = 0)
        {
            UNREFERENCED_PARAMETER(0);
            VERIFY_FAIL();
        }

    private:
    };

    //  D2D1Factory Mock

    class MockD2D1Factory :
        public MockComObjectBase<IMockD2D1Factory>,
        public MockAlsoImplementsComInterface<ID2D1Multithread>
    {
    public:
        MockD2D1Factory() : MockAlsoImplementsComInterface<ID2D1Multithread>((IMockD2D1Factory*)this)
        {
        }

        virtual ~MockD2D1Factory()
        {
        }

        virtual void* CastTo(REFIID iid)
        {
            if (iid == __uuidof(ID2D1Factory)) return static_cast<ID2D1Factory*>(this);
            if (iid == __uuidof(ID2D1Factory1)) return static_cast<ID2D1Factory1*>(this);
            if (iid == __uuidof(ID2D1Multithread)) return static_cast<ID2D1Multithread*>(this);
            return __super::CastTo(iid);
        }

        //  ID2D1Factory
        STDMETHOD(ReloadSystemMetrics)()
        {
            return E_NOTIMPL;
        }

        STDMETHOD_(void, GetDesktopDpi)(_Out_ FLOAT *dpiX, _Out_ FLOAT *dpiY)
        {
            *dpiX = 0.0f;
            *dpiY = 0.0f;
            VERIFY_FAIL();
        }

        STDMETHOD(CreateRectangleGeometry)(_In_ CONST D2D1_RECT_F *rectangle, _Outptr_ ID2D1RectangleGeometry **rectangleGeometry)
        {
            UNREFERENCED_PARAMETER(rectangle);
            UNREFERENCED_PARAMETER(rectangleGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateRoundedRectangleGeometry)(_In_ CONST D2D1_ROUNDED_RECT *roundedRectangle, _Outptr_ ID2D1RoundedRectangleGeometry **roundedRectangleGeometry)
        {
            UNREFERENCED_PARAMETER(roundedRectangle);
            UNREFERENCED_PARAMETER(roundedRectangleGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateEllipseGeometry)(_In_ CONST D2D1_ELLIPSE *ellipse, _Outptr_ ID2D1EllipseGeometry **ellipseGeometry)
        {
            UNREFERENCED_PARAMETER(ellipse);
            UNREFERENCED_PARAMETER(ellipseGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateGeometryGroup)(D2D1_FILL_MODE fillMode, _In_reads_(geometriesCount) ID2D1Geometry **geometries, UINT32 geometriesCount, _Outptr_ ID2D1GeometryGroup **geometryGroup)
        {
            UNREFERENCED_PARAMETER(geometries);
            UNREFERENCED_PARAMETER(geometriesCount);
            UNREFERENCED_PARAMETER(geometryGroup);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateTransformedGeometry)(_In_ ID2D1Geometry *sourceGeometry, _In_ CONST D2D1_MATRIX_3X2_F *transform, _Outptr_ ID2D1TransformedGeometry **transformedGeometry)
        {
            UNREFERENCED_PARAMETER(sourceGeometry);
            UNREFERENCED_PARAMETER(transform);
            UNREFERENCED_PARAMETER(transformedGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreatePathGeometry)(_Outptr_ ID2D1PathGeometry **pathGeometry)
        {
            UNREFERENCED_PARAMETER(pathGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateStrokeStyle)(_In_ CONST D2D1_STROKE_STYLE_PROPERTIES *strokeStyleProperties, _In_reads_opt_(dashesCount) CONST FLOAT *dashes, UINT32 dashesCount, _Outptr_ ID2D1StrokeStyle **strokeStyle)
        {
            UNREFERENCED_PARAMETER(dashes);
            UNREFERENCED_PARAMETER(dashesCount);
            UNREFERENCED_PARAMETER(strokeStyle);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateDrawingStateBlock)(_In_opt_ CONST D2D1_DRAWING_STATE_DESCRIPTION *drawingStateDescription, _In_opt_ IDWriteRenderingParams *textRenderingParams, _Outptr_ ID2D1DrawingStateBlock **drawingStateBlock)
        {
            UNREFERENCED_PARAMETER(drawingStateDescription);
            UNREFERENCED_PARAMETER(textRenderingParams);
            UNREFERENCED_PARAMETER(drawingStateBlock);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateWicBitmapRenderTarget)(_In_ IWICBitmap *target, _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, _Outptr_ ID2D1RenderTarget **renderTarget)
        {
            UNREFERENCED_PARAMETER(target);
            UNREFERENCED_PARAMETER(renderTargetProperties);
            UNREFERENCED_PARAMETER(renderTarget);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateHwndRenderTarget)(_In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, _In_ CONST D2D1_HWND_RENDER_TARGET_PROPERTIES *hwndRenderTargetProperties, _Outptr_ ID2D1HwndRenderTarget **hwndRenderTarget)
        {
            UNREFERENCED_PARAMETER(renderTargetProperties);
            UNREFERENCED_PARAMETER(hwndRenderTargetProperties);
            UNREFERENCED_PARAMETER(hwndRenderTarget);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateDxgiSurfaceRenderTarget)(_In_ IDXGISurface *dxgiSurface, _In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, _Outptr_ ID2D1RenderTarget **renderTarget)
        {
            UNREFERENCED_PARAMETER(dxgiSurface);
            UNREFERENCED_PARAMETER(renderTargetProperties);
            UNREFERENCED_PARAMETER(renderTarget);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateDCRenderTarget)(_In_ CONST D2D1_RENDER_TARGET_PROPERTIES *renderTargetProperties, _Outptr_ ID2D1DCRenderTarget **dcRenderTarget)
        {
            UNREFERENCED_PARAMETER(renderTargetProperties);
            UNREFERENCED_PARAMETER(dcRenderTarget);
            return E_NOTIMPL;
        }

        //  ID2D1Factory1
        STDMETHOD(CreateDevice)(_In_ IDXGIDevice *dxgiDevice, _Outptr_ ID2D1Device **d2dDevice)
        {
            UNREFERENCED_PARAMETER(dxgiDevice);
            *d2dDevice = (ID2D1Device*) new MockD2D1Device(this);
            return S_OK;
        }

        STDMETHOD(CreateStrokeStyle)(_In_ CONST D2D1_STROKE_STYLE_PROPERTIES1 *strokeStyleProperties, _In_reads_opt_(dashesCount) CONST FLOAT *dashes, UINT32 dashesCount, _Outptr_ ID2D1StrokeStyle1 **strokeStyle)
        {
            UNREFERENCED_PARAMETER(dashes);
            UNREFERENCED_PARAMETER(dashesCount);
            UNREFERENCED_PARAMETER(strokeStyle);
            return E_NOTIMPL;
        }

        STDMETHOD(CreatePathGeometry)(_Outptr_ ID2D1PathGeometry1 **pathGeometry)
        {
            UNREFERENCED_PARAMETER(pathGeometry);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateDrawingStateBlock)(_In_opt_ CONST D2D1_DRAWING_STATE_DESCRIPTION1 *drawingStateDescription, _In_opt_ IDWriteRenderingParams *textRenderingParams, _Outptr_ ID2D1DrawingStateBlock1 **drawingStateBlock)
        {
            UNREFERENCED_PARAMETER(drawingStateDescription);
            UNREFERENCED_PARAMETER(textRenderingParams);
            UNREFERENCED_PARAMETER(drawingStateBlock);
            return E_NOTIMPL;
        }

        STDMETHOD(CreateGdiMetafile)(_In_ IStream *metafileStream, _Outptr_ ID2D1GdiMetafile **metafile)
        {
            UNREFERENCED_PARAMETER(metafileStream);
            UNREFERENCED_PARAMETER(metafile);
            return E_NOTIMPL;
        }

        STDMETHOD(RegisterEffectFromStream)(_In_ REFCLSID classId, _In_ IStream *propertyXml, _In_reads_opt_(bindingsCount) CONST D2D1_PROPERTY_BINDING *bindings, UINT32 bindingsCount, _In_ CONST PD2D1_EFFECT_FACTORY effectFactory)
        {
            UNREFERENCED_PARAMETER(bindings);
            UNREFERENCED_PARAMETER(bindingsCount);
            UNREFERENCED_PARAMETER(effectFactory);
            return E_NOTIMPL;
        }

        STDMETHOD(RegisterEffectFromString)(_In_ REFCLSID classId, _In_ PCWSTR propertyXml, _In_reads_opt_(bindingsCount) CONST D2D1_PROPERTY_BINDING *bindings, UINT32 bindingsCount, _In_ CONST PD2D1_EFFECT_FACTORY effectFactory)
        {
            UNREFERENCED_PARAMETER(bindings);
            UNREFERENCED_PARAMETER(bindingsCount);
            UNREFERENCED_PARAMETER(effectFactory);
            return E_NOTIMPL;
        }

        STDMETHOD(UnregisterEffect)(_In_ REFCLSID classId)
        {
            UNREFERENCED_PARAMETER(classId);
            return E_NOTIMPL;
        }

        STDMETHOD(GetRegisteredEffects)(_Out_writes_to_opt_(effectsCount, *effectsReturned) CLSID *effects, UINT32 effectsCount, _Out_opt_ UINT32 *effectsReturned, _Out_opt_ UINT32 *effectsRegistered) CONST
        {
            return E_NOTIMPL;
        }

        STDMETHOD(GetEffectProperties)(_In_ REFCLSID effectId, _Outptr_ ID2D1Properties **properties) CONST
        {
            return E_NOTIMPL;
        }

        //  ID2D1Multithread
        STDMETHOD_(BOOL, GetMultithreadProtected)() CONST
        {
            VERIFY_FAIL();
            return FALSE;
        }

        STDMETHOD_(void, Enter)()
        {
            VERIFY_FAIL();
        }

        STDMETHOD_(void, Leave)()
        {
            VERIFY_FAIL();
        }
    };

    HRESULT CreateMockD2D1Factory(_Out_ IMockD2D1Factory ** factory)
    {
        *factory = new MockD2D1Factory();
        return S_OK;
    }

} } } } }