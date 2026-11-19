// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <iterator>

// D3D device creation for the D2D modes. Hardware adapter selection is owned by the immediate-rendering-d2d-hw branch.
namespace RenderingBackend
{
    namespace
    {
        constexpr D3D_FEATURE_LEVEL c_featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0,
        };

        HRESULT FillIdentity(Mode mode, ID3D11Device* device, bool isWarp, DeviceIdentity* identity) noexcept
        {
            Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
            Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
            DXGI_ADAPTER_DESC desc{};
            HRESULT hr = device->QueryInterface(IID_PPV_ARGS(&dxgiDevice));
            if (SUCCEEDED(hr)) hr = dxgiDevice->GetAdapter(&adapter);
            if (SUCCEEDED(hr)) hr = adapter->GetDesc(&desc);
            if (FAILED(hr))
            {
                return hr;
            }
            identity->mode = mode;
            identity->adapterLuid = (static_cast<uint64_t>(static_cast<uint32_t>(desc.AdapterLuid.HighPart)) << 32) | desc.AdapterLuid.LowPart;
            identity->isWarp = isWarp;
            identity->featureLevel = static_cast<uint32_t>(device->GetFeatureLevel());
            return S_OK;
        }
    }

    HRESULT CreateD3DDeviceForMode(Mode mode, ID3D11Device** device, DeviceIdentity* identity) noexcept
    {
        *device = nullptr;
        *identity = {};

        if (mode == Mode::ImmediateD2DSoftware)
        {
            Microsoft::WRL::ComPtr<ID3D11Device> warp;
            HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                c_featureLevels, static_cast<UINT>(std::size(c_featureLevels)), D3D11_SDK_VERSION, &warp, nullptr, nullptr);
            if (SUCCEEDED(hr)) hr = FillIdentity(mode, warp.Get(), true, identity);
            if (SUCCEEDED(hr))
            {
                TraceDeviceInfo(mode, identity->adapterLuid, identity->isWarp, identity->featureLevel);
                *device = warp.Detach();
            }
            return hr;
        }

        if (mode == Mode::ImmediateD2DHardware)
        {
            return E_NOTIMPL;
        }

        return E_INVALIDARG;
    }
}
