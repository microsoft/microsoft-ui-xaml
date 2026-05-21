// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <d3d11.h>
#include <wincodec.h>
#include <Microsoft.Graphics.DirectX.h>

//-------------------------------------------------------------------------------------
// WIC Pixel Format Translation Data
//-------------------------------------------------------------------------------------
struct WICTranslate
{
    GUID wicPixelFormat;
    D2D1_PIXEL_FORMAT d2d1PixelFormat;
    ::Windows::Graphics::Imaging::BitmapPixelFormat bitmapPixelFormat;
    ::Windows::Graphics::Imaging::BitmapAlphaMode bitmapAlphaMode;
    Platform::String^ pixelFormatName;
};

D2D1_PIXEL_FORMAT D2D1PixelFormatFromWGIPixelFormat(
    _In_ ::Windows::Graphics::Imaging::BitmapPixelFormat bitmapPixelFormat,
    _In_ ::Windows::Graphics::Imaging::BitmapAlphaMode bitmapAlphaMode);

Platform::String^ PixelFormatNameFromD2D1PixelFormat(
    _In_ D2D1_PIXEL_FORMAT d2d1PixelFormat);

namespace Tests { namespace Tools { namespace XamlDiagnostics {
    
    [::Windows::Foundation::Metadata::Internal]
    [::Windows::Foundation::Metadata::WebHostHidden]
    public ref class CompositionGraphicsDevice sealed
    {
    public:
        static CompositionGraphicsDevice^ CreateCompositionGraphicsDevice(
            Microsoft::UI::Composition::Compositor^ compositor);

        Microsoft::UI::Composition::ICompositionSurface^ CreateDrawingSurfaceFromFile(
            Platform::String^ filename,
            Microsoft::Graphics::DirectX::DirectXPixelFormat pixelFormat,
            Microsoft::Graphics::DirectX::DirectXAlphaMode alphaMode);

        ::Windows::Graphics::Imaging::SoftwareBitmap^ CreateSoftwareBitmapFromFile(
            Platform::String^ filename);

        void CreateImageFileFromMemory(
            unsigned int width,
            unsigned int height,
            unsigned int stride,
            BYTE* pixels,
            Platform::String^ filename);

        virtual ~CompositionGraphicsDevice();

        unsigned int GetBytesPerPixel(
            Microsoft::Graphics::DirectX::DirectXPixelFormat pixelFormat,
            Microsoft::Graphics::DirectX::DirectXAlphaMode alphaMode);

    private:
        CompositionGraphicsDevice(Microsoft::UI::Composition::Compositor^ compositor);

        unsigned int GetBytesPerPixel(const GUID& wicPixelFormat);

        HRESULT InitializeGraphicsDevice();
        
        HRESULT InitializeDX();

    private:
        Microsoft::UI::Composition::Compositor^ m_compositor;
        
        wrl::ComPtr<IInspectable> m_igraphicsDevice;
        wrl::ComPtr<ID3D11Device> m_d3d11Device;
        wrl::ComPtr<ID3D11DeviceContext> m_d3d11DeviceContext;
        wrl::ComPtr<IWICImagingFactory2> m_wicImagingFactory;
    };

}}}
