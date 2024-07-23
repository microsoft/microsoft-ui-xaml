// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <windows.graphics.imaging.h>
#include <memorybuffer.h>
#include <Windows.Graphics.Imaging.Interop.h>
#include "SoftwareBitmapUtility.h"

_Check_return_ HRESULT SoftwareBitmapUtility::CreateSoftwareBitmap(
    uint32_t width,
    uint32_t height,
    _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
    )
{
    return SoftwareBitmapUtility::CreateSoftwareBitmap(
        width,
        height,
        wgri::BitmapPixelFormat_Bgra8,
        wgri::BitmapAlphaMode_Ignore,
        spSoftwareBitmap);
}

_Check_return_ HRESULT SoftwareBitmapUtility::CreateSoftwareBitmap(
    uint32_t width,
    uint32_t height,
    wgri::BitmapPixelFormat pixelFormat,
    wgri::BitmapAlphaMode alphaMode,
    _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
    )
{
    wrl::ComPtr<wgri::ISoftwareBitmapFactory> spSoftwareBitmapFactory;
    IFCFAILFAST(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap).Get(),
        &spSoftwareBitmapFactory));

    IFC_RETURN(spSoftwareBitmapFactory->CreateWithAlpha(
        pixelFormat,
        width,
        height,
        alphaMode,
        &spSoftwareBitmap));

    return S_OK;
}

_Check_return_ HRESULT SoftwareBitmapUtility::CreateSoftwareBitmap(
    _In_ const wrl::ComPtr<IWICBitmap>& spWicBitmap,
    bool readOnly,
    _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
    )
{
    wrl::ComPtr<ISoftwareBitmapNativeFactory> spSoftwareBitmapNativeFactory;
    IFC_RETURN(CoCreateInstance(
        CLSID_SoftwareBitmapNativeFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&spSoftwareBitmapNativeFactory)));

    IFC_RETURN(spSoftwareBitmapNativeFactory->CreateFromWICBitmap(
        spWicBitmap.Get(),
        readOnly ? TRUE : FALSE,
        IID_PPV_ARGS(&spSoftwareBitmap)));

    return S_OK;
}

// TODO: Make sure to replace the validity check in SoftwareBitmapSource::SetBitmapAsyncImpl
//                 with a call to this new method.
_Check_return_ HRESULT SoftwareBitmapUtility::ValidateSoftwareBitmap(
    _In_ const wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
    )
{
    auto width = 0;
    auto height = 0;
    wgri::BitmapPixelFormat pixelFormat = wgri::BitmapPixelFormat_Unknown;
    wgri::BitmapAlphaMode alphaMode = wgri::BitmapAlphaMode_Ignore;

    // Get SoftwareBitmap info
    IFC_RETURN(spSoftwareBitmap->get_PixelWidth(&width));
    IFC_RETURN(spSoftwareBitmap->get_PixelHeight(&height));
    IFC_RETURN(spSoftwareBitmap->get_BitmapPixelFormat(&pixelFormat));
    IFC_RETURN(spSoftwareBitmap->get_BitmapAlphaMode(&alphaMode));
    IFCCHECK_RETURN(
        (width > 0) &&
        (height > 0) &&
        (pixelFormat == wgri::BitmapPixelFormat_Bgra8) &&
        (alphaMode != wgri::BitmapAlphaMode_Straight));

    return S_OK;
}

SoftwareBitmapLock::SoftwareBitmapLock(
    _In_ const wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap,
    wgri::BitmapBufferAccessMode accessMode
    )
{
    wrl::ComPtr<wgri::IBitmapBuffer> spBitmapBuffer;

    IFCFAILFAST(spSoftwareBitmap->LockBuffer(
        accessMode,
        &spBitmapBuffer));

    int32_t planeCount = 0;
    IFCFAILFAST(spBitmapBuffer->GetPlaneCount(&planeCount));
    FAIL_FAST_ASSERT(planeCount == 1);

    IFCFAILFAST(spBitmapBuffer->GetPlaneDescription(0, &m_bitmapPlaneDescription));

    wrl::ComPtr<wf::IMemoryBuffer> spMemoryBuffer;
    IFCFAILFAST(spBitmapBuffer.As(&spMemoryBuffer));

    // This holds onto the buffer reference so its lifetime survives this object.
    IFCFAILFAST(spMemoryBuffer->CreateReference(&m_spMemoryBufferReference));

    wrl::ComPtr<wf_::IMemoryBufferByteAccess> spMemoryBufferByteAccess;
    IFCFAILFAST(m_spMemoryBufferReference.As(&spMemoryBufferByteAccess));

    IFCFAILFAST(spMemoryBufferByteAccess->GetBuffer(&m_pBuffer, &m_bufferSize));
}