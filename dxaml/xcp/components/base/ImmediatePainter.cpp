// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <wrl/client.h>

namespace RenderingBackend
{
    HRESULT CreateImmediatePainter(Mode mode, const PainterOptions& options, std::unique_ptr<IImmediatePainter>& painter) noexcept
    {
        painter.reset();
        if (options.width == 0 || options.height == 0 || !(options.dpiScale > 0))
        {
            return E_INVALIDARG;
        }

        switch (mode)
        {
            case Mode::ImmediateGdiPlus:
                return CreateGdiPlusPainter(options, painter);

            case Mode::ImmediateD2DHardware:
            case Mode::ImmediateD2DSoftware:
            {
                Microsoft::WRL::ComPtr<ID3D11Device> device;
                DeviceIdentity identity{};
                const HRESULT hr = CreateD3DDeviceForMode(mode, &device, &identity);
                if (FAILED(hr))
                {
                    return hr;
                }
                return CreateD2DPainter(mode, device.Get(), options, painter);
            }

            default:
                return E_INVALIDARG;
        }
    }

    FallbackReason ProbeImmediateMode(Mode mode) noexcept
    {
        if (mode == Mode::Composition)
        {
            return FallbackReason::None;
        }
        PainterOptions options;
        options.width = 16;
        options.height = 16;
        std::unique_ptr<IImmediatePainter> painter;
        const HRESULT hr = CreateImmediatePainter(mode, options, painter);
        if (SUCCEEDED(hr) && painter)
        {
            return FallbackReason::None;
        }
        if (hr == E_NOTIMPL || hr == E_INVALIDARG)
        {
            return FallbackReason::NotImplemented;
        }
        return FallbackReason::DeviceUnavailable;
    }
}
