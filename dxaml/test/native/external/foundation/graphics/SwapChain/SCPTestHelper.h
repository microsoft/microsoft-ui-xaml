// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Microsoft.UI.Xaml.media.dxinterop.h"
#include <DXGI1_2.h>
#include <Dxgi1_3.h>
#include <D3D11.h>
#include <D2d1_1.h>

using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Animation;
using namespace Microsoft::WRL;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Controls::Primitives;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Shapes;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

// Helper class for warpping the SwapChainPanel testing: handles creation/interaction with D3DDevice, SwapChain Handle and SwapChain, and other test methods
class SwapChainPanelTestWrapper
{

public:

    SwapChainPanelTestWrapper::SwapChainPanelTestWrapper();
    void CreateSwapChain(SwapChainPanel^ scp,unsigned int width,unsigned int height, DXGI_ALPHA_MODE alphaMode);
    void CreateSwapChainHandle(SwapChainPanel^ scp,unsigned int width,unsigned int height, DXGI_ALPHA_MODE alphaMode);
    void Update(D2D1::ColorF color);
    void SetSwapChain();
    void SetSwapChainNull();
    void SetSwapChainHandle();
    void SetSwapChainBadHandle();
    void SetSwapChainHandleNull();
    ComPtr<ID3D11Device> GetD3DDevice();
    void SetD3DDevice(ComPtr<ID3D11Device> newDevice);
    void ReleaseSwapChainPanel();

private:
    void CreateSwapChainInternal(SwapChainPanel^ scp,unsigned int width,unsigned int height, DXGI_ALPHA_MODE alphaMode, bool bHandle); // bHandle: true create SwapChain Handle for testing, otherwise regular SwapChain
    SwapChainPanel^ m_scp;
    ComPtr<IDXGISwapChain1> m_swapChain;
    HANDLE m_swapChainHandle;
    ComPtr<ID3D11Device> m_d3dDevice;
    static unsigned int s_swapChainSequence;
    static unsigned int s_swapChainHandleSequence;
};

} } } } } }


