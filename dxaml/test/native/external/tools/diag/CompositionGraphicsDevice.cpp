// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <memorybuffer.h>
#include <XamlLogging.h>

// This is necessary in order to access the ABI namespace.
#undef BUILD_WINDOWS
#include <microsoft.ui.composition.interop.h>
#include "CompositionGraphicsDevice.h"

WICTranslate wicTranslateTable[7] =
{
    { GUID_WICPixelFormat64bppPRGBAHalf,{ DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_PREMULTIPLIED }, wgri::BitmapPixelFormat::Rgba16, wgri::BitmapAlphaMode::Premultiplied, L"Rgba16Premultiplied" },
    { GUID_WICPixelFormat32bppPBGRA,{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, wgri::BitmapPixelFormat::Bgra8, wgri::BitmapAlphaMode::Premultiplied, L"BGRA8Premultiplied" },
    { GUID_WICPixelFormat32bppPRGBA,{ DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, wgri::BitmapPixelFormat::Rgba8, wgri::BitmapAlphaMode::Premultiplied, L"RGBA8Premultiplied" },
    { GUID_WICPixelFormat8bppAlpha,{ DXGI_FORMAT_A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED }, wgri::BitmapPixelFormat::Gray8, wgri::BitmapAlphaMode::Premultiplied, L"A8Premultiplied" },
    { GUID_WICPixelFormat64bppRGBHalf,{ DXGI_FORMAT_R16G16B16A16_FLOAT, D2D1_ALPHA_MODE_IGNORE }, wgri::BitmapPixelFormat::Rgba16, wgri::BitmapAlphaMode::Ignore, L"RGBA16Ignore" },
    { GUID_WICPixelFormat32bppBGR,{ DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE }, wgri::BitmapPixelFormat::Bgra8, wgri::BitmapAlphaMode::Ignore, L"BGRA8Ignore" },
    { GUID_WICPixelFormat32bppRGB,{ DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_IGNORE }, wgri::BitmapPixelFormat::Rgba8, wgri::BitmapAlphaMode::Ignore, L"RGBA8Ignore" },
};

GUID WicPixelFormatFromD2D1PixelFormat(
    _In_ D2D1_PIXEL_FORMAT d2d1PixelFormat)
{
    for (int i = 0; (i < ARRAYSIZE(wicTranslateTable)); i++)
    {
        if (wicTranslateTable[i].d2d1PixelFormat.format == d2d1PixelFormat.format &&
            wicTranslateTable[i].d2d1PixelFormat.alphaMode == d2d1PixelFormat.alphaMode)
        {
            return wicTranslateTable[i].wicPixelFormat;
        }
    }

    return GUID_NULL;
}

wgri::BitmapPixelFormat GetWGIPixelFormatFromWicPixelFormat(
    _In_ const GUID& wicPixelFormat)
{
    for (int i = 0; (i < ARRAYSIZE(wicTranslateTable)); i++)
    {
        if (wicTranslateTable[i].wicPixelFormat == wicPixelFormat)
        {
            return wicTranslateTable[i].bitmapPixelFormat;
        }
    }

    return wgri::BitmapPixelFormat::Unknown;
}

wgri::BitmapAlphaMode GetWGIAlphaModeFromWicPixelFormat(
    _In_ const GUID& wicPixelFormat)
{
    for (int i = 0; (i < ARRAYSIZE(wicTranslateTable)); i++)
    {
        if (wicTranslateTable[i].wicPixelFormat == wicPixelFormat)
        {
            return wicTranslateTable[i].bitmapAlphaMode;
        }
    }
    return wgri::BitmapAlphaMode::Ignore;
}

D2D1_PIXEL_FORMAT D2D1PixelFormatFromWGIPixelFormat(
    _In_ wgri::BitmapPixelFormat bitmapPixelFormat,
    _In_ wgri::BitmapAlphaMode bitmapAlphaMode)
{
    for (int i = 0; (i < ARRAYSIZE(wicTranslateTable)); i++)
    {
        if (wicTranslateTable[i].bitmapPixelFormat == bitmapPixelFormat && wicTranslateTable[i].bitmapAlphaMode == bitmapAlphaMode)
        {
            return wicTranslateTable[i].d2d1PixelFormat;
        }
    }

    return{ DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_UNKNOWN };
}

Platform::String^ PixelFormatNameFromD2D1PixelFormat(
    _In_ D2D1_PIXEL_FORMAT d2d1PixelFormat)
{
    for (int i = 0; (i < ARRAYSIZE(wicTranslateTable)); i++)
    {
        if (wicTranslateTable[i].d2d1PixelFormat.format == d2d1PixelFormat.format &&
            wicTranslateTable[i].d2d1PixelFormat.alphaMode == d2d1PixelFormat.alphaMode)
        {
            return wicTranslateTable[i].pixelFormatName;
        }
    }

    return ref new Platform::String();
}

namespace Tests { namespace Tools { namespace XamlDiagnostics {

    CompositionGraphicsDevice^ CompositionGraphicsDevice::CreateCompositionGraphicsDevice(
        Microsoft::UI::Composition::Compositor^ compositor)
    {
        CompositionGraphicsDevice^ device = ref new CompositionGraphicsDevice(compositor);

        LogThrow_IfFailed(device->InitializeGraphicsDevice());

        return device;
    }

    Microsoft::UI::Composition::ICompositionSurface^ CompositionGraphicsDevice::CreateDrawingSurfaceFromFile(
        Platform::String^ filename,
        Microsoft::Graphics::DirectX::DirectXPixelFormat pixelFormat,
        Microsoft::Graphics::DirectX::DirectXAlphaMode alphaMode)
    {
        wrl::ComPtr<IWICBitmapDecoder> bitmapDecoder;
        LogThrow_IfFailed(m_wicImagingFactory->CreateDecoderFromFilename(filename->Data(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &bitmapDecoder));

        wrl::ComPtr<IWICBitmapFrameDecode> bitmapFrameDecode;
        LogThrow_IfFailed(bitmapDecoder->GetFrame(0, &bitmapFrameDecode));
        wrl::ComPtr<IWICFormatConverter> formatConverter;
        LogThrow_IfFailed(m_wicImagingFactory->CreateFormatConverter(&formatConverter));
        LogThrow_IfFailed(formatConverter->Initialize(
            bitmapFrameDecode.Get(),
            WicPixelFormatFromD2D1PixelFormat({ (DXGI_FORMAT)pixelFormat, (D2D1_ALPHA_MODE)alphaMode }),
            WICBitmapDitherTypeNone,
            nullptr,
            0.0f,
            WICBitmapPaletteTypeMedianCut));

        unsigned int imageWidth;
        unsigned int imageHeight;
        LogThrow_IfFailed(formatConverter->GetSize(&imageWidth, &imageHeight));
        ABI::Windows::Foundation::Size size{ static_cast<float>(imageWidth), static_cast<float>(imageHeight) };

        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionGraphicsDevice> igraphicsDevice;
        LogThrow_IfFailed(m_igraphicsDevice.As(&igraphicsDevice));

        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionDrawingSurface> drawingSurface;
        LogThrow_IfFailed(igraphicsDevice->CreateDrawingSurface(
            size,
            (ABI::Microsoft::Graphics::DirectX::DirectXPixelFormat)pixelFormat,
            (ABI::Microsoft::Graphics::DirectX::DirectXAlphaMode)alphaMode,
            &drawingSurface));

        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionDrawingSurfaceInterop> drawingSurfaceInterop;
        LogThrow_IfFailed(drawingSurface.As(&drawingSurfaceInterop));

        wrl::ComPtr<ID3D11Texture2D> d3d11Texture2d;
        POINT surfaceUpdateOffset;
        LogThrow_IfFailed(drawingSurfaceInterop->BeginDraw(
            nullptr,
            IID_PPV_ARGS(&d3d11Texture2d),
            &surfaceUpdateOffset));

        // It is necessary to always call EndDraw in order to allow the rest of the tests
        // to run in case one of them fails.
        auto drawingSurfaceEndDraw = wil::scope_exit([&drawingSurfaceInterop]()
        {
            drawingSurfaceInterop->EndDraw();
        });

        // Get a copy of the description of the Texture that is used as updateObject.
        D3D11_TEXTURE2D_DESC d3d11Texture2dDesc{};
        d3d11Texture2d->GetDesc(&d3d11Texture2dDesc);

        // Modify the description to a description that allows CPU access.
        d3d11Texture2dDesc.Usage = D3D11_USAGE_STAGING;
        d3d11Texture2dDesc.BindFlags = 0;
        d3d11Texture2dDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        d3d11Texture2dDesc.MiscFlags = 0;

        // Create a new staging Texture with the modified description.
        wrl::ComPtr<ID3D11Texture2D> stagingd3d11Texture2d;
        LogThrow_IfFailed(m_d3d11Device->CreateTexture2D(
            &d3d11Texture2dDesc,
            nullptr,
            &stagingd3d11Texture2d));

        unsigned int bytesPerPixel = GetBytesPerPixel(WicPixelFormatFromD2D1PixelFormat({
            (DXGI_FORMAT)pixelFormat, (D2D1_ALPHA_MODE)alphaMode }));

        // Get the bitmap data from the WICBitmapSource.
        std::vector<BYTE> fcPixels(imageWidth * imageHeight * bytesPerPixel);
        unsigned int bytesPerPixelUInt = static_cast<unsigned int>(bytesPerPixel);
        LogThrow_IfFailed(formatConverter->CopyPixels(
            nullptr,
            imageWidth * bytesPerPixelUInt,
            imageWidth * imageHeight * bytesPerPixelUInt,
            fcPixels.data()));

        // Get CPU Access to the staging texture.
        D3D11_MAPPED_SUBRESOURCE d3d11MappedSubresource{};
        LogThrow_IfFailed(m_d3d11DeviceContext->Map(
            stagingd3d11Texture2d.Get(),
            0,
            D3D11_MAP_WRITE,
            0,
            &d3d11MappedSubresource));

        // Adding the proper Update Offset for writing to the Texture.
        // This is particularly important when the D3DSubresource is shared by various surfaces.
        char* pData = static_cast<char*>(d3d11MappedSubresource.pData);
        pData += (d3d11MappedSubresource.RowPitch * surfaceUpdateOffset.y);
        pData += (surfaceUpdateOffset.x * bytesPerPixel);

        // Write to the texture one row at a time
        for (unsigned int currentLine = 0; currentLine < imageHeight; currentLine++)
        {
            LogThrow_IfFailed(HRESULT_FROM_WIN32(memcpy_s(
                pData,
                imageWidth * bytesPerPixel,
                fcPixels.data() + (imageWidth * currentLine * (bytesPerPixel)),
                imageWidth * bytesPerPixel)));

            // Point to the next row of the Subresource data.
            pData += d3d11MappedSubresource.RowPitch;
        }

        // Denying CPU access to the staging Texture in order to use it with the GPU.
        m_d3d11DeviceContext->Unmap(stagingd3d11Texture2d.Get(), 0);

        // Make a GPU copy from the staging texture to the update object.
        m_d3d11DeviceContext->CopyResource(
            d3d11Texture2d.Get(),
            stagingd3d11Texture2d.Get());

        return reinterpret_cast<Microsoft::UI::Composition::ICompositionSurface^>(drawingSurface.Get());
    }

    ::Windows::Graphics::Imaging::SoftwareBitmap^ CompositionGraphicsDevice::CreateSoftwareBitmapFromFile(Platform::String^ filename)
    {
        wrl::ComPtr<IWICBitmapDecoder> bitmapDecoder;
        LogThrow_IfFailed(m_wicImagingFactory->CreateDecoderFromFilename(filename->Data(),
            nullptr,
            GENERIC_READ,
            WICDecodeMetadataCacheOnLoad,
            &bitmapDecoder));

        wrl::ComPtr<IWICBitmapFrameDecode> bitmapFrameDecode;
        LogThrow_IfFailed(bitmapDecoder->GetFrame(0, &bitmapFrameDecode));

        unsigned int imageWidth;
        unsigned int imageHeight;
        LogThrow_IfFailed(bitmapFrameDecode->GetSize(&imageWidth, &imageHeight));
        GUID pixelFormat;
        LogThrow_IfFailed(bitmapFrameDecode->GetPixelFormat(&pixelFormat));

        wgri::BitmapPixelFormat bitmapPixelFormat = GetWGIPixelFormatFromWicPixelFormat(pixelFormat);
        VERIFY_ARE_NOT_EQUAL(wgri::BitmapPixelFormat::Unknown, bitmapPixelFormat);
        wgri::BitmapAlphaMode bitmapAlphaMode = GetWGIAlphaModeFromWicPixelFormat(pixelFormat);

        wgri::SoftwareBitmap^ softwareBitmap = ref new ::Windows::Graphics::Imaging::SoftwareBitmap(
            bitmapPixelFormat,
            static_cast<int>(imageWidth),
            static_cast<int>(imageHeight),
            bitmapAlphaMode);

        wgri::BitmapBuffer^ bitmapBuffer = softwareBitmap->LockBuffer(wgri::BitmapBufferAccessMode::Write);
        VERIFY_ARE_EQUAL(1, bitmapBuffer->GetPlaneCount());
        wgri::BitmapPlaneDescription bitmapPlaneDescription = bitmapBuffer->GetPlaneDescription(0);
        wf::IMemoryBufferReference^ memoryBufferReference = bitmapBuffer->CreateReference();
        wrl::ComPtr<IInspectable> iMemoryBufferReference = reinterpret_cast<IInspectable*>(memoryBufferReference);
        VERIFY_IS_NOT_NULL(iMemoryBufferReference);
        wrl::ComPtr<::Windows::Foundation::IMemoryBufferByteAccess> memoryBufferByteAccess;
        LogThrow_IfFailed(iMemoryBufferReference.As(&memoryBufferByteAccess));

        BYTE* bitmapBytes;
        unsigned int capacity;
        LogThrow_IfFailed(memoryBufferByteAccess->GetBuffer(&bitmapBytes, &capacity));
        LogThrow_IfFailed(bitmapFrameDecode->CopyPixels(nullptr, bitmapPlaneDescription.Stride, capacity, bitmapBytes));

        return softwareBitmap;
    }

    void CompositionGraphicsDevice::CreateImageFileFromMemory(
        unsigned int width,
        unsigned int height,
        unsigned int stride,
        BYTE* pixels,
        Platform::String^ filename)
    {
        wrl::ComPtr<IWICBitmapEncoder> encoder;
        VERIFY_SUCCEEDED(m_wicImagingFactory->CreateEncoder(GUID_ContainerFormatWmp, nullptr, &encoder));

        wrl::ComPtr<IWICStream> stream;
        VERIFY_SUCCEEDED(m_wicImagingFactory->CreateStream(&stream));

        VERIFY_SUCCEEDED(stream->InitializeFromFilename(filename->Data(), GENERIC_WRITE));
        VERIFY_SUCCEEDED(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
        wrl::ComPtr<IWICBitmapFrameEncode> frameEncode;
        wrl::ComPtr<IPropertyBag2> propertyBag = nullptr;

        VERIFY_SUCCEEDED(encoder->CreateNewFrame(&frameEncode, &propertyBag));

        // Set frame encode properties.
        PROPBAG2 options[6] = { 0 };
        VARIANT varValue[6];
        VariantInit(&varValue[0]);
        VariantInit(&varValue[1]);
        VariantInit(&varValue[2]);
        VariantInit(&varValue[3]);
        VariantInit(&varValue[4]);
        VariantInit(&varValue[5]);

        options[0].pstrName = const_cast<wchar_t*>(L"AlphaDataDiscard");
        varValue[0].vt = VT_UI1;
        varValue[0].bVal = 0;

        options[1].pstrName = const_cast<wchar_t*>(L"ImageQuality");
        varValue[1].vt = VT_R4;
        varValue[1].fltVal = 1.0;

        options[2].pstrName = const_cast<wchar_t*>(L"InterleavedAlpha");
        varValue[2].vt = VT_BOOL;
        varValue[2].boolVal = VARIANT_TRUE;

        options[3].pstrName = const_cast<wchar_t*>(L"Lossless");
        varValue[3].vt = VT_BOOL;
        varValue[3].boolVal = VARIANT_TRUE;

        options[4].pstrName = const_cast<wchar_t*>(L"Overlap");
        varValue[4].vt = VT_UI1;
        varValue[4].bVal = 0;

        options[5].pstrName = const_cast<wchar_t*>(L"Subsampling");
        varValue[5].vt = VT_UI1;
        varValue[5].bVal = 3;
        VERIFY_SUCCEEDED(propertyBag->Write(6u, options, varValue));

        VERIFY_SUCCEEDED(frameEncode->Initialize(propertyBag.Get()));
        VERIFY_SUCCEEDED(frameEncode->SetSize(width, height));
        GUID pixelFormat = GUID_WICPixelFormat32bppPBGRA;
        VERIFY_SUCCEEDED(frameEncode->SetPixelFormat(&pixelFormat));

        VERIFY_SUCCEEDED(frameEncode->WritePixels(height, stride, height * stride, pixels));
        VERIFY_SUCCEEDED(frameEncode->Commit());
        VERIFY_SUCCEEDED(encoder->Commit());
    }

    CompositionGraphicsDevice::CompositionGraphicsDevice(Microsoft::UI::Composition::Compositor^ compositor) :
        m_compositor(compositor)
    {
        // Create WICImagingFactory2
        VERIFY_SUCCEEDED(CoCreateInstance(
            CLSID_WICImagingFactory,
            NULL,
            CLSCTX_INPROC_SERVER,
            IID_IWICImagingFactory2,
            (LPVOID*)&m_wicImagingFactory));
    }

    CompositionGraphicsDevice::~CompositionGraphicsDevice()
    {
    }

    // Calls InitializeDX and then creates an ICompositionGraphicsDevice which is
    // used to create drawing surfaces.
    HRESULT CompositionGraphicsDevice::InitializeGraphicsDevice()
    {
        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositorInterop> compositorInterop;
        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositionGraphicsDevice> graphicsDevice;
        wrl::ComPtr<ABI::Microsoft::UI::Composition::ICompositor> iCompositor;

        VERIFY_SUCCEEDED(InitializeDX());

        iCompositor = reinterpret_cast<ABI::Microsoft::UI::Composition::ICompositor*>(m_compositor);
        VERIFY_SUCCEEDED(iCompositor.As(&compositorInterop));
        VERIFY_SUCCEEDED(compositorInterop->CreateGraphicsDevice(m_d3d11Device.Get(), &graphicsDevice));

        m_igraphicsDevice = graphicsDevice;

        return S_OK;
    }

    HRESULT CompositionGraphicsDevice::InitializeDX()
    {
        HRESULT hr = E_FAIL;

        //
        // Create D3D Device:
        // - We need to test different device types to find one that will work in the current
        //   environment.
        //

        D3D_DRIVER_TYPE driverTypes[] =
        {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
        };

        D3D_FEATURE_LEVEL featureLevelSupported;

        for (int i = 0; i < ARRAYSIZE(driverTypes); ++i)
        {
            wrl::ComPtr<ID3D11Device> d3d11DeviceTemp;
            wrl::ComPtr<ID3D11DeviceContext> d3d11DeviceContextTemp;

            hr = D3D11CreateDevice(
                nullptr,
                driverTypes[i],
                nullptr,
                D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                &d3d11DeviceTemp,
                &featureLevelSupported,
                &d3d11DeviceContextTemp);

            if (SUCCEEDED(hr))
            {
                m_d3d11Device = d3d11DeviceTemp;
                m_d3d11DeviceContext = d3d11DeviceContextTemp;
                break;
            }
        }

        return hr;
    }

    unsigned int CompositionGraphicsDevice::GetBytesPerPixel(
        Microsoft::Graphics::DirectX::DirectXPixelFormat pixelFormat,
        Microsoft::Graphics::DirectX::DirectXAlphaMode alphaMode)
    {
        return GetBytesPerPixel(WicPixelFormatFromD2D1PixelFormat({
            (DXGI_FORMAT)pixelFormat, (D2D1_ALPHA_MODE)alphaMode }));
    }

    unsigned int CompositionGraphicsDevice::GetBytesPerPixel(const GUID& wicPixelFormat)
    {
        wrl::ComPtr<IWICComponentInfo> componentInfo;
        VERIFY_SUCCEEDED(m_wicImagingFactory->CreateComponentInfo(wicPixelFormat, &componentInfo));
        wrl::ComPtr<IWICPixelFormatInfo> pixelFormatInfo;
        VERIFY_SUCCEEDED(componentInfo.As(&pixelFormatInfo));
        unsigned int bpp;
        VERIFY_SUCCEEDED(pixelFormatInfo->GetBitsPerPixel(&bpp));

        // The tests only support Pixels Formats with a bpp that fits exactly in n bytes,
        // If the bpp does not fits exactly the tests must fail.
        VERIFY_ARE_EQUAL(0u, bpp % 8u);
        return bpp / 8;
    }

}}}
