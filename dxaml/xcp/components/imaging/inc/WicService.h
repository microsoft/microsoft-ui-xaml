// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <d2d1_1.h>

struct IWICImagingFactory;
struct IWICBitmapDecoder;
struct ImageMetadata;
class IRawData;
struct IImageDecoder;

class WicService final
{
public:
    static WicService& GetInstance()
    {
        // Safe because static initialization
        static WicService *instance = nullptr;
        static INIT_ONCE s_InitOnce = INIT_ONCE_STATIC_INIT;

        InitOnceExecuteOnce(&s_InitOnce, InitOnceCallback, &instance, nullptr);
        return *instance;
    }    

    // Called from DLL_PROCESS_DETACH, like ThreadPoolService::DetachFactories, so the final release of the D2D factory
    // (and the WARP device SVG decodes created on it) doesn't run from this static's destructor during process exit.
    static void DetachD2DFactory();

    // Since this is a singleton, caller can use the IWICImagingFactory without incurring
    // the cost of ref counting.  However, caller should never release the object without
    // also doing an AddRef.
    const wrl::ComPtr<IWICImagingFactory>& GetFactory();

    // For rasterizing SVGs off the UI thread. Not the render factory: its lock is also taken around XAML's D3D device
    // context calls, and a software EndDraw holds the factory lock for the whole WARP flush.
    const wrl::ComPtr<ID2D1Factory1>& GetD2DFactory();

     _Check_return_ HRESULT GetBitmapDecoder(
        _In_ IRawData& rawData,
        _Out_ wrl::ComPtr<IWICBitmapDecoder>& spBitmapDecoder
        );

    _Check_return_ HRESULT GetMetadata(
        _In_ const wrl::ComPtr<IWICBitmapDecoder>& spWICBitmapDecoder,
        _Out_ ImageMetadata& imageMetadata
        );

    std::unique_ptr<IImageDecoder> CreateDefaultDecoder(
        _In_ const ImageMetadata& imageMetadata
        );

private:
    static BOOL CALLBACK InitOnceCallback(
        _Inout_      PINIT_ONCE pInitOnce,
        _Inout_opt_  PVOID param,
        _Out_opt_    PVOID* pContext)
    {
        static WicService instance;
        *static_cast<WicService**>(param) = &instance;
        return TRUE;
    }
    
    _Check_return_ HRESULT WICGetTransformOptionFromMetadata(
        _In_ IWICMetadataQueryReader *pQueryReader,
        _Out_ WICBitmapTransformOptions *pOptions
        );
        
    WicService();
    ~WicService() = default;
    WicService(const WicService&) = delete;
    WicService& operator=(const WicService&) = delete;

    static bool s_hasInstance;
    wrl::ComPtr<IWICImagingFactory> m_spIWICFactory;
    wrl::ComPtr<ID2D1Factory1> m_d2dFactory;
};
