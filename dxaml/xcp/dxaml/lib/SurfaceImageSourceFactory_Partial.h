// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    class SurfaceImageSourceFactory :
        public SurfaceImageSourceFactoryGenerated,
        public ISurfaceImageSourceManagerNative
    {
    public:
        SurfaceImageSourceFactory();
        ~SurfaceImageSourceFactory() override;

        BEGIN_INTERFACE_MAP(SurfaceImageSourceFactory, SurfaceImageSourceFactoryGenerated)
            INTERFACE_ENTRY(SurfaceImageSourceFactory, ISurfaceImageSourceManagerNative)
        END_INTERFACE_MAP(SurfaceImageSourceFactory, SurfaceImageSourceFactoryGenerated)

        IFACEMETHOD(FlushAllSurfacesWithDevice)(_In_ IUnknown *pDevice);

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
    };
}

