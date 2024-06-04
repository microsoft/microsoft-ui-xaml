// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ImageProviderInterfaces.h"

class OfferableSoftwareBitmap;

// TODO: Modernize this class similar to ImageCopyParams
class AsyncCopyToSurfaceTask final : public CXcpObjectBase< IImageTask >
{
public:
    static _Check_return_ HRESULT Create(
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
        _In_ IImageDecodeCallback* pCallback,
        _Outptr_ AsyncCopyToSurfaceTask** ppTask
        );

    // IImageTask
    uint64_t GetRequestId() const override { return 0; }
    _Check_return_ HRESULT Execute() override;

    // Test hook - simulates a device lost error when we're doing an off-thread hardware surface upload.
    // Access to this is not synchronized. The test is expected to prime this flag before doing any off-thread decoding/uploading.
    static bool s_testHook_ForceDeviceLostOnNextUpload;

protected:
    AsyncCopyToSurfaceTask(
        );

    ~AsyncCopyToSurfaceTask(
        ) override;

    _Check_return_ HRESULT Initialize(
        _In_ ImageTaskDispatcher* pDispatcher,
        _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
        _In_ IImageDecodeCallback* pCallback
        );

    _Check_return_ HRESULT CopyOperation(
        unsigned int width,
        unsigned int height,
        _Out_ xref_ptr<OfferableSoftwareBitmap>& spOutputSurface
        );

    void CopyToSoftwareSurface(
        _In_reads_(capacity) uint8_t* pInputBuffer,
        wgri::BitmapPlaneDescription bitmapPlaneDescription,
        unsigned long capacity,
        _In_ const xref_ptr<OfferableSoftwareBitmap>& spOutputSurface
        );

    _Check_return_ HRESULT CopyToHardwareSurfaces(
        _In_reads_(capacity) uint8_t* pInputBuffer,
        wgri::BitmapPlaneDescription bitmapPlaneDescription,
        unsigned long capacity
        );

    xref_ptr<ImageCopyParams> m_spCopyParams;

    // TODO: Change this to a smart pointer
    IImageDecodeCallback* m_pCallback;
    ImageTaskDispatcher* m_pDispatcherNoRef;
};
