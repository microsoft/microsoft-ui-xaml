// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "HardwareTileCopyTests.h"
#include <CoInitHelper.h>
#include <ImageDecodeParams.h>
#include <ImageMetadata.h>
#include <ImagingUtility.h>
#include <OfferableSoftwareBitmap.h>
#include <WicService.h>

// The isolated imaging target does not link the DComp-backed HWTexture implementation.
// These test-only lifetime stubs let the in-memory subclass exercise the real copy routine.
HWTexture::HWTexture(DCompSurface* surface, HWTextureManager* manager)
    : m_pTextureMgrNoRef(manager), m_pDeviceSurface(surface)
{
}

HWTexture::~HWTexture()
{
    FireOnDelete();
}

namespace
{
    constexpr uint32_t PixelBytes = 4;
    constexpr uint8_t PaddingByte = 0xa5;

    struct TileState
    {
        TileState(XRECT bounds, uint32_t imageWidth, uint32_t imageHeight, bool isVirtual)
            : rect(bounds), virtualSurface(isVirtual),
              width(isVirtual ? imageWidth : bounds.Width),
              height(isVirtual ? imageHeight : bounds.Height),
              stride(width * PixelBytes + 16), pixels(stride * height, PaddingByte)
        {
        }

        XRECT rect;
        bool virtualSurface;
        uint32_t width;
        uint32_t height;
        uint32_t stride;
        std::vector<uint8_t> pixels;
        bool holdFlush = false;
        uint32_t holdCount = 0;
        uint32_t releaseCount = 0;
        uint32_t lockCount = 0;
        std::vector<bool> updates;
        HRESULT lockResult = S_OK;
    };

    class MemoryTexture final : public HWTexture
    {
    public:
        explicit MemoryTexture(TileState& state) : HWTexture(nullptr, nullptr), m_state(state) {}

        uint32_t GetWidth() const override { return m_state.width; }
        uint32_t GetHeight() const override { return m_state.height; }
        bool IsVirtual() override { return m_state.virtualSurface; }
        PixelFormat GetPixelFormat() override { return pixelColor32bpp_A8R8G8B8; }
        bool IsOpaque() override { return false; }
        HRESULT QueueUpdate() override { return E_NOTIMPL; }
        HRESULT Lock(void**, int32_t*, uint32_t*, uint32_t*) override { return E_NOTIMPL; }
        HRESULT Unlock() override { return E_NOTIMPL; }

        HRESULT LockRect(const XRECT& rect, void** address, int32_t* stride, uint32_t* width, uint32_t* height) override
        {
            VERIFY_IS_TRUE(m_state.holdFlush);
            VERIFY_ARE_EQUAL(m_state.virtualSurface ? m_state.rect.X : 0, rect.X);
            VERIFY_ARE_EQUAL(m_state.virtualSurface ? m_state.rect.Y : 0, rect.Y);
            VERIFY_ARE_EQUAL(m_state.rect.Width, rect.Width);
            VERIFY_ARE_EQUAL(m_state.rect.Height, rect.Height);
            ++m_state.lockCount;
            if (FAILED(m_state.lockResult)) { return m_state.lockResult; }
            *address = m_state.pixels.data() + rect.Y * m_state.stride + rect.X * PixelBytes;
            *stride = m_state.stride;
            *width = rect.Width;
            *height = rect.Height;
            return S_OK;
        }

        HRESULT Unlock(bool update) override
        {
            m_state.updates.push_back(update);
            return S_OK;
        }

        void SetHoldFlush(bool hold) override
        {
            m_state.holdFlush = hold;
            if (hold) { ++m_state.holdCount; }
            else { ++m_state.releaseCount; }
        }

    private:
        TileState& m_state;
    };

    struct CopyObservation
    {
        WICRect rect;
        UINT stride;
        UINT size;
        BYTE* buffer;
        MEMORY_BASIC_INFORMATION memory;
    };

    class ObservingBitmapSource final : public wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::ClassicCom>, IWICBitmapSource>
    {
    public:
        explicit ObservingBitmapSource(wrl::ComPtr<IWICBitmap> bitmap) : m_bitmap(std::move(bitmap)) {}

        IFACEMETHOD(GetSize)(UINT* width, UINT* height) override { return m_bitmap->GetSize(width, height); }
        IFACEMETHOD(GetPixelFormat)(WICPixelFormatGUID* format) override { return m_bitmap->GetPixelFormat(format); }
        IFACEMETHOD(GetResolution)(double* x, double* y) override { return m_bitmap->GetResolution(x, y); }
        IFACEMETHOD(CopyPalette)(IWICPalette* palette) override { return m_bitmap->CopyPalette(palette); }

