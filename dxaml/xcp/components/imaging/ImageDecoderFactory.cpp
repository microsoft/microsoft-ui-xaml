// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <WindowRenderTarget.h>
#include <WindowsGraphicsDeviceManager.h>
#include <d3d11device.h>
#include <D2DAccelerated.h>
#include <EncodedImageData.h>
#include <ImageMetadata.h>
#include <SvgImageDecoder.h>
#include <WicService.h>
#include <corep.h>
#include "GraphicsUtility.h"

namespace ImageDecoderFactory
{

_Check_return_ HRESULT CreateDecoder(
    _In_ CCoreServices* core,
    _In_ const EncodedImageData& encodedImageData,
    _Out_ std::unique_ptr<IImageDecoder>& decoder)
{
    if (encodedImageData.IsSvg())
    {
        CD3D11Device* deviceNoRef = core->NWGetWindowRenderTarget()->GetGraphicsDeviceManager()->GetGraphicsDevice();

        if (SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder)
        {
            deviceNoRef->TestHook_LoseDevice();
            SvgImageDecoder::s_testHook_ForceDeviceLostOnCreatingSvgDecoder = false;
        }

        HRESULT hr = deviceNoRef->EnsureD2DResources();
        if (GraphicsUtility::IsDeviceLostError(hr))
        {
            //
            // Explicitly call into CCoreServices to mark device lost. This is usually done when a failure HR propagates
            // up the stack to CWindowRenderTarget::Draw, but in this case Draw isn't always on the stack because we set
            // up decoders with the ImageTaskDispatcher as a UI thread callback work item.
            //
            // HandleDeviceLost also resets the hr to S_OK so make a copy of it. We do want to bubble the failure HR up
            // the stack to cancel the decode. This bubbles up to CXcpDispatcher::OnWindowMessage where the error is
            // swallowed by IGNOREHR, but we're ok with that because we already manually marked the device lost and
            // kicked off the recovery process.
            //
            HRESULT hrCopy = hr;
            core->HandleDeviceLost(&hrCopy);
            return hr;
        }
        else
        {
            IFCFAILFAST(hr);

            // The decoder should take a reference on the factory.
            ctl::ComPtr<ID2D1Factory1> d2dFactory(deviceNoRef->GetD2DFactory()->GetFactory());

            decoder = std::make_unique<SvgImageDecoder>(std::move(d2dFactory));
        }
    }
    else
    {
        decoder = WicService::GetInstance().CreateDefaultDecoder(encodedImageData.GetMetadata());
    }

    return S_OK;
}

}