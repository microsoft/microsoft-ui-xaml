// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SCPTestHelper.h"
#include <XamlTailored.h>
#include <Dcomp.h>


using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// use static sequence number so they can be stay unique when multiple SCP wrappers are active
unsigned int SwapChainPanelTestWrapper::s_swapChainSequence = 0;
unsigned int SwapChainPanelTestWrapper::s_swapChainHandleSequence = 0;

SwapChainPanelTestWrapper::SwapChainPanelTestWrapper()
: m_swapChainHandle(nullptr)
, m_swapChain(nullptr)

{
}

void SwapChainPanelTestWrapper::ReleaseSwapChainPanel()
{
    // reset sequence number
    SwapChainPanelTestWrapper::s_swapChainSequence = 0;
    SwapChainPanelTestWrapper::s_swapChainHandleSequence = 0;
    m_scp = nullptr;
    if (m_swapChainHandle)
    {
        CloseHandle(m_swapChainHandle);
        m_swapChainHandle = nullptr;
    }
}

ComPtr<ID3D11Device> SwapChainPanelTestWrapper::GetD3DDevice()
{
    return m_d3dDevice;
}

void SwapChainPanelTestWrapper::SetD3DDevice(ComPtr<ID3D11Device> newDevice)
{
    m_d3dDevice = newDevice;
}

void SwapChainPanelTestWrapper::CreateSwapChain(SwapChainPanel^ scp,unsigned int width,unsigned int height, DXGI_ALPHA_MODE alphaMode)
{
    CreateSwapChainInternal(scp, width, height, alphaMode, false);
}

void SwapChainPanelTestWrapper::CreateSwapChainHandle(SwapChainPanel^ scp,unsigned int width,unsigned int height, DXGI_ALPHA_MODE alphaMode)
{
    CreateSwapChainInternal(scp, width, height, alphaMode, true);
}

// Helper function to create a swap chain or swap chain handle 
void SwapChainPanelTestWrapper::CreateSwapChainInternal
(
    SwapChainPanel^ scp,
    unsigned int width,unsigned int height,
    DXGI_ALPHA_MODE alphaMode,
    bool bCreateHandle // Create SwapChain Handle if true
)
{
    ComPtr<IDXGIAdapter> dxgiAdapter;
    ComPtr<IDXGIFactory2> dxgiFactory;
    ComPtr<IDXGIFactoryMedia> dxgiFactoryMedia;
    ComPtr<IDXGIDevice> dxgiDevice;
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};

    m_scp = scp;
    if (m_d3dDevice == nullptr)
    {
        const D3D_FEATURE_LEVEL featureLevels[] =
        {
            D3D_FEATURE_LEVEL_11_0,
            D3D_FEATURE_LEVEL_10_1,
            D3D_FEATURE_LEVEL_10_0,
            D3D_FEATURE_LEVEL_9_3,
            D3D_FEATURE_LEVEL_9_2,
            D3D_FEATURE_LEVEL_9_1,
        }; 

        unsigned int flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;   

        LogThrow_IfFailed(D3D11CreateDevice(
            nullptr,
            D3D_DRIVER_TYPE_HARDWARE,
            nullptr,
            flags,
            featureLevels,
            ARRAYSIZE(featureLevels),
            D3D11_SDK_VERSION,
            &m_d3dDevice,
            nullptr,
            nullptr
            ));
    }

    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;           
    swapChainDesc.Stereo = false; 
    swapChainDesc.SampleDesc.Count = 1;                         
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = 2;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; 
    swapChainDesc.AlphaMode = alphaMode;
    swapChainDesc.Flags = 0;

    LogThrow_IfFailed(m_d3dDevice.As(&dxgiDevice));
    LogThrow_IfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));
    LogThrow_IfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory2), &dxgiFactory));
    if (!bCreateHandle)
    {
        LogThrow_IfFailed(dxgiFactory->CreateSwapChainForComposition(m_d3dDevice.Get(), &swapChainDesc, nullptr, &m_swapChain));
    }
    else
    {
        // Create a DComp surface handle
        LogThrow_IfFailed(::DCompositionCreateSurfaceHandle(GENERIC_ALL, nullptr, &m_swapChainHandle));
        LogThrow_IfFailed(dxgiFactory.As(&dxgiFactoryMedia));
        LogThrow_IfFailed(dxgiFactoryMedia->CreateSwapChainForCompositionSurfaceHandle(
                m_d3dDevice.Get(),
                m_swapChainHandle,
                &swapChainDesc,
                nullptr,
                &m_swapChain
            ));
    }
}