        IFACEMETHOD(CopyPixels)(const WICRect* rect, UINT stride, UINT size, BYTE* buffer) override
        {
            VERIFY_IS_NOT_NULL(rect);
            CopyObservation observation = { *rect, stride, size, buffer, {} };
            VERIFY_ARE_EQUAL(sizeof(observation.memory), VirtualQuery(buffer, &observation.memory, sizeof(observation.memory)));
            copies.push_back(observation);
            if (copies.size() == failingCopy) { return WINCODEC_ERR_BADIMAGE; }
            return m_bitmap->CopyPixels(rect, stride, size, buffer);
        }

        std::vector<CopyObservation> copies;
        size_t failingCopy = 0;

    private:
        wrl::ComPtr<IWICBitmap> m_bitmap;
    };

    enum class Failure
    {
        None,
        SecondCopy,
        Lock
    };

    void VerifyCopy(uint32_t width, uint32_t height, const std::vector<XRECT>& rects,
        bool expectVirtualAllocation, uint32_t expectedCopies, Failure failure = Failure::None, bool virtualSurface = false)
    {
        // Use real WIC pixel copying; only the GPU surface and injected error are simulated.
        std::vector<uint8_t> source(width * height * PixelBytes);
        for (uint32_t y = 0; y < height; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                const uint32_t offset = (y * width + x) * PixelBytes;
                source[offset] = static_cast<uint8_t>(x % 251);
                source[offset + 1] = static_cast<uint8_t>(y % 251);
                source[offset + 2] = static_cast<uint8_t>((x + y) % 251);
                source[offset + 3] = 255;
            }
        }
        wrl::ComPtr<IWICBitmap> bitmap;
        VERIFY_SUCCEEDED(WicService::GetInstance().GetFactory()->CreateBitmapFromMemory(
            width, height, GUID_WICPixelFormat32bppPBGRA, width * PixelBytes,
            static_cast<UINT>(source.size()), source.data(), &bitmap));
        auto observingSource = wrl::Make<ObservingBitmapSource>(bitmap);

        std::vector<std::unique_ptr<TileState>> states;
        std::vector<xref_ptr<MemoryTexture>> textures;
        SurfaceUpdateList tiles;
        for (const auto& rect : rects)
        {
            states.push_back(std::make_unique<TileState>(rect, width, height, virtualSurface));
            textures.push_back(make_xref<MemoryTexture>(*states.back()));
            tiles.push_back(make_xref<SurfaceDecodeParams>(rect, textures.back().get()));
        }
        if (failure == Failure::SecondCopy) { observingSource->failingCopy = 2; }
        if (failure == Failure::Lock) { states.front()->lockResult = DXGI_ERROR_DEVICE_REMOVED; }

