// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Microsoft.UI.Xaml.CompositionTarget-private.h"
#include "RenderedEventArgs.g.h"

namespace DirectUI
{
    class CompositionTarget:
        public xaml_media::ICompositionTarget,
        public xaml_media::ICompositionTargetStatics,
        public ICompositionTargetPrivate,
        public ctl::AbstractActivationFactory
    {
        public:
            IFACEMETHOD(add_Rendering)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
            IFACEMETHOD(remove_Rendering)(_In_ EventRegistrationToken tToken) override;
            IFACEMETHOD(add_Rendered)(_In_ wf::IEventHandler<xaml_media::RenderedEventArgs*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
            IFACEMETHOD(remove_Rendered)(_In_ EventRegistrationToken tToken) override;
            IFACEMETHOD(add_SurfaceContentsLost)(_In_ wf::IEventHandler<IInspectable*>* pValue, _Out_ EventRegistrationToken* ptToken) override;
            IFACEMETHOD(remove_SurfaceContentsLost)(_In_ EventRegistrationToken tToken) override;
            IFACEMETHOD(SuspendRendering)(_In_ BOOLEAN isSuspended) override;
            IFACEMETHOD(GetCompositorForCurrentThread)(_Outptr_ WUComp::ICompositor** compositor) override;

            HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;
            CompositionTarget();

            
    };
}