void SwapChainPanelTestWrapper::SetSwapChain()
{
    ComPtr<ISwapChainPanelNative> panelNative;

    LogThrow_IfFailed(reinterpret_cast<IUnknown*>(m_scp)->QueryInterface(IID_PPV_ARGS(&panelNative)));
    TestServices::Utilities->SetMockDCompSwapChainIdentifier(reinterpret_cast<unsigned __int64>(m_swapChain.Get()), ++s_swapChainSequence);
    LogThrow_IfFailed(panelNative->SetSwapChain(m_swapChain.Get()));
}

// test function to be able to set null swap chain
void SwapChainPanelTestWrapper::SetSwapChainNull()
{
    ComPtr<ISwapChainPanelNative> panelNative;

    LogThrow_IfFailed(reinterpret_cast<IUnknown*>(m_scp)->QueryInterface(IID_PPV_ARGS(&panelNative)));
    LogThrow_IfFailed(panelNative->SetSwapChain(nullptr));
}

void SwapChainPanelTestWrapper::SetSwapChainHandle()
{
    ComPtr<ISwapChainPanelNative2> panelNative2;

    LogThrow_IfFailed(reinterpret_cast<IUnknown*>(m_scp)->QueryInterface(IID_PPV_ARGS(&panelNative2)));
    TestServices::Utilities->SetMockDCompSwapChainIdentifier(reinterpret_cast<unsigned __int64>(m_swapChainHandle), ++s_swapChainHandleSequence);
    LogThrow_IfFailed(panelNative2->SetSwapChainHandle(m_swapChainHandle));
}

void SwapChainPanelTestWrapper::SetSwapChainBadHandle()
{
    ComPtr<ISwapChainPanelNative2> panelNative2;

    LogThrow_IfFailed(reinterpret_cast<IUnknown*>(m_scp)->QueryInterface(IID_PPV_ARGS(&panelNative2)));
    // By using the current thread handle, we know it isn't a swap chain handle. Since we don't know what error DComp
    // will return when using this handle, we just check to make sure we don't succeed.
    WEX::Common::Throw::If(SUCCEEDED(panelNative2->SetSwapChainHandle(::GetCurrentThread())), E_UNEXPECTED);
}

void SwapChainPanelTestWrapper::SetSwapChainHandleNull()
{
    ComPtr<ISwapChainPanelNative2> panelNative2;

    LogThrow_IfFailed(reinterpret_cast<IUnknown*>(m_scp)->QueryInterface(IID_PPV_ARGS(&panelNative2)));
    LogThrow_IfFailed(panelNative2->SetSwapChainHandle(nullptr));
}

// Update swap chain with color specified
void SwapChainPanelTestWrapper::Update(D2D1::ColorF color)
{
    ComPtr<ID2D1Bitmap1> spD2DBitmap;
    ComPtr<IDXGISurface> spSurface;
    ComPtr<IDXGIDevice> dxgiDevice;
    ComPtr<ID2D1DeviceContext> spD2DDeviceContext;
    ComPtr<ID2D1Device> spD2DDevice;
    DXGI_PRESENT_PARAMETERS presentParam = { 0, nullptr, nullptr, nullptr };

    LogThrow_IfFailed(m_d3dDevice.As(&dxgiDevice));
    LogThrow_IfFailed(D2D1CreateDevice(dxgiDevice.Get(), nullptr, &spD2DDevice));
    LogThrow_IfFailed(spD2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &spD2DDeviceContext));
    LogThrow_IfFailed(m_swapChain->GetBuffer(0, __uuidof(IDXGISurface), &spSurface));
    LogThrow_IfFailed(spD2DDeviceContext->CreateBitmapFromDxgiSurface(spSurface.Get(), nullptr, &spD2DBitmap));
    spD2DDeviceContext->BeginDraw();
    spD2DDeviceContext->SetTarget(spD2DBitmap.Get());
    spD2DDeviceContext->Clear(color);
    spD2DDeviceContext->SetTarget(nullptr);
    LogThrow_IfFailed(spD2DDeviceContext->EndDraw());
    LogThrow_IfFailed(m_swapChain->Present1(1, 0, &presentParam));
}

} } } }}}
