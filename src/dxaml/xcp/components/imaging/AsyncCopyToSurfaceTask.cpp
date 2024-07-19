// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include <windows.graphics.imaging.h>
#include "ImageProviderInterfaces.h"
#include "palgfx.h"
#include "corep.h"
#include "depends.h"
#include "framework.h"
#include "refcounting.h"
#include "SurfaceDecodeParams.h"
#include "ImageDecodeParams.h"
#include "ImageCopyParams.h"
#include "OfferableSoftwareBitmap.h"
#include "ImageProviderInterfaces.h"
#include "GraphicsUtility.h"
#include "AsyncDecodeResponse.h"
#include "AsyncCopyToSurfaceTask.h"
#include "AsyncReleaseTask.h"
#include "ImageTaskDispatcher.h"
#include <MemoryBuffer.h>

// TODO: Throughout this entire file, modernize it.  (refer to other files in imaging)
// TODO: Should change this to pass back an AsyncCopyResponse instead of AsyncDecodeResponse
//                 because we need to include wincodec.h and ImageDecodeParams.h because of it.

bool AsyncCopyToSurfaceTask::s_testHook_ForceDeviceLostOnNextUpload = false;

_Check_return_ HRESULT
AsyncCopyToSurfaceTask::Create(
    _In_ ImageTaskDispatcher* pDispatcher,
    _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
    _In_ IImageDecodeCallback* pCallback,
    _Outptr_ AsyncCopyToSurfaceTask** ppTask
)
{
    HRESULT hr = S_OK;
    AsyncCopyToSurfaceTask* pNewTask = nullptr;

    pNewTask = new AsyncCopyToSurfaceTask();

    IFC(pNewTask->Initialize(pDispatcher, spCopyParams, pCallback));

    SetInterface(*ppTask, pNewTask);

Cleanup:
    ReleaseInterface(pNewTask);

    return hr;
}

AsyncCopyToSurfaceTask::AsyncCopyToSurfaceTask()
    : m_pCallback(nullptr)
    , m_pDispatcherNoRef(nullptr)
{
    XCP_WEAK(&m_pDispatcherNoRef);
}

AsyncCopyToSurfaceTask::~AsyncCopyToSurfaceTask(
    )
{
    ReleaseInterface(m_pCallback);
}

_Check_return_ HRESULT
AsyncCopyToSurfaceTask::Initialize(
    _In_ ImageTaskDispatcher* pDispatcher,
    _In_ const xref_ptr<ImageCopyParams>& spCopyParams,
    _In_ IImageDecodeCallback* pCallback
)
{
    HRESULT hr = S_OK;

    m_pDispatcherNoRef = pDispatcher;
    m_spCopyParams = spCopyParams;

    SetInterface(m_pCallback, pCallback);

    return hr;
}

_Check_return_ HRESULT
AsyncCopyToSurfaceTask::Execute(
    )
{
    INT32 width = 0;
    INT32 height = 0;
    IFC_RETURN(m_spCopyParams->GetBitmap()->get_PixelWidth(&width));
    IFC_RETURN(m_spCopyParams->GetBitmap()->get_PixelHeight(&height));

    xref_ptr<OfferableSoftwareBitmap> spSurface;
    auto decodeResult = CopyOperation(
        static_cast<unsigned int>(width),
        static_cast<unsigned int>(height),
        spSurface);

    // The async copy operation piggy backs on the current decode response except
    // it doesn't use the decode params because it isn't cached and it doesn't use
    // the encoded image.
    //
    // This could be rolled into a better hierarchy of abstraction but it would
    // require a fair bit of refactoring to make it extensible for these
    // and future scenarios.  This refactoring is a bit overkill but might be
    // valuable if this needs to be extended for async image operations that don't
    // have a subset of fields.  We get away with it for now because copy uses
    // a subset of fields that is used for decode.
    auto response = make_xref<AsyncDecodeResponse>(decodeResult, std::move(spSurface));

    IFC_RETURN(m_pCallback->OnDecode(std::move(response), -1 /*requestId*/));

    return S_OK;
}