        ImageMetadata metadata;
        metadata.width = width;
        metadata.height = height;
        auto params = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, width, height,
            false /* autoPlay */, tiles, true /* isLoadedImageSurface */, 0, xstring_ptr::EmptyString());
        xref_ptr<OfferableSoftwareBitmap> softwareBitmap;
        const HRESULT result = ImagingUtility::RealizeBitmapSource(metadata, observingSource.Get(), *params, softwareBitmap);

        // Query immediately, before test logging/allocations can reuse the released address.
        MEMORY_BASIC_INFORMATION after = {};
        SIZE_T queried = 0;
        if (!observingSource->copies.empty())
        {
            queried = VirtualQuery(observingSource->copies.front().buffer, &after, sizeof(after));
        }
        const HRESULT expectedResult = failure == Failure::SecondCopy ? WINCODEC_ERR_BADIMAGE :
            failure == Failure::Lock ? DXGI_ERROR_DEVICE_REMOVED : S_OK;
        VERIFY_ARE_EQUAL(expectedResult, result);
        VERIFY_IS_NULL(softwareBitmap.get());
        VERIFY_ARE_EQUAL(static_cast<size_t>(expectedCopies), observingSource->copies.size());

        const auto& first = observingSource->copies.front();
        VERIFY_ARE_EQUAL(static_cast<DWORD>(MEM_COMMIT), first.memory.State);
        VERIFY_ARE_EQUAL(expectVirtualAllocation, first.memory.AllocationBase == first.buffer);
        if (expectVirtualAllocation)
        {
            VERIFY_ARE_EQUAL(sizeof(after), queried);
            VERIFY_ARE_EQUAL(static_cast<DWORD>(MEM_FREE), after.State);
        }
        for (const auto& copy : observingSource->copies)
        {
            VERIFY_ARE_EQUAL(first.buffer, copy.buffer);
            VERIFY_ARE_EQUAL(width * PixelBytes, copy.stride);
            VERIFY_ARE_EQUAL(copy.stride * static_cast<UINT>(copy.rect.Height), copy.size);
        }
        if (rects.size() == 1 && expectedCopies == 2)
        {
            VERIFY_ARE_EQUAL(256, first.rect.Height);
            VERIFY_ARE_EQUAL(256, observingSource->copies.back().rect.Y);
            VERIFY_ARE_EQUAL(44, observingSource->copies.back().rect.Height);
            VERIFY_ARE_EQUAL(1024u * 1024u, first.size);
            VERIFY_ARE_EQUAL(44u * 4096u, observingSource->copies.back().size);
        }
        for (const auto& state : states)
        {
            VERIFY_IS_FALSE(state->holdFlush);
            VERIFY_ARE_EQUAL(1u, state->holdCount);
            VERIFY_ARE_EQUAL(1u, state->releaseCount);
            const uint32_t copiedHeight = failure == Failure::None ? state->rect.Height :
                failure == Failure::SecondCopy ? 256 : 0;
            auto expectedPixels = std::vector<uint8_t>(state->pixels.size(), PaddingByte);
            const uint32_t destinationX = state->virtualSurface ? state->rect.X : 0;
            const uint32_t destinationY = state->virtualSurface ? state->rect.Y : 0;
            for (uint32_t y = 0; y < copiedHeight; ++y)
            {
                memcpy(expectedPixels.data() + (destinationY + y) * state->stride + destinationX * PixelBytes,
                    source.data() + ((state->rect.Y + y) * width + state->rect.X) * PixelBytes,
                    state->rect.Width * PixelBytes);
            }
            VERIFY_IS_TRUE(expectedPixels == state->pixels);
            if (failure == Failure::None)
            {
                VERIFY_IS_FALSE(state->updates.empty());
                VERIFY_IS_TRUE(state->updates.back());
                VERIFY_ARE_EQUAL(state->lockCount, static_cast<uint32_t>(state->updates.size()));
            }
            else if (failure == Failure::SecondCopy)
            {
                VERIFY_ARE_EQUAL(1u, state->lockCount);
                VERIFY_ARE_EQUAL(size_t(1), state->updates.size());
                VERIFY_IS_FALSE(state->updates.front());
            }
            else
            {
                VERIFY_ARE_EQUAL(failure == Failure::Lock ? 1u : 0u, state->lockCount);
                VERIFY_IS_TRUE(state->updates.empty());
            }
        }
    }
}

using namespace Windows::UI::Xaml::Tests::Foundation::Imaging;

bool HardwareTileCopyTests::ClassSetup()
{
    CoInitHelper::EnsureCoInitialized();
    return true;
}

void HardwareTileCopyTests::BelowThresholdUsesHeap()
{
    VerifyCopy(96, 96, { {0, 0, 96, 96} }, false, 1);
    VerifyCopy(127, 256, { {0, 0, 127, 256} }, false, 1);
}

void HardwareTileCopyTests::AtThresholdReleasesScratch()
{
    VerifyCopy(128, 256, { {0, 0, 128, 256} }, true, 1);
}

void HardwareTileCopyTests::AboveThresholdReleasesScratch()
{
    VerifyCopy(129, 256, { {0, 0, 129, 256} }, true, 1);
}

void HardwareTileCopyTests::FinalStripReleasesOriginalAllocation()
{
    VerifyCopy(1024, 300, { {0, 0, 1024, 300} }, true, 2);
}

void HardwareTileCopyTests::VirtualTilesPreservePixels()
{
    VerifyCopy(512, 129, { {256, 64, 256, 64}, {0, 0, 256, 64}, {256, 128, 256, 1},
        {256, 0, 256, 64}, {0, 128, 256, 1}, {0, 64, 256, 64} }, true, 3, Failure::None, true);
}

void HardwareTileCopyTests::CopyPixelsFailureReleasesScratch()
{
    VerifyCopy(1024, 300, { {0, 0, 1024, 300} }, true, 2, Failure::SecondCopy);
}

void HardwareTileCopyTests::LockFailureReleasesScratch()
{
    VerifyCopy(128, 256, { {0, 0, 128, 256} }, true, 1, Failure::Lock);
}
