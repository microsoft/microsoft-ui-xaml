// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <WexTestClass.h>
#include "d3d11_2.h"
#include "MockComObjectBase.h"
#include "MockDxgi.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {

    // D3D11DeviceContext Mock
    struct __declspec(uuid("401D86E9-6DAE-400E-B508-BBDF7F3EB545")) IMockD3D11DeviceContext : public ID3D11DeviceContext
    {
        // Put Mock specific functions here
    };

    HRESULT CreateMockD3D11DeviceContext(_In_ ID3D11Device * device, _Out_ IMockD3D11DeviceContext ** context);

    //  CreateD3D11DeviceDetour - Class used to mock the D3D Device and other objects.  It will use Detours to
    //                            detour the CreateDevice call to return a mock D3D device that can be manipulated
    //                            by the test.

    typedef HRESULT(WINAPI D3D11CREATEDEVICE)(
        _In_ IDXGIAdapter *pAdapter,
        _In_ D3D_DRIVER_TYPE DriverType,
        _In_ HMODULE Software,
        _In_ UINT Flags,
        _In_ const D3D_FEATURE_LEVEL *pFeatureLevels,
        _In_ UINT FeatureLevels,
        _In_ UINT SDKVersion,
        _Out_ ID3D11Device **ppDevice,
        _Out_ D3D_FEATURE_LEVEL *pFeatureLevel,
        _Out_ ID3D11DeviceContext **ppImmediateContext
        );

    class CreateD3D11DeviceDetour : public CreateMockDetourBase<D3D11CREATEDEVICE>
    {
    public:
        CreateD3D11DeviceDetour() : CreateMockDetourBase<D3D11CREATEDEVICE>(L"d3d11.dll", "D3D11CreateDevice")
        {
            SetDetour([](IDXGIAdapter *pAdapter, D3D_DRIVER_TYPE driverType, HMODULE software, UINT flags, const D3D_FEATURE_LEVEL *pFeatureLevels, UINT featureLevels, UINT SDKVersion, ID3D11Device **ppDevice, D3D_FEATURE_LEVEL *pFeatureLevel, ID3D11DeviceContext **ppImmediateContext) -> HRESULT
            {
                Microsoft::WRL::ComPtr<IMockDXGIDevice> dxgiDevice;
                IFC_RETURN(CreateMockDXGIDevice(pAdapter, driverType, flags, &dxgiDevice));
                
                Microsoft::WRL::ComPtr<ID3D11Device> d3d11Device;
                IFC_RETURN(dxgiDevice.As(&d3d11Device));

                if (ppImmediateContext)
                {
                    Microsoft::WRL::ComPtr<IMockD3D11DeviceContext> context;
                    IFC_RETURN(CreateMockD3D11DeviceContext(d3d11Device.Get(), &context));
                    IFC_RETURN(context.CopyTo(ppImmediateContext));
                }

                if (pFeatureLevel)
                {
                    if (pAdapter)
                    {
                        Microsoft::WRL::ComPtr<IMockDXGIAdapter> mockAdapter;
                        IFC_RETURN(pAdapter->QueryInterface(__uuidof(IMockDXGIAdapter), &mockAdapter));
                        *pFeatureLevel = mockAdapter->GetFeatureLevel();
                    }
                    else
                    {
                        *pFeatureLevel = D3D_FEATURE_LEVEL_11_0;
                    }
                }

                if (ppDevice)
                {
                    *ppDevice = d3d11Device.Detach();
                }

                return S_OK;
            });
        }
    };

} } } } }