_Check_return_ HRESULT
AsyncCopyToSurfaceTask::CopyOperation(
    unsigned int width,
    unsigned int height,
    _Outref_ xref_ptr<OfferableSoftwareBitmap>& spOutputSurface
    )
{
    wgri::BitmapAlphaMode bitmapAlphaMode;
    ctl::ComPtr<wgri::IBitmapBuffer> spBitmapBuffer;
    ctl::ComPtr<wf::IMemoryBuffer> spMemoryBuffer;
    ctl::ComPtr<wf::IMemoryBufferReference> spMemoryBufferReference;
    ctl::ComPtr<wf_::IMemoryBufferByteAccess> spMemoryBufferByteAccess;
    wgri::BitmapPlaneDescription bitmapPlaneDescription;
    INT32 planeCount = 0;
    BYTE* pInputBuffer = nullptr;
    UINT32 capacity = 0;

    // Setup source information
    IFCFAILFAST(m_spCopyParams->GetBitmap()->LockBuffer(
        wgri::BitmapBufferAccessMode_Read,
        spBitmapBuffer.GetAddressOf()));

    // Verify there is only one plane and get the plane output description
    IFCFAILFAST(spBitmapBuffer->GetPlaneCount(&planeCount));
    FAIL_FAST_ASSERT(planeCount == 1);

    IFCFAILFAST(spBitmapBuffer->GetPlaneDescription(0, &bitmapPlaneDescription));

    IFCFAILFAST(spBitmapBuffer.As(&spMemoryBuffer));
    IFCFAILFAST(spMemoryBuffer->CreateReference(&spMemoryBufferReference));
    IFCFAILFAST(spMemoryBufferReference.As(&spMemoryBufferByteAccess));

    IFCFAILFAST(spMemoryBufferByteAccess->GetBuffer(&pInputBuffer, &capacity));

    // Prepare for the copy
    const SurfaceUpdateList& surfaceUpdateList = m_spCopyParams->GetSurfaceUpdateList();
    if (surfaceUpdateList.empty())
    {
        IFCFAILFAST(m_spCopyParams->GetBitmap()->get_BitmapAlphaMode(&bitmapAlphaMode));

        bool opaque = (bitmapAlphaMode == wgri::BitmapAlphaMode_Ignore);

        // No hardware surfaces to write to, so allocate a software surface and write to that.

        spOutputSurface = make_xref<OfferableSoftwareBitmap>(
            pixelColor32bpp_A8R8G8B8,
            width,
            height,
            opaque);

        CopyToSoftwareSurface(pInputBuffer, bitmapPlaneDescription, capacity, spOutputSurface);
    }
    else
    {
        // Hardware surfaces are available, write to that and don't return a software surface.
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(CopyToHardwareSurfaces(pInputBuffer, bitmapPlaneDescription, capacity));
    }

    return S_OK;
}

void
AsyncCopyToSurfaceTask::CopyToSoftwareSurface(
    _In_reads_(capacity) uint8_t* pInputBuffer,
    wgri::BitmapPlaneDescription bitmapPlaneDescription,
    unsigned long capacity,
    _In_ const xref_ptr<OfferableSoftwareBitmap>& spOutputSurface
    )
{
    uint8_t* pDestinationBuffer = nullptr;
    int32_t destinationStride = 0;
    uint32_t destinationWidth = 0;
    uint32_t destinationHeight = 0;

    // TODO: This doesn't have to be locked anymore with OfferableSoftwareBitmap
    IFCFAILFAST(spOutputSurface->Lock(
        reinterpret_cast< void** >(&pDestinationBuffer),
        &destinationStride,
        &destinationWidth,
        &destinationHeight));

    FAIL_FAST_ASSERT(
        (destinationWidth == bitmapPlaneDescription.Width) &&
        (destinationHeight == bitmapPlaneDescription.Height));

    FAIL_FAST_ASSERT(destinationStride > 0);

    uint32_t destinationBufferSize = destinationStride * destinationHeight;

    if (destinationBufferSize < capacity)
    {
        XAML_FAIL_FAST();
    }

    size_t lineLength = bitmapPlaneDescription.Width * 4; // Multiplied by 4 for Bytes per pixel
    uint8_t* pSrcLine = pInputBuffer + bitmapPlaneDescription.StartIndex;
    uint8_t* pDestLine = reinterpret_cast<uint8_t*>(pDestinationBuffer);
    for (long line = 0;
        line < bitmapPlaneDescription.Height;
        line++, pSrcLine += bitmapPlaneDescription.Stride, pDestLine += destinationStride)
    {
        memcpy(pDestLine, pSrcLine, lineLength);
    }

    IGNOREHR(spOutputSurface->Unlock());
}

