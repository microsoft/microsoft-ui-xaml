// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GdiPlusPainterTests.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <RenderingBackend.h>
#include <cmath>
#include <vector>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwrite.h>
#include <wrl/client.h>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using namespace WEX::TestExecution;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        constexpr uint32_t c_size = 64;
        constexpr ColorF c_white{ 1, 1, 1, 1 };
        constexpr ColorF c_black{ 0, 0, 0, 1 };
        constexpr ColorF c_transparent{ 0, 0, 0, 0 };

        struct Pixel
        {
            uint8_t b, g, r, a;
        };

        struct Image
        {
            uint32_t width = 0;
            uint32_t height = 0;
            std::vector<uint8_t> bytes;

            Pixel At(uint32_t x, uint32_t y) const
            {
                const uint8_t* p = &bytes[(static_cast<size_t>(y) * width + x) * 4];
                return { p[0], p[1], p[2], p[3] };
            }
        };

        bool Near(int actual, int expected, int tolerance)
        {
            return std::abs(actual - expected) <= tolerance;
        }

        bool IsPremultiplied(const Pixel& p, int b, int g, int r, int a, int tolerance = 2)
        {
            return Near(p.b, b, tolerance) && Near(p.g, g, tolerance) && Near(p.r, r, tolerance) && Near(p.a, a, tolerance);
        }

        bool IsPartial(uint8_t channel)
        {
            return channel > 24 && channel < 231;
        }

        void LogPixel(const wchar_t* label, uint32_t x, uint32_t y, const Pixel& p)
        {
            Log::Comment(String().Format(L"%s (%u,%u) = B%u G%u R%u A%u", label, x, y, p.b, p.g, p.r, p.a));
        }

        // Fails the test through VERIFY (no crash) while CreateGdiPlusPainter is still the E_NOTIMPL stub.
        std::unique_ptr<IImmediatePainter> CreateGdiPlus(uint32_t width = c_size, uint32_t height = c_size)
        {
            std::unique_ptr<IImmediatePainter> painter;
            const HRESULT hr = CreateImmediatePainter(Mode::ImmediateGdiPlus, PainterOptions{ width, height, 1.0f }, painter);
            VERIFY_SUCCEEDED(hr, L"CreateImmediatePainter(gdiplus)");
            VERIFY_IS_NOT_NULL(painter.get());
            return painter;
        }

        Image Read(IImmediatePainter& painter, uint32_t width = c_size, uint32_t height = c_size)
        {
            Image image{ width, height, std::vector<uint8_t>(static_cast<size_t>(width) * height * 4) };
            VERIFY_SUCCEEDED(painter.ReadPixels(image.bytes.data(), width * 4, static_cast<uint32_t>(image.bytes.size())));
            return image;
        }

        template <typename PaintFn>
        FrameStats Paint(IImmediatePainter& painter, PaintFn&& paint, const RectF* damage = nullptr, uint32_t damageCount = 0)
        {
            VERIFY_SUCCEEDED(painter.BeginFrame(damage, damageCount));
            paint(painter);
            FrameStats stats{};
            VERIFY_SUCCEEDED(painter.EndFrame(&stats));
            return stats;
        }

        GdiPlus::ResourceChurn LastChurn(const IImmediatePainter& painter)
        {
            GdiPlus::ResourceChurn churn{};
            VERIFY_SUCCEEDED(GdiPlus::GetLastFrameResourceChurn(painter, &churn));
            return churn;
        }

        // S2-like (dashboard) primitive mix: 4x4 tiles with rounded fill, border, changing text, clip, and a translucent
        // bar under group opacity. Colors come from a fixed palette and text from a bounded set, so a correct cache
        // reaches a steady size during warm-up.
        constexpr uint32_t c_dashboardWidth = 256;
        constexpr uint32_t c_dashboardHeight = 192;

        void PaintDashboard(IImmediatePainter& p, uint32_t frame)
        {
            static constexpr ColorF palette[] =
            {
                { 0.95f, 0.95f, 0.97f, 1 }, { 0.87f, 0.93f, 1.0f, 1 }, { 1.0f, 0.94f, 0.86f, 1 }, { 0.9f, 1.0f, 0.9f, 1 },
            };
            p.Clear({ 0.98f, 0.98f, 0.98f, 1 });
            TextStyle style{};
            style.fontSize = 12;
            for (uint32_t i = 0; i < 16; ++i)
            {
                const float left = 4.0f + (i % 4) * 63.0f;
                const float top = 4.0f + (i / 4) * 47.0f;
                const RectF tile{ left, top, left + 59, top + 43 };
                p.PushAxisAlignedClip(tile);
                p.FillRoundedRectangle(tile, 4, 4, palette[(i + frame) % 4]);
                p.StrokeRoundedRectangle({ tile.left + 0.5f, tile.top + 0.5f, tile.right - 0.5f, tile.bottom - 0.5f }, 4, 4, 1, { 0.8f, 0.8f, 0.8f, 1 });
                wchar_t text[8]{};
                VERIFY_SUCCEEDED(StringCchPrintfW(text, ARRAYSIZE(text), L"%u%%", (i * 7 + frame) % 100));
                p.DrawText(text, static_cast<uint32_t>(wcslen(text)), style, { left + 6, top + 4, left + 56, top + 22 }, c_black);
                if (i % 4 == 0)
                {
                    p.PushOpacityLayer(0.6f);
                    p.FillRectangle({ left + 6, top + 28, left + 6 + static_cast<float>((frame + i) % 40), top + 34 }, { 0, 0.47f, 0.84f, 1 });
                    p.PopOpacityLayer();
                }
                p.PopAxisAlignedClip();
            }
        }

        uint32_t GdiObjectCount()
        {
            return GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
        }

        // The GDI+ painter has no device of its own; the root's swap chain can live on any D3D11 device. WARP keeps the
        // tests hardware-independent.
        ComPtr<ID3D11Device> CreateWarpDevice()
        {
            ComPtr<ID3D11Device> device;
            VERIFY_SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                D3D11_SDK_VERSION, &device, nullptr, nullptr));
            return device;
        }

        ComPtr<IDXGISwapChain1> CreateCompositionSwapChain(ID3D11Device* device, uint32_t width, uint32_t height,
            DXGI_FORMAT format = DXGI_FORMAT_B8G8R8A8_UNORM)
        {
            ComPtr<IDXGIDevice> dxgiDevice;
            ComPtr<IDXGIAdapter> adapter;
            ComPtr<IDXGIFactory2> factory;
            VERIFY_SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));
            VERIFY_SUCCEEDED(dxgiDevice->GetAdapter(&adapter));
            VERIFY_SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&factory)));

            DXGI_SWAP_CHAIN_DESC1 desc{};
            desc.Width = width;
            desc.Height = height;
            desc.Format = format;
            desc.SampleDesc.Count = 1;
            desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
            desc.BufferCount = 2;
            desc.Scaling = DXGI_SCALING_STRETCH;
            desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
            desc.AlphaMode = DXGI_ALPHA_MODE_PREMULTIPLIED;
            ComPtr<IDXGISwapChain1> swapChain;
            VERIFY_SUCCEEDED(factory->CreateSwapChainForComposition(device, &desc, nullptr, &swapChain));
            return swapChain;
        }

        DXGI_SWAP_CHAIN_DESC1 SwapChainDesc(IDXGISwapChain1* swapChain)
        {
            DXGI_SWAP_CHAIN_DESC1 desc{};
            VERIFY_SUCCEEDED(swapChain->GetDesc1(&desc));
            return desc;
        }

        // Reads the swap chain's current back buffer (buffer 0) through a staging copy.
        Image ReadBackBuffer(ID3D11Device* device, IDXGISwapChain1* swapChain)
        {
            ComPtr<ID3D11Texture2D> backBuffer;
            VERIFY_SUCCEEDED(swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));
            D3D11_TEXTURE2D_DESC desc{};
            backBuffer->GetDesc(&desc);
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            desc.MiscFlags = 0;
            ComPtr<ID3D11Texture2D> staging;
            VERIFY_SUCCEEDED(device->CreateTexture2D(&desc, nullptr, &staging));
            ComPtr<ID3D11DeviceContext> context;
            device->GetImmediateContext(&context);
            context->CopyResource(staging.Get(), backBuffer.Get());

            Image image{ desc.Width, desc.Height, std::vector<uint8_t>(static_cast<size_t>(desc.Width) * desc.Height * 4) };
            D3D11_MAPPED_SUBRESOURCE mapped{};
            VERIFY_SUCCEEDED(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            for (uint32_t y = 0; y < desc.Height; ++y)
            {
                memcpy(&image.bytes[static_cast<size_t>(y) * desc.Width * 4], static_cast<const uint8_t*>(mapped.pData) + static_cast<size_t>(y) * mapped.RowPitch, static_cast<size_t>(desc.Width) * 4);
            }
            context->Unmap(staging.Get(), 0);
            return image;
        }
    }

    // ---- GDI+ runtime lifetime ----

    void GdiPlusPainterTests::RuntimeReferencesAreRefcounted()
    {
        const uint32_t baseline = GdiPlus::GetRuntimeReferenceCount();
        {
            GdiPlus::RuntimeReference first;
            GdiPlus::RuntimeReference second;
            VERIFY_SUCCEEDED(first.Acquire());
            VERIFY_ARE_EQUAL(baseline + 1, GdiPlus::GetRuntimeReferenceCount());
            VERIFY_IS_TRUE(GdiPlus::IsRuntimeStarted());
            VERIFY_SUCCEEDED(first.Acquire());
            VERIFY_ARE_EQUAL(baseline + 1, GdiPlus::GetRuntimeReferenceCount(), L"re-acquiring a held reference is a no-op");
            VERIFY_SUCCEEDED(second.Acquire());
            VERIFY_ARE_EQUAL(baseline + 2, GdiPlus::GetRuntimeReferenceCount());
            first.Release();
            first.Release();
            VERIFY_ARE_EQUAL(baseline + 1, GdiPlus::GetRuntimeReferenceCount(), L"double release is a no-op");
            VERIFY_IS_TRUE(GdiPlus::IsRuntimeStarted());
        }
        VERIFY_ARE_EQUAL(baseline, GdiPlus::GetRuntimeReferenceCount(), L"destructor releases");
        if (baseline == 0)
        {
            VERIFY_IS_FALSE(GdiPlus::IsRuntimeStarted(), L"last release shuts GDI+ down");
            GdiPlus::RuntimeReference again;
            VERIFY_SUCCEEDED(again.Acquire(), L"GDI+ restarts after shutdown");
            VERIFY_IS_TRUE(GdiPlus::IsRuntimeStarted());
        }
    }

    void GdiPlusPainterTests::PaintersShareOneRuntime()
    {
        const uint32_t baseline = GdiPlus::GetRuntimeReferenceCount();
        auto first = CreateGdiPlus();
        if (!first) return;
        VERIFY_ARE_EQUAL(baseline + 1, GdiPlus::GetRuntimeReferenceCount());
        auto second = CreateGdiPlus();
        if (!second) return;
        VERIFY_ARE_EQUAL(baseline + 2, GdiPlus::GetRuntimeReferenceCount());

        first.reset();
        VERIFY_ARE_EQUAL(baseline + 1, GdiPlus::GetRuntimeReferenceCount());
        VERIFY_IS_TRUE(GdiPlus::IsRuntimeStarted(), L"a live painter keeps GDI+ running");
        Paint(*second, [](IImmediatePainter& p) { p.Clear(c_black); });
        VERIFY_IS_TRUE(IsPremultiplied(Read(*second).At(1, 1), 0, 0, 0, 255), L"surviving painter still renders");
    }

    void GdiPlusPainterTests::LastPainterShutsDownRuntime()
    {
        const uint32_t baseline = GdiPlus::GetRuntimeReferenceCount();
        auto first = CreateGdiPlus();
        if (!first) return;
        auto second = CreateGdiPlus();
        if (!second) return;
        second.reset();
        first.reset();
        VERIFY_ARE_EQUAL(baseline, GdiPlus::GetRuntimeReferenceCount());
        if (baseline == 0)
        {
            VERIFY_IS_FALSE(GdiPlus::IsRuntimeStarted(), L"destroying the last painter calls GdiplusShutdown");
        }
    }

    void GdiPlusPainterTests::PainterCanBeRecreatedAfterShutdown()
    {
        for (int round = 0; round < 3; ++round)
        {
            auto painter = CreateGdiPlus();
            if (!painter) return;
            Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.FillRectangle({ 8, 8, 24, 24 }, c_black);
            });
            const auto image = Read(*painter);
            VERIFY_IS_TRUE(IsPremultiplied(image.At(16, 16), 0, 0, 0, 255), String().Format(L"round %d renders", round));
            VERIFY_IS_TRUE(IsPremultiplied(image.At(40, 40), 255, 255, 255, 255));
        }
    }

    // ---- PARGB target ----

    void GdiPlusPainterTests::HalfAlphaFillReadsBackPremultiplied()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.FillRectangle({ 8, 8, 32, 56 }, { 1, 0, 0, 0.5f });
            p.FillRectangle({ 32, 8, 56, 56 }, { 0.2f, 0.6f, 1.0f, 0.5f });
            // Source-over twice: alpha 0.75, red 0.75 in premultiplied space.
            p.FillRectangle({ 8, 40, 32, 56 }, { 1, 0, 0, 0.5f });
        });
        const auto image = Read(*painter);
        const auto red = image.At(16, 16);
        const auto blue = image.At(44, 16);
        const auto twice = image.At(16, 48);
        LogPixel(L"red50", 16, 16, red);
        LogPixel(L"mixed50", 44, 16, blue);
        LogPixel(L"red50x2", 16, 48, twice);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(4, 4), 0, 0, 0, 0), L"transparent clear stays zero");
        VERIFY_IS_TRUE(IsPremultiplied(red, 0, 0, 128, 128), L"50% red reads back premultiplied (R128 A128), not R255");
        VERIFY_IS_TRUE(IsPremultiplied(blue, 128, 77, 26, 128), L"each channel scaled by alpha");
        VERIFY_IS_TRUE(IsPremultiplied(twice, 0, 0, 191, 191), L"premultiplied source-over blend");
    }

    // ---- Quality settings ----

    void GdiPlusPainterTests::GeometryIsAntialiased()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            // 30 degrees around the target center.
            const float c = std::cos(0.5235988f);
            const float s = std::sin(0.5235988f);
            Matrix3x2F rotate{ c, s, -s, c, 0, 0 };
            rotate.dx = 32 - (32 * c - 32 * s);
            rotate.dy = 32 - (32 * s + 32 * c);
            p.SetTransform(rotate);
            p.FillRectangle({ 16, 16, 48, 48 }, c_black);
            p.SetTransform({});
        });
        const auto image = Read(*painter);
        uint32_t partial = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                partial += IsPartial(image.At(x, y).r) ? 1 : 0;
            }
        }
        Log::Comment(String().Format(L"partially covered pixels on the rotated edges: %u", partial));
        VERIFY_IS_TRUE(partial >= 32, L"SmoothingModeAntiAlias: a rotated square has partially covered edge pixels");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(32, 32), 0, 0, 0, 255));
    }

    void GdiPlusPainterTests::GeometryUsesHalfPixelOffset()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 8, 8, 24, 24 }, c_black);
            p.FillRectangle({ 40.25f, 8, 56.25f, 24 }, c_black);
        });
        const auto image = Read(*painter);
        // Integer-aligned edges are crisp only when pixel x covers [x, x+1] (PixelOffsetModeHalf, as in D2D). With the
        // GDI+ default, pixel centers sit on integers and every edge pixel would be half covered.
        VERIFY_IS_TRUE(IsPremultiplied(image.At(7, 16), 255, 255, 255, 255, 1), L"left of rect untouched");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 16), 0, 0, 0, 255, 1), L"first column fully covered");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(23, 16), 0, 0, 0, 255, 1), L"last column fully covered");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(24, 16), 255, 255, 255, 255, 1), L"right of rect untouched");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(16, 7), 255, 255, 255, 255, 1), L"above rect untouched");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(16, 23), 0, 0, 0, 255, 1), L"last row fully covered");

        // Edge at x = 40.25: half offset => pixel 40 is 75% covered (~64); integer centers => 25% (~191). Mirror at 56.25.
        const auto leftEdge = image.At(40, 16);
        const auto rightEdge = image.At(56, 16);
        LogPixel(L"edge 40.25", 40, 16, leftEdge);
        LogPixel(L"edge 56.25", 56, 16, rightEdge);
        VERIFY_IS_TRUE(Near(leftEdge.r, 64, 24), L"pixel 40 is ~75% covered");
        VERIFY_IS_TRUE(Near(rightEdge.r, 191, 24), L"pixel 56 is ~25% covered");
    }

    void GdiPlusPainterTests::TextUsesGrayscaleAntialiasing()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        TextStyle style{};
        style.fontSize = 24;

        // Black on opaque white: grayscale AA keeps R == G == B; ClearType would leave colored fringes.
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawText(L"Hgw", 3, style, { 2, 2, 62, 40 }, c_black);
        });
        auto image = Read(*painter);
        uint32_t ink = 0, partial = 0, fringe = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                const auto p = image.At(x, y);
                if (p.r == 255 && p.g == 255 && p.b == 255) continue;
                ++ink;
                partial += IsPartial(p.g) ? 1 : 0;
                const int spread = std::max({ p.r, p.g, p.b }) - std::min({ p.r, p.g, p.b });
                if (spread > 2)
                {
                    if (fringe++ < 4) LogPixel(L"color fringe", x, y, p);
                }
            }
        }
        Log::Comment(String().Format(L"opaque: ink %u, partial %u, fringe %u", ink, partial, fringe));
        VERIFY_IS_TRUE(ink > 20);
        VERIFY_IS_TRUE(partial > 0, L"text is antialiased");
        VERIFY_ARE_EQUAL(0u, fringe, L"no ClearType color fringes");

        // White on transparent: coverage must land in alpha, with R == G == B == A (premultiplied grayscale).
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawText(L"Hgw", 3, style, { 2, 2, 62, 40 }, c_white);
        });
        image = Read(*painter);
        ink = partial = fringe = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                const auto p = image.At(x, y);
                if (p.a == 0 && p.r == 0 && p.g == 0 && p.b == 0) continue;
                ++ink;
                partial += IsPartial(p.a) ? 1 : 0;
                if (!Near(p.r, p.a, 2) || !Near(p.g, p.a, 2) || !Near(p.b, p.a, 2))
                {
                    if (fringe++ < 4) LogPixel(L"not grayscale premultiplied", x, y, p);
                }
            }
        }
        Log::Comment(String().Format(L"transparent: ink %u, partial alpha %u, mismatched %u", ink, partial, fringe));
        VERIFY_IS_TRUE(ink > 20);
        VERIFY_IS_TRUE(partial > 0, L"antialiasing coverage is stored in alpha");
        VERIFY_ARE_EQUAL(0u, fringe, L"grayscale AA on an alpha target: R == G == B == A");
    }

    // ---- Leaks and churn ----

    void GdiPlusPainterTests::SteadyFramesDoNotLeakGdiObjectsOrCache()
    {
        auto painter = CreateGdiPlus(c_dashboardWidth, c_dashboardHeight);
        if (!painter) return;
        const RectF tileDamage{ 4, 4, 130, 98 };
        auto frame = [&](uint32_t n)
        {
            // Alternate full and partial damage so both paths are exercised.
            const bool partial = (n % 2) == 1;
            VERIFY_SUCCEEDED(PaintTracedFrame(*painter, n, 1, partial ? &tileDamage : nullptr, partial ? 1u : 0u,
                [n](IImmediatePainter& p) { PaintDashboard(p, n); }));
        };

        constexpr uint32_t c_warmup = 20;
        constexpr uint32_t c_frames = 200;
        for (uint32_t n = 0; n < c_warmup; ++n) frame(n);
        const uint32_t gdiBefore = GdiObjectCount();
        const uint32_t userBefore = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
        const auto inventoryBefore = painter->GetResourceInventory();

        for (uint32_t n = c_warmup; n < c_warmup + c_frames; ++n) frame(n);
        const uint32_t gdiAfter = GdiObjectCount();
        const uint32_t userAfter = GetGuiResources(GetCurrentProcess(), GR_USEROBJECTS);
        const auto inventoryAfter = painter->GetResourceInventory();

        Log::Comment(String().Format(L"GDI objects %u -> %u, USER objects %u -> %u, cached objects %u -> %u, cache bytes %llu -> %llu",
            gdiBefore, gdiAfter, userBefore, userAfter, inventoryBefore.cachedObjectCount, inventoryAfter.cachedObjectCount,
            inventoryBefore.cacheBytes, inventoryAfter.cacheBytes));
        VERIFY_IS_TRUE(gdiAfter <= gdiBefore, L"no GDI handle growth after warm-up");
        VERIFY_IS_TRUE(userAfter <= userBefore, L"no USER handle growth after warm-up");
        VERIFY_IS_TRUE(inventoryAfter.cachedObjectCount <= inventoryBefore.cachedObjectCount, L"resource cache reached steady size");
        VERIFY_ARE_EQUAL(inventoryBefore.surfaceBytes, inventoryAfter.surfaceBytes);

        painter.reset();
        VERIFY_IS_TRUE(GdiObjectCount() <= gdiBefore, L"destroying the painter releases its GDI objects");
    }

    void GdiPlusPainterTests::IdenticalFrameCreatesNoResources()
    {
        auto painter = CreateGdiPlus(c_dashboardWidth, c_dashboardHeight);
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { PaintDashboard(p, 0); });
        const auto first = LastChurn(*painter);
        TraceResourceChurn(Mode::ImmediateGdiPlus, 1, first.created, first.destroyed);

        Paint(*painter, [](IImmediatePainter& p) { PaintDashboard(p, 0); });
        const auto second = LastChurn(*painter);
        TraceResourceChurn(Mode::ImmediateGdiPlus, 2, second.created, second.destroyed);

        Log::Comment(String().Format(L"churn: first %u/%u, second %u/%u (created/destroyed)",
            first.created, first.destroyed, second.created, second.destroyed));
        VERIFY_IS_TRUE(first.created > 0, L"the first frame creates brushes, pens, and fonts (counter is live)");
        VERIFY_ARE_EQUAL(0u, second.created, L"an identical frame reuses cached brushes, pens, and fonts");
        VERIFY_ARE_EQUAL(0u, second.destroyed, L"and does not evict them");
    }

    // ---- Upload/copy accounting ----

    void GdiPlusPainterTests::DamageCopyBytesCountsClippedDamage()
    {
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(64 * 48), GdiPlus::DamagedPixelCount(64, 48, nullptr, 0), L"no damage list => full target");
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(64 * 48 * 4), GdiPlus::DamageCopyBytes(64, 48, nullptr, 0));

        const RectF one{ 8, 8, 40, 24 };
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(32 * 16), GdiPlus::DamagedPixelCount(64, 64, &one, 1));
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(32 * 16 * 4), GdiPlus::DamageCopyBytes(64, 64, &one, 1));

        const RectF fractional{ 7.5f, 8.25f, 9.1f, 9.9f }; // rounds outward to [7,10) x [8,10)
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(3 * 2), GdiPlus::DamagedPixelCount(64, 64, &fractional, 1));

        const RectF offTarget[] = { { -10, -10, 4, 4 }, { 60, 60, 100, 100 }, { 70, 0, 80, 10 }, { 20, 20, 10, 30 } };
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(16 + 16), GdiPlus::DamagedPixelCount(64, 64, offTarget, ARRAYSIZE(offTarget)),
            L"clipped to the target; outside and inverted rects copy nothing");

        const RectF nonFinite{ std::numeric_limits<float>::quiet_NaN(), 0, 8, 8 };
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(64), GdiPlus::DamagedPixelCount(64, 64, &nonFinite, 1), L"NaN edge clamps to 0");
    }

    void GdiPlusPainterTests::PartialDamageUploadsOnlyDamagedPixels()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"no swap chain: offscreen only, nothing copied");

        auto device = CreateWarpDevice();
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));

        const RectF damage{ 8, 8, 40, 24 };
        auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &damage, 1);
        auto inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(c_size) * c_size, stats.damagedPixels, L"first frame after attach is fully damaged");
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(c_size) * c_size * 4, inventory.uploadBytesThisFrame, L"full frame copies the whole bitmap");
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, inventory.uploadBytesThisFrame);

        stats = Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_black);
            p.FillRectangle({ 0, 0, 64, 64 }, { 1, 0, 0, 1 });
        }, &damage, 1);
        inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"damaged %llu px, upload %llu bytes", stats.damagedPixels, inventory.uploadBytesThisFrame));
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(32 * 16), stats.damagedPixels);
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, inventory.uploadBytesThisFrame, L"only the damage is copied");
        VERIFY_ARE_EQUAL(GdiPlus::DamageCopyBytes(c_size, c_size, &damage, 1), inventory.uploadBytesThisFrame);

        const RectF twoRects[] = { { 0, 0, 8, 8 }, { 32, 32, 48, 40 } };
        stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, twoRects, ARRAYSIZE(twoRects));
        inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(2u, stats.damageRectCount);
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(64 + 128), stats.damagedPixels);
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, inventory.uploadBytesThisFrame);

        const RectF offTarget{ 48, 48, 80, 80 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &offTarget, 1);
        VERIFY_ARE_EQUAL(16ull * 16 * 4, painter->GetResourceInventory().uploadBytesThisFrame, L"damage outside the target is not copied");

        Read(*painter);
        VERIFY_ARE_EQUAL(16ull * 16 * 4, painter->GetResourceInventory().uploadBytesThisFrame, L"ReadPixels is not counted");

        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(nullptr));
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &damage, 1);
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"detached: offscreen only again");
    }

    void GdiPlusPainterTests::PresentationCopiesDamageIntoBackBuffer()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        auto device = CreateWarpDevice();
        // Attaching resizes a mismatched swap chain to the painter's target.
        auto swapChain = CreateCompositionSwapChain(device.Get(), 32, 16);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        VERIFY_ARE_EQUAL(c_size, SwapChainDesc(swapChain.Get()).Width);
        VERIFY_ARE_EQUAL(c_size, SwapChainDesc(swapChain.Get()).Height);

        // Two buffers: after frames 1 (red, into A) and 2 (blue, into B), buffer 0 is A again and holds frame 1.
        const ColorF red{ 1, 0, 0, 1 };
        const ColorF blue{ 0, 0, 1, 0.5f };
        Paint(*painter, [&](IImmediatePainter& p) { p.Clear(red); });
        Paint(*painter, [&](IImmediatePainter& p) { p.Clear(blue); });
        auto image = ReadBackBuffer(device.Get(), swapChain.Get());
        LogPixel(L"frame 1 in buffer A", 8, 8, image.At(8, 8));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 8), 0, 0, 255, 255), L"full frame copied into the back buffer");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(63, 63), 0, 0, 255, 255));

        // Frame 3 (into A) damages the left half with premultiplied translucent green; frame 4 (into B) is tiny, so
        // buffer 0 is A again and shows frame 3's damage.
        const RectF left{ 0, 0, 32, 64 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear({ 0, 1, 0, 0.5f }); }, &left, 1);
        const RectF corner{ 63, 0, 64, 1 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &corner, 1);
        image = ReadBackBuffer(device.Get(), swapChain.Get());
        LogPixel(L"frame 3 damage in buffer A", 8, 40, image.At(8, 40));
        LogPixel(L"outside frame 3 damage in buffer A", 48, 40, image.At(48, 40));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 40), 0, 128, 0, 128), L"damaged pixels copied unchanged (premultiplied BGRA)");
    }

    void GdiPlusPainterTests::ResizeResizesAttachedSwapChain()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        auto device = CreateWarpDevice();
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });

        VERIFY_SUCCEEDED(painter->Resize(96, 48, 1.0f));
        VERIFY_ARE_EQUAL(96u, SwapChainDesc(swapChain.Get()).Width);
        VERIFY_ARE_EQUAL(48u, SwapChainDesc(swapChain.Get()).Height);

        const RectF damage{ 0, 0, 8, 8 };
        const auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_black); }, &damage, 1);
        VERIFY_ARE_EQUAL(96ull * 48, stats.damagedPixels, L"first frame after Resize is fully damaged");
        VERIFY_ARE_EQUAL(96ull * 48 * 4, painter->GetResourceInventory().uploadBytesThisFrame);
        const auto image = ReadBackBuffer(device.Get(), swapChain.Get());
        VERIFY_ARE_EQUAL(96u, image.width);
        VERIFY_IS_TRUE(IsPremultiplied(Read(*painter, 96, 48).At(90, 40), 0, 0, 0, 255), L"whole resized target painted");
    }

    void GdiPlusPainterTests::RejectsNonBgraSwapChain()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        auto device = CreateWarpDevice();
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size, DXGI_FORMAT_R8G8B8A8_UNORM);
        VERIFY_ARE_EQUAL(E_INVALIDARG, painter->SetPresentationSwapChain(swapChain.Get()));
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame, L"rejected swap chain is not attached");
    }

    // ---- Extended primitives ----

    namespace
    {
        // Segoe UI glyphs for text with unshaped (cmap + design advance) positions; enough for GDI+ DrawDriverString.
        struct TestGlyphs
        {
            ComPtr<IDWriteFontFace> face;
            std::vector<uint16_t> indices;
            std::vector<float> advances;
        };

        TestGlyphs MakeGlyphs(const wchar_t* text, float emSize)
        {
            TestGlyphs glyphs;
            ComPtr<IDWriteFactory> factory;
            VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())));
            ComPtr<IDWriteFontCollection> collection;
            VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection));
            UINT32 familyIndex = 0;
            BOOL exists = FALSE;
            VERIFY_SUCCEEDED(collection->FindFamilyName(L"Segoe UI", &familyIndex, &exists));
            VERIFY_IS_TRUE(!!exists);
            ComPtr<IDWriteFontFamily> family;
            VERIFY_SUCCEEDED(collection->GetFontFamily(familyIndex, &family));
            ComPtr<IDWriteFont> font;
            VERIFY_SUCCEEDED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font));
            VERIFY_SUCCEEDED(font->CreateFontFace(&glyphs.face));

            const size_t length = wcslen(text);
            std::vector<UINT32> codePoints(text, text + length);
            glyphs.indices.resize(length);
            VERIFY_SUCCEEDED(glyphs.face->GetGlyphIndices(codePoints.data(), static_cast<UINT32>(length), glyphs.indices.data()));
            DWRITE_FONT_METRICS fontMetrics{};
            glyphs.face->GetMetrics(&fontMetrics);
            std::vector<DWRITE_GLYPH_METRICS> metrics(length);
            VERIFY_SUCCEEDED(glyphs.face->GetDesignGlyphMetrics(glyphs.indices.data(), static_cast<UINT32>(length), metrics.data(), FALSE));
            for (const auto& metric : metrics)
            {
                glyphs.advances.push_back(metric.advanceWidth * emSize / fontMetrics.designUnitsPerEm);
            }
            return glyphs;
        }

        GlyphRunDesc MakeRun(const TestGlyphs& glyphs, float emSize, PointF origin, uint32_t bidiLevel = 0)
        {
            GlyphRunDesc run{};
            run.fontFace = glyphs.face.Get();
            run.fontEmSize = emSize;
            run.glyphIndices = glyphs.indices.data();
            run.glyphAdvances = glyphs.advances.data();
            run.glyphCount = static_cast<uint32_t>(glyphs.indices.size());
            run.baselineOrigin = origin;
            run.bidiLevel = bidiLevel;
            return run;
        }

        // Bounding box of pixels that differ from opaque white; right/bottom exclusive. Empty: left >= right.
        struct InkBounds
        {
            uint32_t left = UINT32_MAX, top = UINT32_MAX, right = 0, bottom = 0, count = 0;
        };

        InkBounds FindInk(const Image& image)
        {
            InkBounds ink;
            for (uint32_t y = 0; y < image.height; ++y)
            {
                for (uint32_t x = 0; x < image.width; ++x)
                {
                    const auto p = image.At(x, y);
                    if (p.r == 255 && p.g == 255 && p.b == 255) continue;
                    ++ink.count;
                    ink.left = std::min(ink.left, x);
                    ink.top = std::min(ink.top, y);
                    ink.right = std::max(ink.right, x + 1);
                    ink.bottom = std::max(ink.bottom, y + 1);
                }
            }
            return ink;
        }

        // A triangle: three verbs, closed.
        constexpr PathVerb c_triangleVerbs[] = { PathVerb::MoveTo, PathVerb::LineTo, PathVerb::LineTo, PathVerb::Close };
        constexpr PointF c_trianglePoints[] = { { 8, 8 }, { 56, 8 }, { 8, 56 } };

        PathData Triangle(uint64_t cacheKey)
        {
            PathData path{};
            path.verbs = c_triangleVerbs;
            path.verbCount = ARRAYSIZE(c_triangleVerbs);
            path.points = c_trianglePoints;
            path.pointCount = ARRAYSIZE(c_trianglePoints);
            path.cacheKey = cacheKey;
            return path;
        }

        // Opaque premultiplied BGRA image with one color.
        std::vector<uint8_t> SolidPixels(uint32_t width, uint32_t height, uint8_t b, uint8_t g, uint8_t r)
        {
            std::vector<uint8_t> pixels(static_cast<size_t>(width) * height * 4);
            for (size_t i = 0; i < pixels.size(); i += 4)
            {
                pixels[i] = b;
                pixels[i + 1] = g;
                pixels[i + 2] = r;
                pixels[i + 3] = 255;
            }
            return pixels;
        }

        // Each row y gets a distinct opaque gray (4 * y), so moved rows are identifiable.
        void PaintRowRamp(IImmediatePainter& p)
        {
            p.Clear(c_white);
            for (uint32_t y = 0; y < c_size; ++y)
            {
                const float v = (4.0f * y) / 255.0f;
                p.FillRectangle({ 0, static_cast<float>(y), static_cast<float>(c_size), static_cast<float>(y + 1) }, { v, v, v, 1 });
            }
        }
    }

    void GdiPlusPainterTests::RoundedClipIsAntialiasedInsideOpacityLayer()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushOpacityLayer(0.5f);
            p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 16, 16);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip();
            p.PopOpacityLayer();
        });
        const auto image = Read(*painter);
        LogPixel(L"center", 32, 32, image.At(32, 32));
        LogPixel(L"corner", 9, 9, image.At(9, 9));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(32, 32), 128, 128, 128, 255, 3), L"black at 50% group opacity over white");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(9, 9), 255, 255, 255, 255, 1), L"outside the rounded corner stays white");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(4, 32), 255, 255, 255, 255, 1), L"outside the clip stays white");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 32), 128, 128, 128, 255, 3), L"straight edge on a pixel boundary is crisp");

        // Along the corner arc (center (24,24), radius 16) coverage is partial: an antialiased, not aliased, clip.
        uint32_t partial = 0;
        for (uint32_t y = 8; y < 24; ++y)
        {
            for (uint32_t x = 8; x < 24; ++x)
            {
                const uint8_t g = image.At(x, y).g;
                partial += (g > 132 && g < 250) ? 1 : 0;
            }
        }
        Log::Comment(String().Format(L"partially covered pixels in the top-left corner: %u", partial));
        VERIFY_IS_TRUE(partial >= 8, L"rounded clip edges are antialiased");
    }

    void GdiPlusPainterTests::RoundedClipSharesStackWithClipsAndLayers()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 32, 64 });
            p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 4, 4);
            p.PopAxisAlignedClip(); // does not match the top entry: ignored
            p.PushOpacityLayer(1.0f);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip(); // does not match the top entry: ignored
            p.PopOpacityLayer();
            p.PopRoundedRectangleClip();
            p.FillRectangle({ 40, 0, 48, 4 }, c_black); // only the axis-aligned clip [0,32) remains: clipped away
            p.PopAxisAlignedClip();
            p.FillRectangle({ 56, 60, 64, 64 }, c_black); // no clip left
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(20, 32), 0, 0, 0, 255, 1), L"inside both clips");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(40, 32), 255, 255, 255, 255, 1), L"axis-aligned clip still applied inside the rounded clip");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(4, 32), 255, 255, 255, 255, 1), L"rounded clip applied");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(44, 2), 255, 255, 255, 255, 1), L"axis-aligned clip outlives the rounded clip");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(60, 62), 0, 0, 0, 255, 1), L"stack fully unwound");

        // An unbalanced frame is unwound by EndFrame in stack order.
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 8, 8);
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
        });
        const auto unwound = Read(*painter);
        VERIFY_IS_TRUE(IsPremultiplied(unwound.At(32, 32), 128, 128, 128, 255, 3), L"open layer and clip are composited at EndFrame");
        VERIFY_IS_TRUE(IsPremultiplied(unwound.At(2, 2), 255, 255, 255, 255, 1));
    }

    void GdiPlusPainterTests::SquarePixelAlignedRoundedClipNeedsNoLayer()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        const uint64_t targetBytes = static_cast<uint64_t>(c_size) * c_size * 4;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushRoundedRectangleClip({ 8, 8, 24, 24 }, 0, 0);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip();
        });
        VERIFY_ARE_EQUAL(targetBytes, painter->GetResourceInventory().surfaceBytes, L"no layer or mask surface allocated");
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 8), 0, 0, 0, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(24, 24), 255, 255, 255, 255, 1));

        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushRoundedRectangleClip({ 8, 8, 24, 24 }, 4, 4);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip();
        });
        Log::Comment(String().Format(L"surface bytes with a rounded clip: %llu", painter->GetResourceInventory().surfaceBytes));
        VERIFY_ARE_EQUAL(2 * targetBytes, painter->GetResourceInventory().surfaceBytes, L"target + one pooled layer; axis-aligned coverage is analytic");

        // Rotated: the clip shape is rasterized by GDI+ into a mask surface.
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ 0.70710678f, 0.70710678f, -0.70710678f, 0.70710678f, 32, 4 });
            p.PushRoundedRectangleClip({ 8, 8, 32, 32 }, 6, 6);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip();
        });
        VERIFY_ARE_EQUAL(3 * targetBytes, painter->GetResourceInventory().surfaceBytes, L"target + one pooled layer + the mask");
        const auto rotated = Read(*painter);
        // (20, 20) maps to (32, 32.28); the rect's corner (8, 8) maps to (32, 15.3) and the arc pulls it in to ~(32, 17.8).
        VERIFY_IS_TRUE(IsPremultiplied(rotated.At(32, 32), 0, 0, 0, 255, 1), L"rotated clip center is drawn");
        LogPixel(L"rotated corner", 32, 16, rotated.At(32, 16));
        VERIFY_IS_TRUE(IsPremultiplied(rotated.At(32, 16), 255, 255, 255, 255, 8), L"rotated rounded corner is clipped");
        VERIFY_IS_TRUE(IsPremultiplied(rotated.At(4, 4), 255, 255, 255, 255, 1));
    }

    void GdiPlusPainterTests::PathCacheIsSteadyAndBounded()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        const auto paintTriangles = [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillPath(Triangle(1), c_black);
            p.StrokePath(Triangle(1), 2, { 1, 0, 0, 1 });
            p.FillPath(Triangle(0), { 0, 0, 1, 0.5f }); // uncached: reuses the scratch path
        };
        Paint(*painter, paintTriangles);
        const auto first = LastChurn(*painter);
        const auto inventory = painter->GetResourceInventory();
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(image.At(16, 16).r < 200, L"triangle interior painted");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(48, 48), 255, 255, 255, 255, 1), L"outside the hypotenuse untouched");

        const auto stats = Paint(*painter, paintTriangles);
        const auto second = LastChurn(*painter);
        VERIFY_ARE_EQUAL(3u, stats.primitiveCount);
        Log::Comment(String().Format(L"churn: first %u/%u, second %u/%u", first.created, first.destroyed, second.created, second.destroyed));
        VERIFY_IS_TRUE(first.created > 0);
        VERIFY_ARE_EQUAL(0u, second.created, L"cached and scratch paths are reused");
        VERIFY_ARE_EQUAL(0u, second.destroyed);
        VERIFY_ARE_EQUAL(inventory.cachedObjectCount, painter->GetResourceInventory().cachedObjectCount);
        VERIFY_ARE_EQUAL(inventory.cacheBytes, painter->GetResourceInventory().cacheBytes);

        // Many distinct keys: the LRU stays bounded and the evictions are reported.
        Paint(*painter, [](IImmediatePainter& p)
        {
            for (uint64_t key = 100; key < 1100; ++key)
            {
                p.FillPath(Triangle(key), c_black);
            }
        });
        const auto churn = LastChurn(*painter);
        const auto bounded = painter->GetResourceInventory();
        Log::Comment(String().Format(L"1000 keys: churn %u/%u, cached objects %u -> %u, bytes %llu -> %llu", churn.created, churn.destroyed,
            inventory.cachedObjectCount, bounded.cachedObjectCount, inventory.cacheBytes, bounded.cacheBytes));
        VERIFY_ARE_EQUAL(1000u, churn.created);
        VERIFY_IS_TRUE(churn.destroyed >= 1000 - 256, L"evictions are counted");
        VERIFY_IS_TRUE(bounded.cachedObjectCount <= inventory.cachedObjectCount + 256, L"path cache is bounded");
    }

    void GdiPlusPainterTests::GlyphRunFontCacheIsSteadyAndBounded()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        const auto glyphs = MakeGlyphs(L"Hgw", 24);
        const auto run = MakeRun(glyphs, 24, { 4, 40 });
        const auto paintRun = [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(run, c_black);
        };
        Paint(*painter, paintRun);
        const auto first = LastChurn(*painter);
        const auto inventory = painter->GetResourceInventory();
        const auto ink = FindInk(Read(*painter));
        Log::Comment(String().Format(L"ink %u pixels in [%u,%u)x[%u,%u)", ink.count, ink.left, ink.right, ink.top, ink.bottom));
        VERIFY_IS_TRUE(ink.count > 40, L"glyphs drawn");
        VERIFY_IS_TRUE(ink.left >= 4 && ink.left <= 8, L"first glyph starts at the origin");
        VERIFY_IS_TRUE(ink.top >= 40 - 24 && ink.top <= 40 - 14, L"cap height of a 24px 'H' above the baseline (em size honored)");
        VERIFY_IS_TRUE(ink.bottom > 40 && ink.bottom <= 40 + 8, L"descender of 'g' below the baseline");

        Paint(*painter, paintRun);
        const auto second = LastChurn(*painter);
        Log::Comment(String().Format(L"churn: first %u/%u, second %u/%u", first.created, first.destroyed, second.created, second.destroyed));
        VERIFY_IS_TRUE(first.created >= 2, L"font face mapping and font are created once");
        VERIFY_ARE_EQUAL(0u, second.created);
        VERIFY_ARE_EQUAL(0u, second.destroyed);
        VERIFY_ARE_EQUAL(inventory.cachedObjectCount, painter->GetResourceInventory().cachedObjectCount);

        // An animated font size: one font per size, bounded LRU, evictions reported.
        Paint(*painter, [&](IImmediatePainter& p)
        {
            for (uint32_t size = 0; size < 200; ++size)
            {
                p.DrawGlyphRun(MakeRun(glyphs, 8 + size * 0.25f, { 4, 40 }), c_black);
            }
        });
        const auto churn = LastChurn(*painter);
        const auto bounded = painter->GetResourceInventory();
        Log::Comment(String().Format(L"200 sizes: churn %u/%u, cached objects %u -> %u", churn.created, churn.destroyed,
            inventory.cachedObjectCount, bounded.cachedObjectCount));
        VERIFY_IS_TRUE(churn.created >= 199);
        VERIFY_IS_TRUE(churn.destroyed >= 200 - 64, L"font evictions are counted");
        VERIFY_IS_TRUE(bounded.cachedObjectCount <= inventory.cachedObjectCount + 64, L"glyph font cache is bounded");
    }

    void GdiPlusPainterTests::GlyphRunRightToLeftAdvancesLeftwards()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        const auto glyphs = MakeGlyphs(L"HHH", 16);
        float width = 0;
        for (float advance : glyphs.advances) width += advance;

        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(MakeRun(glyphs, 16, { 60, 30 }, 1), c_black);
        });
        const auto ink = FindInk(Read(*painter));
        Log::Comment(String().Format(L"RTL ink [%u,%u), run width %.1f", ink.left, ink.right, width));
        VERIFY_IS_TRUE(ink.count > 0);
        VERIFY_IS_TRUE(ink.right <= 60, L"an RTL run extends left of its origin");
        VERIFY_IS_TRUE(ink.left >= static_cast<uint32_t>(60 - width - 2), L"by about its total advance");

        // Offsets move a glyph in the run's direction: positive advanceOffset moves an RTL glyph left.
        const GlyphOffset offsets[] = { { 3, 0 }, { 3, 0 }, { 3, 0 } };
        auto run = MakeRun(glyphs, 16, { 60, 30 }, 1);
        run.glyphOffsets = offsets;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(run, c_black);
        });
        const auto shifted = FindInk(Read(*painter));
        VERIFY_ARE_EQUAL(ink.left - 3, shifted.left);
        VERIFY_ARE_EQUAL(ink.right - 3, shifted.right);
    }

    void GdiPlusPainterTests::BitmapCacheCountsBytesAndUploads()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        const auto before = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(0ull, before.uploadBytesThisFrame);

        const auto red = SolidPixels(16, 8, 0, 0, 255);
        BitmapSource source{ 7, 1, 16, 8, 16 * 4, red.data() };
        VERIFY_SUCCEEDED(painter->EnsureBitmap(source));
        Paint(*painter, [](IImmediatePainter& p) { p.DrawBitmap(7, { 0, 0, 16, 8 }, nullptr, 1); });
        auto inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(16ull * 8 * 4, inventory.uploadBytesThisFrame, L"copy made outside a frame counts in the next frame");
        Log::Comment(String().Format(L"cache bytes %llu -> %llu, objects %u -> %u", before.cacheBytes, inventory.cacheBytes,
            before.cachedObjectCount, inventory.cachedObjectCount));
        VERIFY_IS_TRUE(inventory.cacheBytes >= before.cacheBytes + 16 * 8 * 4, L"bitmap storage counts in cacheBytes");

        // Same (key, generation): no copy.
        VERIFY_SUCCEEDED(painter->EnsureBitmap(source));
        Paint(*painter, [](IImmediatePainter& p) { p.DrawBitmap(7, { 0, 0, 16, 8 }, nullptr, 1); });
        VERIFY_ARE_EQUAL(0ull, painter->GetResourceInventory().uploadBytesThisFrame);
        VERIFY_ARE_EQUAL(0u, LastChurn(*painter).created);

        // New generation inside a frame, same size: refreshed in place (copied, not recreated).
        const auto blue = SolidPixels(16, 8, 255, 0, 0);
        BitmapSource refreshed{ 7, 2, 16, 8, 16 * 4, blue.data() };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            VERIFY_SUCCEEDED(p.EnsureBitmap(refreshed));
            p.DrawBitmap(7, { 0, 0, 16, 8 }, nullptr, 1);
        });
        VERIFY_ARE_EQUAL(16ull * 8 * 4, painter->GetResourceInventory().uploadBytesThisFrame);
        VERIFY_ARE_EQUAL(0u, LastChurn(*painter).created);
        VERIFY_IS_TRUE(IsPremultiplied(Read(*painter).At(8, 4), 255, 0, 0, 255, 1), L"refreshed pixels are drawn");

        const auto loaded = painter->GetResourceInventory();
        painter->ReleaseBitmap(7);
        inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(16ull * 8 * 4, loaded.cacheBytes - inventory.cacheBytes, L"released bitmap storage leaves the cache");
        VERIFY_ARE_EQUAL(loaded.cachedObjectCount - 1, inventory.cachedObjectCount);
        const auto stats = Paint(*painter, [](IImmediatePainter& p) { p.DrawBitmap(7, { 0, 0, 16, 8 }, nullptr, 1); });
        VERIFY_ARE_EQUAL(0u, stats.primitiveCount, L"unknown key is a no-op");
    }

    void GdiPlusPainterTests::BitmapDrawsOneToOneWithClampedEdges()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        // Left half red, right half green, so the 1:1 copy and the source rect are both observable.
        auto pixels = SolidPixels(16, 16, 0, 0, 255);
        for (uint32_t y = 0; y < 16; ++y)
        {
            for (uint32_t x = 8; x < 16; ++x)
            {
                uint8_t* p = &pixels[(y * 16 + x) * 4];
                p[0] = 0; p[1] = 255; p[2] = 0;
            }
        }
        VERIFY_SUCCEEDED(painter->EnsureBitmap({ 1, 1, 16, 16, 16 * 4, pixels.data() }));
        const RectF greenHalf{ 8, 0, 16, 16 };
        const auto stats = Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 1);
            p.DrawBitmap(1, { 32, 8, 56, 32 }, &greenHalf, 1); // 3x upscale of the green half
            p.DrawBitmap(1, { 8, 40, 24, 56 }, nullptr, 0.5f);
        });
        VERIFY_ARE_EQUAL(3u, stats.primitiveCount);
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(8, 8), 0, 0, 255, 255, 1), L"1:1 corner pixel exact (no fade at the edge)");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(15, 23), 0, 0, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(16, 8), 0, 255, 0, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(23, 23), 0, 255, 0, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(7, 8), 255, 255, 255, 255, 1), L"nothing outside the destination");
        LogPixel(L"upscaled edge", 32, 8, image.At(32, 8));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(32, 8), 0, 255, 0, 255, 2), L"source rect edges clamp: no red bleed, no transparent fade");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(55, 31), 0, 255, 0, 255, 2));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(10, 44), 127, 127, 255, 255, 3), L"50% red over white");
    }

    void GdiPlusPainterTests::ScrollPixelsMovesRowsWithinRect()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        Paint(*painter, PaintRowRamp);
        const auto before = Read(*painter);

        VERIFY_ARE_EQUAL(S_OK, painter->ScrollPixels({ 0, 0, 32, 64 }, 0, -8)); // left half scrolls up by 8
        VERIFY_ARE_EQUAL(S_OK, painter->ScrollPixels({ 32, 0, 64, 64 }, 0, 8)); // right half scrolls down by 8
        auto image = Read(*painter);
        for (uint32_t y = 0; y < 56; ++y)
        {
            VERIFY_ARE_EQUAL(before.At(0, y + 8).r, image.At(0, y).r);
            VERIFY_ARE_EQUAL(before.At(31, y + 8).r, image.At(31, y).r);
            VERIFY_ARE_EQUAL(before.At(32, y).r, image.At(32, y + 8).r, L"downward moves copy bottom-up (memmove-safe overlap)");
        }
        VERIFY_ARE_EQUAL(before.At(0, 60).r, image.At(0, 60).r, L"the exposed strip keeps stale pixels until repainted");
        VERIFY_ARE_EQUAL(before.At(40, 2).r, image.At(40, 2).r);

        // Horizontal, clipped to the rect and the target.
        VERIFY_ARE_EQUAL(S_OK, painter->ScrollPixels({ 8, 8, 100, 16 }, 4, 0));
        const auto shifted = Read(*painter);
        VERIFY_IS_TRUE(memcmp(&shifted.bytes[(10 * 64 + 12) * 4], &image.bytes[(10 * 64 + 8) * 4], (64 - 12) * 4) == 0);
        VERIFY_IS_TRUE(memcmp(&shifted.bytes[(16 * 64) * 4], &image.bytes[(16 * 64) * 4], 64 * 4) == 0, L"rows outside the rect untouched");

        VERIFY_ARE_EQUAL(S_OK, painter->ScrollPixels({ 0, 0, 64, 64 }, 0, 100), L"moving everything out of the rect is a no-op");
        VERIFY_SUCCEEDED(painter->BeginFrame(nullptr, 0));
        VERIFY_ARE_EQUAL(E_ILLEGAL_METHOD_CALL, painter->ScrollPixels({ 0, 0, 64, 64 }, 0, 1));
        FrameStats stats{};
        VERIFY_SUCCEEDED(painter->EndFrame(&stats));
    }

    void GdiPlusPainterTests::ScrollPixelsPresentsMovedArea()
    {
        auto painter = CreateGdiPlus();
        if (!painter) return;
        auto device = CreateWarpDevice();
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        Paint(*painter, PaintRowRamp);
        Paint(*painter, PaintRowRamp); // both flip buffers hold the ramp

        // Scroll the left half up by 8 and repaint only the exposed strip.
        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 32, 64 }, 0, -8));
        const RectF strip{ 0, 56, 32, 64 };
        const auto stats = Paint(*painter, [](IImmediatePainter& p) { p.FillRectangle({ 0, 0, 64, 64 }, c_black); }, &strip, 1);
        VERIFY_ARE_EQUAL(32ull * 8, stats.damagedPixels, L"the moved area is not damage");
        VERIFY_ARE_EQUAL(32ull * 64 * 4, painter->GetResourceInventory().uploadBytesThisFrame,
            L"presented = exposed strip + moved area, without double counting");

        // The scroll is consumed by that frame. Frame 4 (into B) is tiny, so buffer 0 is A again and holds frame 3.
        const RectF corner{ 0, 0, 4, 4 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); }, &corner, 1);
        VERIFY_ARE_EQUAL(16ull * 4, painter->GetResourceInventory().uploadBytesThisFrame);

        const auto target = Read(*painter);
        const auto presented = ReadBackBuffer(device.Get(), swapChain.Get());
        for (uint32_t y = 0; y < c_size; y += 7)
        {
            VERIFY_ARE_EQUAL(target.At(4, y).r, presented.At(4, y).r, L"moved and repainted pixels reach the back buffer");
        }
        VERIFY_IS_TRUE(IsPremultiplied(presented.At(4, 60), 0, 0, 0, 255, 1));
    }

    // ---- gdilayers ----

    namespace
    {
        constexpr uint32_t c_largeWidth = 1024;
        constexpr uint32_t c_largeHeight = 768;
        constexpr uint64_t c_largeTargetBytes = static_cast<uint64_t>(c_largeWidth) * c_largeHeight * 4;

        uint32_t MaxChannelDifference(const Image& a, const Image& b, uint32_t& atX, uint32_t& atY)
        {
            uint32_t worst = 0;
            for (uint32_t y = 0; y < a.height; ++y)
            {
                for (uint32_t x = 0; x < a.width; ++x)
                {
                    const size_t i = (static_cast<size_t>(y) * a.width + x) * 4;
                    for (int c = 0; c < 4; ++c)
                    {
                        const uint32_t d = static_cast<uint32_t>(std::abs(static_cast<int>(a.bytes[i + c]) - static_cast<int>(b.bytes[i + c])));
                        if (d > worst)
                        {
                            worst = d;
                            atX = x;
                            atY = y;
                        }
                    }
                }
            }
            return worst;
        }
    }

    void GdiPlusPainterTests::LayersAreSizedToTheirBounds()
    {
        auto painter = CreateGdiPlus(c_largeWidth, c_largeHeight);
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 600, 500, 700, 560 });
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 0, 0, 1024, 768 }, c_black);
            p.PopOpacityLayer();
            p.PopAxisAlignedClip();
            p.PushRoundedRectangleClip({ 100, 100, 180, 150 }, 8, 8);
            p.FillRectangle({ 0, 0, 1024, 768 }, c_black);
            p.PopRoundedRectangleClip();
        });
        auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"surface %llu (target %llu), layers %llu", inventory.surfaceBytes, c_largeTargetBytes, inventory.layerBytes));
        VERIFY_IS_TRUE(inventory.surfaceBytes > c_largeTargetBytes, L"a pooled layer is held");
        VERIFY_ARE_EQUAL(inventory.surfaceBytes - c_largeTargetBytes, inventory.layerBytes, L"layer bytes are the surface bytes beyond the target");
        VERIFY_IS_TRUE(inventory.layerBytes <= 128ull * 64 * 4, L"one layer sized to the larger bounds (100x60, 64 px buckets), not to the window");
        auto image = Read(*painter, c_largeWidth, c_largeHeight);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(650, 530), 128, 128, 128, 255, 3), L"50% black composited at the layer's offset");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(600, 500), 128, 128, 128, 255, 3));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(699, 559), 128, 128, 128, 255, 3));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(599, 530), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(700, 559), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(140, 125), 0, 0, 0, 255, 1), L"rounded clip content at its offset");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(100, 125), 0, 0, 0, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(101, 101), 255, 255, 255, 255, 1), L"outside the rounded corner");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(99, 125), 255, 255, 255, 255, 1));

        // Rotated: the clip shape is rasterized into a mask surface, sized like the layer.
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ 0.70710678f, 0.70710678f, -0.70710678f, 0.70710678f, 500, 300 });
            p.PushRoundedRectangleClip({ 0, 0, 60, 60 }, 8, 8);
            p.FillRectangle({ -100, -100, 200, 200 }, c_black);
            p.PopRoundedRectangleClip();
        });
        inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"rotated: surface %llu, layers %llu", inventory.surfaceBytes, inventory.layerBytes));
        VERIFY_IS_TRUE(inventory.layerBytes <= 2ull * 128 * 128 * 4, L"layer + mask sized to the ~86x86 clip bounds");
        image = Read(*painter, c_largeWidth, c_largeHeight);
        // (30, 30) maps to (500, 342.4); (0, 0) maps to (500, 300) and the arc pulls the corner in to ~(500, 303.3).
        VERIFY_IS_TRUE(IsPremultiplied(image.At(500, 342), 0, 0, 0, 255, 1), L"rotated clip center");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(500, 370), 0, 0, 0, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(500, 301), 255, 255, 255, 255, 8), L"rotated rounded corner is clipped");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(460, 300), 255, 255, 255, 255, 1));
    }

    void GdiPlusPainterTests::OpacityBoundsHintLimitsLayer()
    {
        auto painter = CreateGdiPlus(c_largeWidth, c_largeHeight);
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushOpacityLayerWithBounds(0.5f, { 10, 10, 50, 40 });
            p.FillRectangle({ 10, 10, 50, 40 }, c_black);
            p.PopOpacityLayer();
            p.PushOpacityLayerWithBounds(0.5f, { 900, 700, 1000, 760 });
            p.PushRoundedRectangleClip({ 910, 710, 990, 750 }, 6, 6);
            p.FillRectangle({ 900, 700, 1000, 760 }, c_black);
            p.PopRoundedRectangleClip();
            p.FillRectangle({ 990, 750, 1000, 760 }, c_black);
            p.PopOpacityLayer();
        });
        const auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"full damage, hinted groups: layers %llu", inventory.layerBytes));
        VERIFY_IS_TRUE(inventory.layerBytes > 0);
        VERIFY_IS_TRUE(inventory.layerBytes <= 2ull * 128 * 64 * 4, L"two nested layers sized to the hints, not to the full damage");
        const auto image = Read(*painter, c_largeWidth, c_largeHeight);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(10, 10), 128, 128, 128, 255, 3));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(49, 39), 128, 128, 128, 255, 3));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(50, 25), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(950, 730), 128, 128, 128, 255, 3), L"nested rounded clip inside the hinted group");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(995, 755), 128, 128, 128, 255, 3), L"drawn in the group outside the nested clip");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(910, 710), 255, 255, 255, 255, 1), L"outside the nested rounded corner");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(913, 713), 128, 128, 128, 255, 3), L"inside the nested rounded corner's arc");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(905, 705), 255, 255, 255, 255, 1));

        // Partial damage further bounds the layer.
        const RectF damage{ 0, 0, 30, 30 };
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushOpacityLayerWithBounds(0.25f, { 10, 10, 50, 40 });
            p.FillRectangle({ 10, 10, 50, 40 }, c_black);
            p.PopOpacityLayer();
        }, &damage, 1);
        const auto partial = Read(*painter, c_largeWidth, c_largeHeight);
        VERIFY_IS_TRUE(IsPremultiplied(partial.At(20, 20), 191, 191, 191, 255, 3), L"repainted at 25%");
        VERIFY_IS_TRUE(IsPremultiplied(partial.At(40, 35), 128, 128, 128, 255, 3), L"outside the damage keeps the 50% frame");
    }

    void GdiPlusPainterTests::NestedLayersAtOffsetsComposite()
    {
        constexpr uint32_t width = 256, height = 192;
        auto painter = CreateGdiPlus(width, height);
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 30, 20, 200, 150 });
            p.PushOpacityLayer(0.5f);
            p.PushRoundedRectangleClip({ 50, 40, 180, 140 }, 12, 12);
            p.FillRectangle({ 60, 50, 120, 100 }, c_black);
            p.PushAxisAlignedClip({ 130, 60, 170, 130 });
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 0, 0, 256, 192 }, { 1, 0, 0, 1 });
            p.PopOpacityLayer();
            p.PopAxisAlignedClip();
            p.PopRoundedRectangleClip();
            p.FillRectangle({ 20, 140, 60, 160 }, c_black); // in the outer layer, cut by the axis-aligned clip at y = 150
            p.PopOpacityLayer();
            p.PopAxisAlignedClip();

            // A layer that runs past the target's bottom-right edge, under a fractional translation.
            p.SetTransform({ 1, 0, 0, 1, 0.5f, 0.25f });
            p.PushRoundedRectangleClip({ 210, 160, 300, 250 }, 10, 10);
            p.FillRectangle({ 0, 0, 400, 400 }, c_black);
            p.PopRoundedRectangleClip();
        });
        const auto image = Read(*painter, width, height);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(90, 75), 128, 128, 128, 255, 3), L"black at 50% in the nested rounded clip");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(150, 100), 191, 191, 255, 255, 3), L"red at 25% in the doubly nested layer");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(129, 100), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(150, 131), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(51, 41), 255, 255, 255, 255, 1), L"outside the rounded corner");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(45, 145), 128, 128, 128, 255, 3), L"outer layer content");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(45, 150), 255, 255, 255, 255, 1), L"cut by the outer clip");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(25, 145), 255, 255, 255, 255, 1));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(240, 180), 0, 0, 0, 255, 1), L"edge layer content");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(255, 191), 0, 0, 0, 255, 1), L"edge layer reaches the target corner");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(211, 161), 255, 255, 255, 255, 1), L"edge layer's rounded corner");
        VERIFY_IS_TRUE(IsPremultiplied(image.At(209, 180), 255, 255, 255, 255, 1));
    }

    void GdiPlusPainterTests::LayerContentMatchesDirectDrawing()
    {
        // The same primitives drawn into the target and into (nested) layers at an offset composite to the same pixels:
        // text, exact and brush (mask-and-shade) gradients, a shadow, a bitmap, backdrop blur, invert, and a rotated clip.
        constexpr uint32_t width = 256, height = 192;
        const RectF region{ 37, 23, 229, 171 };
        const auto bitmap = SolidPixels(16, 16, 0, 160, 255);
        static const GradientStop stops[] = { { 0, { 1, 0, 0, 1 } }, { 1, { 0, 0, 1, 1 } } };
        const auto scene = [&](IImmediatePainter& p)
        {
            p.FillRectangle(region, c_white);
            TextStyle style{};
            style.fontSize = 14;
            p.DrawText(L"Layers", 6, style, { 45, 28, 140, 50 }, c_black);
            p.FillRoundedRectangleGradient({ 50, 55, 150, 85 }, 8, 8, LinearGradientDesc{ { 50, 60 }, { 150, 60 }, stops, 2, 1 });
            BrushDesc radial{};
            radial.kind = BrushKind::Radial;
            radial.stops = stops;
            radial.stopCount = 2;
            radial.center = { 190, 70 };
            radial.origin = { 190, 70 };
            radial.radiusX = radial.radiusY = 25;
            p.FillRoundedRectangleBrush({ 165, 45, 215, 95 }, 25, 25, radial);
            p.DrawRoundedRectangleShadow({ 60, 100, 110, 140 }, 6, 6, 8, { 3, 4 }, { 0, 0, 0, 0.5f });
            p.FillRoundedRectangle({ 60, 100, 110, 140 }, 6, 6, { 0.2f, 0.6f, 0.3f, 1 });
            p.DrawBitmap(1, { 130, 100, 162, 132 }, nullptr, 1);
            p.BlurBackdrop({ 125, 95, 170, 140 }, 4, 4, 6);
            p.InvertRectangle({ 180, 110, 200, 150 }, c_white);
            p.SetTransform({ 0.8660254f, 0.5f, -0.5f, 0.8660254f, 200, 120 });
            p.PushRoundedRectangleClip({ 0, 0, 30, 20 }, 5, 5);
            p.FillRectangle({ -50, -50, 100, 100 }, { 0.5f, 0.1f, 0.7f, 1 });
            p.PopRoundedRectangleClip();
            p.SetTransform({});
        };
        const auto render = [&](auto&& wrap)
        {
            auto painter = CreateGdiPlus(width, height);
            VERIFY_SUCCEEDED(painter->EnsureBitmap({ 1, 1, 16, 16, 16 * 4, bitmap.data() }));
            Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear({ 0.9f, 0.9f, 0.9f, 1 });
                wrap(p);
            });
            return Read(*painter, width, height);
        };
        const auto direct = render([&](IImmediatePainter& p)
        {
            p.PushAxisAlignedClip(region);
            scene(p);
            p.PopAxisAlignedClip();
        });
        const auto layered = render([&](IImmediatePainter& p)
        {
            p.PushAxisAlignedClip(region);
            p.PushOpacityLayer(1);
            scene(p);
            p.PopOpacityLayer();
            p.PopAxisAlignedClip();
        });
        const auto nested = render([&](IImmediatePainter& p)
        {
            p.PushRoundedRectangleClip({ 30, 20, 236, 178 }, 2, 2);
            p.PushOpacityLayerWithBounds(1, { 37, 23, 229, 171 });
            p.PushAxisAlignedClip(region);
            scene(p);
            p.PopAxisAlignedClip();
            p.PopOpacityLayer();
            p.PopRoundedRectangleClip();
        });
        uint32_t x = 0, y = 0;
        uint32_t difference = MaxChannelDifference(direct, layered, x, y);
        Log::Comment(String().Format(L"direct vs one layer: max channel difference %u at (%u, %u)", difference, x, y));
        VERIFY_IS_TRUE(difference <= 2);
        difference = MaxChannelDifference(direct, nested, x, y);
        Log::Comment(String().Format(L"direct vs nested layers: max channel difference %u at (%u, %u)", difference, x, y));
        VERIFY_IS_TRUE(difference <= 2);
        VERIFY_IS_FALSE(IsPremultiplied(direct.At(70, 70), 255, 255, 255, 255, 8), L"the scene drew something");
    }

    void GdiPlusPainterTests::PooledLayersAreReusedAndGrow()
    {
        auto painter = CreateGdiPlus(c_largeWidth, c_largeHeight);
        if (!painter) return;
        const auto layered = [](float left, float top, float size)
        {
            return [=](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.PushAxisAlignedClip({ left, top, left + size, top + size });
                p.PushOpacityLayer(0.5f);
                p.FillRectangle({ 0, 0, 1024, 768 }, c_black);
                p.PopOpacityLayer();
                p.PopAxisAlignedClip();
            };
        };
        Paint(*painter, layered(10, 10, 40));
        const uint64_t smallBytes = painter->GetResourceInventory().layerBytes;
        Paint(*painter, layered(500, 300, 50));
        VERIFY_ARE_EQUAL(0u, LastChurn(*painter).created, L"a layer that fits the pooled capacity is reused at another offset");
        VERIFY_ARE_EQUAL(smallBytes, painter->GetResourceInventory().layerBytes);
        Paint(*painter, layered(100, 100, 300));
        const uint64_t largeBytes = painter->GetResourceInventory().layerBytes;
        Log::Comment(String().Format(L"layer bytes: small %llu, large %llu", smallBytes, largeBytes));
        VERIFY_IS_TRUE(LastChurn(*painter).created > 0, L"a larger layer grows the pooled surface");
        VERIFY_IS_TRUE(largeBytes > smallBytes && largeBytes <= 320ull * 320 * 4);
        Paint(*painter, layered(700, 400, 40));
        VERIFY_ARE_EQUAL(0u, LastChurn(*painter).created, L"the grown layer is reused for smaller bounds");
        const auto image = Read(*painter, c_largeWidth, c_largeHeight);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(720, 420), 128, 128, 128, 255, 3));
        VERIFY_IS_TRUE(IsPremultiplied(image.At(741, 420), 255, 255, 255, 255, 1));
    }

    void GdiPlusPainterTests::IdleTrimReleasesLayersAndScratch()
    {
        auto painter = CreateGdiPlus(c_largeWidth, c_largeHeight);
        if (!painter) return;
        const auto layered = [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 10, 10, 50, 50 }, c_black);
            p.PopOpacityLayer();
            p.DrawRoundedRectangleShadow({ 100, 100, 600, 500 }, 8, 8, 16, { 0, 4 }, { 0, 0, 0, 0.4f });
        };
        Paint(*painter, layered);
        const auto held = painter->GetResourceInventory();
        Log::Comment(String().Format(L"held: surface %llu, layers %llu, cache %llu", held.surfaceBytes, held.layerBytes, held.cacheBytes));
        VERIFY_IS_TRUE(held.surfaceBytes > c_largeTargetBytes);

        const uint64_t now = GetTickCount64();
        const uint32_t dueIn = painter->TrimTransientResources(now);
        Log::Comment(String().Format(L"trim right after use: next due in %u ms", dueIn));
        VERIFY_IS_TRUE(dueIn > 0 && dueIn <= 10000, L"recently used surfaces are kept and a later trim is due");
        VERIFY_ARE_EQUAL(held.surfaceBytes, painter->GetResourceInventory().surfaceBytes);

        VERIFY_ARE_EQUAL(0u, painter->TrimTransientResources(now + 60000), L"nothing is left to trim");
        const auto trimmed = painter->GetResourceInventory();
        Log::Comment(String().Format(L"trimmed: surface %llu, layers %llu, cache %llu", trimmed.surfaceBytes, trimmed.layerBytes, trimmed.cacheBytes));
        VERIFY_ARE_EQUAL(c_largeTargetBytes, trimmed.surfaceBytes, L"idle layers are released");
        VERIFY_ARE_EQUAL(0ull, trimmed.layerBytes);
        VERIFY_IS_TRUE(trimmed.cacheBytes < held.cacheBytes, L"idle scratch is released");

        Paint(*painter, layered);
        const auto image = Read(*painter, c_largeWidth, c_largeHeight);
        VERIFY_IS_TRUE(IsPremultiplied(image.At(30, 30), 128, 128, 128, 255, 3), L"layers are recreated on demand");
        VERIFY_IS_TRUE(painter->GetResourceInventory().surfaceBytes > c_largeTargetBytes);
    }

    // ---- ResourceChurn event ----

    void ResourceChurnEventTests::TraceResourceChurnIsCallableWithoutListener()
    {
        Log::Comment(String().Format(L"Resources keyword enabled by a session: %d", IsTracingEnabled(Keywords::Resources) ? 1 : 0));
        TraceResourceChurn(Mode::ImmediateGdiPlus, 1, 12, 0);
        TraceResourceChurn(Mode::ImmediateD2DSoftware, 2, 0, 3);
        TraceResourceChurn(Mode::Composition, UINT32_MAX, UINT32_MAX, UINT32_MAX);
        TraceResourceChurn(static_cast<Mode>(UINT32_MAX), 0, 0, 0);
        VERIFY_IS_TRUE((Keywords::Resources & (Keywords::Selection | Keywords::Frame | Keywords::Paint)) == 0,
            L"ResourceChurn uses its own keyword bit so it can be enabled without per-frame paint events");
    }
}
