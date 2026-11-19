// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GdiPlusPainterTests.h"
#include <ImmediatePainter.h>
#include <GdiPlusPainter.h>
#include <RenderingBackend.h>
#include <cmath>
#include <vector>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using namespace WEX::TestExecution;

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
        auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        auto inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(c_size) * c_size * 4, inventory.uploadBytesThisFrame, L"full frame copies the whole bitmap");
        VERIFY_ARE_EQUAL(stats.damagedPixels * 4, inventory.uploadBytesThisFrame);

        const RectF damage{ 8, 8, 40, 24 };
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