_Check_return_ HRESULT
AsyncCopyToSurfaceTask::CopyToHardwareSurfaces(
    _In_reads_(capacity) uint8_t* pInputBuffer,
    wgri::BitmapPlaneDescription bitmapPlaneDescription,
    unsigned long capacity
    )
{
    // NOTE: This method has a similar implementation to WICImageDecoder::DecodeToMultipleSurfaces
    //       but that function is build to work off of WIC CopyPixels function and a temp buffer both
    //       of which aren't required here.
    //       Any bug fixes here should be considered there.

    // Copy size is a control variable that can be tweaked for Memory/Performance efficiency
    // It is used to determine the approximate size that is copied in each iteration.
    static const int CopySize = 1 * 1024 * 1024;

    // Go through all the tiles and hold the flush so that they are flushed at the end.
    // If the flush happens in the middle, it is possible there could be a crash if the staging texture is
    // released after Unlock and before QueueUpdate since it needs to use manual update because all lines of a
    // texture need to be copied prior to queuing an update for a texture.
    for (auto& tile : m_spCopyParams->GetSurfaceUpdateList())
    {
        tile->GetSurface()->SetHoldFlush(true);
    }

    auto allowFlushOnExit = wil::scope_exit([&]
    {
        for (auto& tile : m_spCopyParams->GetSurfaceUpdateList())
        {
            tile->GetSurface()->SetHoldFlush(false);
        }
    });

    // Go through all the tiles and copy the content from the input surface to the various output tiles.
    for (auto& spTile : m_spCopyParams->GetSurfaceUpdateList())
    {
        // Claim temporary ownership of the surface for this algorithm... If the lock fails, it means
        // that the surface has been deleted in another thread and no decoding needs to be done.
        auto spOutputSurface = spTile->GetSurface();
        int rectX = spTile->GetRect().X;
        int rectY = spTile->GetRect().Y;
        int rectWidth = spTile->GetRect().Width;
        int rectHeight = spTile->GetRect().Height;

        size_t rectXInBytes = GetPlaneStride(spOutputSurface->GetPixelFormat(), 0, rectX);
        size_t rectWidthInBytes = GetPlaneStride(spOutputSurface->GetPixelFormat(), 0, rectWidth);
        int maxLineCopyCount = std::max<int>(static_cast<int>(CopySize / rectWidthInBytes), 1);
        int lineCopyCount = std::min(maxLineCopyCount, rectHeight);

        ASSERT(spOutputSurface->GetPixelFormat() == pixelColor32bpp_A8R8G8B8);
        ASSERT(spOutputSurface->IsVirtual() ?
               (rectX + rectWidth) <= static_cast<int>(spOutputSurface->GetWidth()) :
               rectWidth <= static_cast<int>(spOutputSurface->GetWidth()));
        ASSERT(spOutputSurface->IsVirtual() ?
               (rectY + rectHeight) <= static_cast<int>(spOutputSurface->GetHeight()) :
               rectHeight <= static_cast<int>(spOutputSurface->GetHeight()));

        // Make sure no tile decodes from outside the bounds of the image
        // Note that this accounts for a scaler applied in the WIC pipeline
        ASSERT((rectX + rectWidth) <= static_cast<int>(bitmapPlaneDescription.Width));
        ASSERT((rectY + rectHeight) <= static_cast<int>(bitmapPlaneDescription.Height));

        // Copy a small segment of the image so it does not monopolize the D3DDevice lock for a long period of time.
        unsigned int copyCount = lineCopyCount;
        for (int currentLine = 0; currentLine < rectHeight; currentLine += lineCopyCount)
        {
            // Early abort mechanic, do a quick AddRef/Release to get the reference count on the surface
            // If it equal to 1, then this decode is holding onto the last reference which means
            // the decode no longer needs to occur and can be released early.  This is also important
            // for device lost scenario in which case the hardware surface gets released, so early exit
            // is important so the hardware surface allocation isn't holding the device from being released.
            // Note that all tiles are assumed allocated and released together via CTiledSurface.  So if one
            // tile hits a reference count of 1, then it is assumed the entire copy operation can be aborted.
            // TODO: This doesn't work, need a new abort mechanism
            spOutputSurface->AddRef();
            if (spOutputSurface->Release() == 1)
            {
                return S_OK;
            }

            // Check to see if this is the last segment and then adjust the lines for the last segment
            if ((currentLine + lineCopyCount) >= rectHeight)
            {
                copyCount = rectHeight - currentLine;
            }

            unsigned char* pDestinationBuffer = nullptr;
            signed int destinationStride = 0;
            unsigned int destinationWidth = 0;
            unsigned int destinationHeight = 0;

            // If the test is simulating a device lost error while doing an off-thread upload to a hardware surface,
            // return it here. The LockRect calls below are the places that can return device lost errors when
            // we're running for real.
            if (s_testHook_ForceDeviceLostOnNextUpload)
            {
                s_testHook_ForceDeviceLostOnNextUpload = false;
                IFC_RETURN(DXGI_ERROR_DEVICE_REMOVED);
            }

            if (spOutputSurface->IsVirtual())
            {
                // Virtual will use the rect with offset to lock a rect inside a virtual surface.
                IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(spOutputSurface->LockRect(
                    spTile->GetRect(),
                    reinterpret_cast<void**>(&pDestinationBuffer),
                    &destinationStride,
                    &destinationWidth,
                    &destinationHeight));
            }
            else
            {
                // Non-virtual needs to adjust the rect to use the same size but with X and Y as 0 since the surface
                // positioning is handled externally.
                // TODO_WinRTSprites: This else block can probably be deleted along with the rest of tiling code
                //                              when SpriteVisualsEnabled is universally enabled.
                XRECT lockRect = { 0, 0, rectWidth, rectHeight };

                IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(spOutputSurface->LockRect(
                    lockRect,
                    reinterpret_cast<void**>(&pDestinationBuffer),
                    &destinationStride,
                    &destinationWidth,
                    &destinationHeight));
            }

            FAIL_FAST_ASSERT(destinationStride > 0);

            // Copy the decoded lines from the temp buffer to the destination surface
            uint8_t* pDestinationLine =
                pDestinationBuffer +
                (destinationStride * currentLine);
            uint8_t* pSourceBufferLine =
                pInputBuffer +
                bitmapPlaneDescription.StartIndex +
                ((rectY + currentLine) * bitmapPlaneDescription.Stride) +
                rectXInBytes;

            for (unsigned int copyLine = 0; copyLine < copyCount; copyLine++)
            {
                memcpy_s(pDestinationLine, destinationStride, pSourceBufferLine, rectWidthInBytes);

                pDestinationLine += destinationStride;
                pSourceBufferLine += bitmapPlaneDescription.Stride;
            }

            // Unlock but don't queue the update until the copy is completed.
            IFCFAILFAST(spOutputSurface->Unlock(false));
        }

        IFCFAILFAST(spOutputSurface->QueueUpdate());
    }

    return S_OK;
}
