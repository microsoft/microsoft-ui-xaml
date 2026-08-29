// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SurfaceImageSource.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(SurfaceImageSource),
        public ISurfaceImageSourceNative
    {
        friend class SurfaceImageSourceFactory;
        BEGIN_INTERFACE_MAP(SurfaceImageSource, SurfaceImageSourceGenerated)
            INTERFACE_ENTRY(SurfaceImageSource, ISurfaceImageSourceNative)
            INTERFACE_ENTRY(SurfaceImageSource, ISurfaceImageSourceNativeWithD2D)
        END_INTERFACE_MAP(SurfaceImageSource, SurfaceImageSourceGenerated)

    private:
        // Since ISurfaceImageSourceNative and ISurfaceImageSourceNativeWithD2D have some
        // methods with exactly the same name and exactly the same parameters, but
        // a different implementation, we need separate entry points.
        // SurfaceImageSourceNativeWithD2D is a little aggregation object that provide
        // the entry points for ISurfaceImageSourceNativeWithD2D.
        class SurfaceImageSourceWithD2D : public ISurfaceImageSourceNativeWithD2D
        {
        public:
            SurfaceImageSourceWithD2D(SurfaceImageSource* pOwner)
            : m_pOwnerNoRef(pOwner)
            {
            }
            ~SurfaceImageSourceWithD2D()
            {
            }

        public:
            // IUnknown implementation
            IFACEMETHOD(QueryInterface)(_In_ REFIID riid, _Outptr_ void **ppvObject) override
            {
                RRETURN(static_cast<ISurfaceImageSourceNative*>(m_pOwnerNoRef)->QueryInterface(riid, ppvObject));
            }

            // AddRef/Release just defer actual ref-counting to the owner, this is safe since this object's lifetime is the same as the owner.
            IFACEMETHOD_(ULONG, AddRef)(void) override
            {
                RRETURN(static_cast<ISurfaceImageSourceNative*>(m_pOwnerNoRef)->AddRef());
            }
            IFACEMETHOD_(ULONG, Release)(void) override
            {
                RRETURN(static_cast<ISurfaceImageSourceNative*>(m_pOwnerNoRef)->Release());
            }

            // ISurfaceImageSourceNativeWithD2D overrides
            IFACEMETHOD(SetDevice)(_In_ IUnknown*  pDevice) override
            {
                RRETURN(m_pOwnerNoRef->SetDeviceWithD2D(pDevice));
            }
            IFACEMETHOD(BeginDraw)(_In_ REFRECT updateRect, _In_ REFIID iid, _COM_Outptr_ void** ppSurface, _Out_ POINT* pOffset) override
            {
                RRETURN(m_pOwnerNoRef->BeginDrawWithD2D(updateRect, iid, reinterpret_cast<IUnknown**>(ppSurface), pOffset));
            }
            IFACEMETHOD(EndDraw)() override
            {
                RRETURN(m_pOwnerNoRef->EndDrawWithD2D());
            }
            IFACEMETHOD(SuspendDraw)() override
            {
                RRETURN(m_pOwnerNoRef->SuspendDraw());
            }
            IFACEMETHOD(ResumeDraw)() override
            {
                RRETURN(m_pOwnerNoRef->ResumeDraw());
            }

        private:
            SurfaceImageSourceWithD2D();

        private:
            SurfaceImageSource* m_pOwnerNoRef;
        };

    public:
        SurfaceImageSource();
        ~SurfaceImageSource() override;

        // ISurfaceImageSourceNative overrides
        IFACEMETHOD(SetDevice)(_In_ IDXGIDevice*  pDevice) override;
        IFACEMETHOD(BeginDraw)(_In_ RECT updateRect, _Outptr_ IDXGISurface** ppSurface, _Out_ POINT* pOffset) override;
        IFACEMETHOD(EndDraw)() override;

        // ISurfaceImageSourceNativeWithD2D actual implementation, aggregated by SurfaceImagesourceWithD2D
        IFACEMETHOD(SetDeviceWithD2D)(_In_ IUnknown*  pDevice);
        IFACEMETHOD(BeginDrawWithD2D)(_In_ RECT updateRect, _In_ REFIID iid, _Outptr_ IUnknown** ppSurface, _Out_ POINT* pOffset);
        IFACEMETHOD(EndDrawWithD2D)();
        IFACEMETHOD(SuspendDraw)();
        IFACEMETHOD(ResumeDraw)();

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        SurfaceImageSourceWithD2D m_SISWithD2DImpl;
    };
}
