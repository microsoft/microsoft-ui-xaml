// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediatePainterTests.h"
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <dxgi.h>
#include <wrl/client.h>
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
        constexpr ColorF c_red{ 1, 0, 0, 1 };
        constexpr ColorF c_blue{ 0, 0, 1, 1 };

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

        bool Near(uint8_t actual, int expected, int tolerance)
        {
            return std::abs(static_cast<int>(actual) - expected) <= tolerance;
        }

        bool IsColor(const Pixel& p, const ColorF& c, int tolerance = 3)
        {
            return Near(p.r, static_cast<int>(std::lround(c.r * c.a * 255)), tolerance) &&
                Near(p.g, static_cast<int>(std::lround(c.g * c.a * 255)), tolerance) &&
                Near(p.b, static_cast<int>(std::lround(c.b * c.a * 255)), tolerance) &&
                Near(p.a, static_cast<int>(std::lround(c.a * 255)), tolerance);
        }

        // Ink on a white background: any channel noticeably darker than white.
        bool IsInk(const Pixel& p)
        {
            return p.r < 200 || p.g < 200 || p.b < 200;
        }

        void LogPixel(const wchar_t* label, uint32_t x, uint32_t y, const Pixel& p)
        {
            Log::Comment(String().Format(L"%s (%u,%u) = B%u G%u R%u A%u", label, x, y, p.b, p.g, p.r, p.a));
        }

        Mode CurrentMode()
        {
            String text;
            VERIFY_SUCCEEDED(TestData::TryGetValue(L"Mode", text));
            Mode mode{};
            VERIFY_IS_TRUE(TryParseMode(static_cast<const wchar_t*>(text), mode));
            return mode;
        }

        // Returns nullptr after logging Skipped when the mode needs hardware this machine does not have.
        std::unique_ptr<IImmediatePainter> CreatePainter(uint32_t width = c_size, uint32_t height = c_size, float dpiScale = 1.0f)
        {
            const Mode mode = CurrentMode();
            std::unique_ptr<IImmediatePainter> painter;
            const HRESULT hr = CreateImmediatePainter(mode, PainterOptions{ width, height, dpiScale }, painter);
            if (mode == Mode::ImmediateD2DHardware && hr == DXGI_ERROR_NOT_FOUND)
            {
                Log::Result(TestResults::Skipped, L"No hardware D3D adapter on this machine; D2D-HW cannot be exercised.");
                return nullptr;
            }
            VERIFY_SUCCEEDED(hr, String().Format(L"CreateImmediatePainter(%s)", ModeName(mode)));
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
    }

    void PainterConformanceTests::CreatesPainterWithExpectedDeviceIdentity()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const auto identity = painter->GetDeviceIdentity();
        const Mode mode = CurrentMode();
        VERIFY_IS_TRUE(identity.mode == mode);
        switch (mode)
        {
            case Mode::ImmediateGdiPlus:
                VERIFY_IS_FALSE(identity.isWarp);
                VERIFY_ARE_EQUAL(0u, identity.featureLevel);
                break;
            case Mode::ImmediateD2DSoftware:
                VERIFY_IS_TRUE(identity.isWarp);
                VERIFY_IS_TRUE(identity.featureLevel != 0);
                break;
            case Mode::ImmediateD2DHardware:
                VERIFY_IS_FALSE(identity.isWarp);
                VERIFY_IS_TRUE(identity.adapterLuid != 0);
                VERIFY_IS_TRUE(identity.featureLevel != 0);
                break;
            default:
                VERIFY_FAIL(L"unexpected mode");
        }
    }

    void PainterConformanceTests::ClearFillsTarget()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(0, 0), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(c_size - 1, c_size - 1), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(c_size / 2, c_size / 2), c_red));
    }

    void PainterConformanceTests::FillRectangleCoversExpectedPixels()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 8, 8, 24, 24 }, c_red);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(16, 16), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(23, 23), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(7, 7), c_white));
        VERIFY_IS_TRUE(IsColor(image.At(24, 24), c_white));
    }

    void PainterConformanceTests::RoundedCornersAreAntialiased()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangle({ 8, 8, 56, 56 }, 12, 12, c_black);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_white), L"corner outside the arc stays background");
        VERIFY_IS_TRUE(IsColor(image.At(32, 32), c_black));
        VERIFY_IS_TRUE(IsColor(image.At(32, 8), c_black), L"straight edge is fully covered");

        // The arc crosses the diagonal near (11.5, 11.5); at least one pixel there must be partially covered.
        bool foundPartial = false;
        for (uint32_t i = 8; i < 20 && !foundPartial; ++i)
        {
            const auto p = image.At(i, i);
            LogPixel(L"diagonal", i, i, p);
            foundPartial = p.r > 24 && p.r < 231;
        }
        VERIFY_IS_TRUE(foundPartial, L"expected antialiased coverage on the rounded corner");
    }

    void PainterConformanceTests::StrokeDrawsOnlyTheEdge()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangle({ 9, 9, 55, 55 }, 0, 0, 2, c_black);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(32, 8), c_black, 8));
        VERIFY_IS_TRUE(IsColor(image.At(32, 9), c_black, 8));
        VERIFY_IS_TRUE(IsColor(image.At(8, 32), c_black, 8));
        VERIFY_IS_TRUE(IsColor(image.At(32, 32), c_white), L"interior is not filled");
        VERIFY_IS_TRUE(IsColor(image.At(32, 4), c_white), L"outside is untouched");
    }

    void PainterConformanceTests::TextProducesInkInsideLayout()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            TextStyle style{};
            style.fontSize = 20;
            p.DrawText(L"Hg", 2, style, { 8, 8, 56, 40 }, c_black);
        });
        const auto image = Read(*painter);
        uint32_t inkInside = 0;
        uint32_t inkOutside = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                if (!IsInk(image.At(x, y))) continue;
                const bool inside = x >= 8 && x < 56 && y >= 8 && y < 44;
                ++(inside ? inkInside : inkOutside);
            }
        }
        Log::Comment(String().Format(L"ink inside %u, outside %u", inkInside, inkOutside));
        VERIFY_IS_TRUE(inkInside > 20);
        VERIFY_ARE_EQUAL(0u, inkOutside);
    }

    void PainterConformanceTests::AxisAlignedClipRestrictsDrawing()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 32, 64 });
            p.FillRectangle({ 0, 0, 64, 64 }, c_blue);
            p.PopAxisAlignedClip();
            p.FillRectangle({ 48, 0, 64, 8 }, c_red);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(16, 32), c_blue));
        VERIFY_IS_TRUE(IsColor(image.At(48, 32), c_white), L"clipped out");
        VERIFY_IS_TRUE(IsColor(image.At(56, 4), c_red), L"clip popped");
    }

    void PainterConformanceTests::TransformTranslatesDrawing()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            Matrix3x2F translate{};
            translate.dx = 20;
            translate.dy = 10;
            p.SetTransform(translate);
            p.FillRectangle({ 0, 0, 10, 10 }, c_red);
            p.SetTransform({});
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(25, 15), c_red));
        VERIFY_IS_TRUE(IsColor(image.At(5, 5), c_white));
    }

    void PainterConformanceTests::OpacityLayerUsesGroupSemantics()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 8, 8, 40, 40 }, c_black);
            p.FillRectangle({ 24, 24, 56, 56 }, c_black);
            p.PopOpacityLayer();
        });
        const auto image = Read(*painter);
        const auto single = image.At(12, 12);
        const auto overlap = image.At(32, 32);
        LogPixel(L"single", 12, 12, single);
        LogPixel(L"overlap", 32, 32, overlap);
        VERIFY_IS_TRUE(Near(single.r, 128, 8));
        VERIFY_IS_TRUE(Near(overlap.r, single.r, 3), L"overlap must not be blended twice");
        VERIFY_IS_TRUE(IsColor(image.At(4, 4), c_white));
    }

    void PainterConformanceTests::DpiScaleMapsDipsToPixels()
    {
        auto painter = CreatePainter(c_size, c_size, 2.0f);
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 0, 0, 8, 8 }, c_red);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(12, 12), c_red), L"8 DIPs at 200% cover 16 pixels");
        VERIFY_IS_TRUE(IsColor(image.At(20, 20), c_white));
    }

    void PainterConformanceTests::FullFrameReportsStats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const auto stats = Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 0, 0, 10, 10 }, c_red);
            p.FillRoundedRectangle({ 10, 10, 30, 30 }, 4, 4, c_blue);
            TextStyle style{};
            p.DrawText(L"A", 1, style, { 30, 30, 60, 60 }, c_black);
        });
        VERIFY_ARE_EQUAL(3u, stats.primitiveCount);
        VERIFY_ARE_EQUAL(1u, stats.damageRectCount);
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(c_size) * c_size, stats.damagedPixels);
        VERIFY_IS_TRUE(stats.paintedPixels >= stats.damagedPixels);
    }

    void PainterConformanceTests::PartialDamagePreservesOutsidePixels()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); });

        const RectF damage{ 0, 0, 32, 64 };
        const auto stats = Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_blue);
            p.FillRectangle({ 0, 0, 64, 8 }, c_white);
        }, &damage, 1);

        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(16, 32), c_blue));
        VERIFY_IS_TRUE(IsColor(image.At(16, 4), c_white));
        VERIFY_IS_TRUE(IsColor(image.At(48, 32), c_red), L"outside damage keeps previous frame");
        VERIFY_IS_TRUE(IsColor(image.At(48, 4), c_red), L"drawing is clipped to damage");
        VERIFY_ARE_EQUAL(1u, stats.damageRectCount);
        VERIFY_ARE_EQUAL(static_cast<uint64_t>(32) * 64, stats.damagedPixels);
    }

    void PainterConformanceTests::ResizeReallocatesTarget()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        VERIFY_SUCCEEDED(painter->Resize(128, 32, 1.0f));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 100, 8, 120, 24 }, c_red);
        });
        const auto image = Read(*painter, 128, 32);
        VERIFY_IS_TRUE(IsColor(image.At(110, 16), c_red));
        VERIFY_IS_TRUE(painter->GetResourceInventory().surfaceBytes >= 128ull * 32 * 4);
    }

    void PainterConformanceTests::InventoryAccountsForTarget()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        const auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"surface %llu cache %llu objects %u upload %llu",
            inventory.surfaceBytes, inventory.cacheBytes, inventory.cachedObjectCount, inventory.uploadBytesThisFrame));
        VERIFY_IS_TRUE(inventory.surfaceBytes >= static_cast<uint64_t>(c_size) * c_size * 4);
    }

    void PainterConformanceTests::PaintTracedFrameReturnsStats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        FrameStats stats{};
        VERIFY_SUCCEEDED(PaintTracedFrame(*painter, 1, 1, nullptr, 0, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRectangle({ 0, 0, 4, 4 }, c_red);
        }, &stats));
        VERIFY_ARE_EQUAL(1u, stats.primitiveCount);
    }

    void PainterFactoryTests::CompositionIsNotAnImmediatePainter()
    {
        std::unique_ptr<IImmediatePainter> painter;
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateImmediatePainter(Mode::Composition, PainterOptions{ 8, 8, 1.0f }, painter));
        VERIFY_IS_NULL(painter.get());
    }

    void PainterFactoryTests::RejectsEmptyTarget()
    {
        std::unique_ptr<IImmediatePainter> painter;
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateImmediatePainter(Mode::ImmediateD2DSoftware, PainterOptions{ 0, 8, 1.0f }, painter));
        VERIFY_ARE_EQUAL(E_INVALIDARG, CreateImmediatePainter(Mode::ImmediateGdiPlus, PainterOptions{ 8, 8, 0.0f }, painter));
    }

    void PainterFactoryTests::WarpDeviceReportsWarpIdentity()
    {
        Microsoft::WRL::ComPtr<ID3D11Device> device;
        DeviceIdentity identity{};
        VERIFY_SUCCEEDED(CreateD3DDeviceForMode(Mode::ImmediateD2DSoftware, &device, &identity));
        VERIFY_IS_TRUE(identity.mode == Mode::ImmediateD2DSoftware);
        VERIFY_IS_TRUE(identity.isWarp);
        VERIFY_IS_TRUE(identity.featureLevel >= D3D_FEATURE_LEVEL_10_0);
        VERIFY_IS_TRUE(identity.adapterLuid != 0);
    }
}
