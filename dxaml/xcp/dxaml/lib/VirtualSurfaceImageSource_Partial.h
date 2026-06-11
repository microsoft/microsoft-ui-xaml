// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualSurfaceImageSource.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(VirtualSurfaceImageSource),
        public IVirtualSurfaceImageSourceNative,
        public IVirtualSurfaceImageSourceCallbacks
    {
        friend class VirtualSurfaceImageSourceFactory;
        BEGIN_INTERFACE_MAP(VirtualSurfaceImageSource, VirtualSurfaceImageSourceGenerated)
            INTERFACE_ENTRY(VirtualSurfaceImageSource, IVirtualSurfaceImageSourceNative)
        END_INTERFACE_MAP(VirtualSurfaceImageSource, VirtualSurfaceImageSourceGenerated)        

    public:
        VirtualSurfaceImageSource();
        ~VirtualSurfaceImageSource() override;

        // ISurfaceImageSourceNative overrides
        IFACEMETHOD(SetDevice)(_In_ IDXGIDevice*  pDevice) override;
        IFACEMETHOD(BeginDraw)(_In_ RECT updateRect, _Outptr_ IDXGISurface** ppSurface, _Out_ POINT* pOffset) override;
        IFACEMETHOD(EndDraw)() override;

        // IVirtualSurfaceImageSourceNative overrides
        IFACEMETHOD(Invalidate)(_In_ RECT updateRect) override;
        IFACEMETHOD(GetUpdateRectCount)(_Out_ DWORD *pCount) override;
        IFACEMETHOD(GetUpdateRects)(_Out_writes_(count) RECT* pRects,_In_ DWORD count) override;
        IFACEMETHOD(GetVisibleBounds)(_Out_ RECT *pBounds) override; 
        IFACEMETHOD(RegisterForUpdatesNeeded)(_In_opt_ IVirtualSurfaceUpdatesCallbackNative *pCallback) override;
        IFACEMETHOD(Resize)(_In_ INT newWidth, _In_ INT newHeight) override;


        // IVirtualSurfaceImageSourceCallbacks implementation
        _Check_return_ HRESULT UpdatesNeeded() override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        IVirtualSurfaceUpdatesCallbackNative *m_pUpdatesCallback;
    };
}
