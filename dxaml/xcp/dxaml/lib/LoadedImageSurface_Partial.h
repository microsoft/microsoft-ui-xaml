// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "LoadedImageSurface.g.h"
#include <fwd/windows.storage.h>
#include <microsoft.ui.composition.private.h>

namespace DirectUI
{
    PARTIAL_CLASS(LoadedImageSurface)
        , public WUComp::ICompositionSurfaceFacade
    {
    public:
        LoadedImageSurface() = default;
        ~LoadedImageSurface() override = default;

        _Check_return_ HRESULT InitFromUri(_In_ wf::IUriRuntimeClass* uri, _In_ wf::Size size);
        _Check_return_ HRESULT InitFromStream(_In_ wsts::IRandomAccessStream* stream, _In_ wf::Size size);

        // IClosable
        IFACEMETHOD(Close()) override;

        // ICompositionSurfaceFacade
        IFACEMETHOD(GetRealSurface(_Outptr_ WUComp::ICompositionSurface** value)) override;

    protected:
        BEGIN_INTERFACE_MAP(LoadedImageSurface, LoadedImageSurfaceGenerated)
            INTERFACE_ENTRY(LoadedImageSurface, WUComp::ICompositionSurfaceFacade)
        END_INTERFACE_MAP(LoadedImageSurface, LoadedImageSurfaceGenerated)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        Microsoft::WRL::ComPtr<wsts::IStreamReadOperation> m_streamReadOperation;

        _Check_return_ HRESULT OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *sender, _In_ wf::AsyncStatus status);
    };
}
