// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediatePainterExtendedTests.h"
#include "ImmediatePainterTestHelpers.h"
#include <ImmediatePainter.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace PainterTestHelpers;
    using namespace WEX::Common;
    using namespace WEX::Logging;
    using Microsoft::WRL::ComPtr;

    namespace
    {
        constexpr ColorF c_green{ 0, 1, 0, 1 };
        constexpr ColorF c_transparent{ 0, 0, 0, 0 };

        // GDI+ antialiasing and filtering differ slightly from D2D's; allow it a little more slack.
        int Tolerance(int base)
        {
            return CurrentMode() == Mode::ImmediateGdiPlus ? base + 4 : base;
        }

        int MaxChannelDifference(const Pixel& a, const Pixel& b)
        {
            return std::max({ std::abs(a.b - b.b), std::abs(a.g - b.g), std::abs(a.r - b.r), std::abs(a.a - b.a) });
        }

        bool SamePixel(const Pixel& actual, const Pixel& expected, int tolerance)
        {
            return MaxChannelDifference(actual, expected) <= tolerance;
        }

        // Verifies every pixel of the image is (near) the given color.
        void VerifyAllPixels(const Image& image, const ColorF& color, const wchar_t* message)
        {
            uint32_t mismatches = 0;
            for (uint32_t y = 0; y < image.height; ++y)
            {
                for (uint32_t x = 0; x < image.width; ++x)
                {
                    if (!IsColor(image.At(x, y), color))
                    {
                        if (mismatches++ == 0)
                        {
                            LogPixel(L"first mismatch", x, y, image.At(x, y));
                        }
                    }
                }
            }
            VERIFY_ARE_EQUAL(0u, mismatches, message);
        }

        // ---- Paths ----

        struct PathBuilder
        {
            std::vector<PathVerb> verbs;
            std::vector<PointF> points;

            PathBuilder& Move(float x, float y) { verbs.push_back(PathVerb::MoveTo); points.push_back({ x, y }); return *this; }
            PathBuilder& Line(float x, float y) { verbs.push_back(PathVerb::LineTo); points.push_back({ x, y }); return *this; }
            PathBuilder& Cubic(float x1, float y1, float x2, float y2, float x3, float y3)
            {
                verbs.push_back(PathVerb::CubicTo);
                points.push_back({ x1, y1 });
                points.push_back({ x2, y2 });
                points.push_back({ x3, y3 });
                return *this;
            }
            PathBuilder& Close() { verbs.push_back(PathVerb::Close); return *this; }

            PathData Data(bool nonZeroFill = false, uint64_t cacheKey = 0) const
            {
                PathData data{};
                data.verbs = verbs.data();
                data.verbCount = static_cast<uint32_t>(verbs.size());
                data.points = points.data();
                data.pointCount = static_cast<uint32_t>(points.size());
                data.nonZeroFill = nonZeroFill;
                data.cacheKey = cacheKey;
                return data;
            }
        };

        PathBuilder Triangle()
        {
            PathBuilder path;
            path.Move(8, 8).Line(56, 8).Line(8, 56).Close();
            return path;
        }

        // Pentagram (vertices visited 0, 2, 4, 1, 3): the center pentagon has winding 2, so even-odd leaves it empty.
        PathBuilder Star()
        {
            constexpr float pi = 3.14159265f;
            PointF vertices[5];
            for (int k = 0; k < 5; ++k)
            {
                const float angle = (-90.0f + 72.0f * k) * pi / 180.0f;
                vertices[k] = { 32 + 28 * std::cos(angle), 32 + 28 * std::sin(angle) };
            }
            PathBuilder path;
            path.Move(vertices[0].x, vertices[0].y);
            for (int k : { 2, 4, 1, 3 })
            {
                path.Line(vertices[k].x, vertices[k].y);
            }
            path.Close();
            return path;
        }

        // ---- Glyph runs ----

        struct GlyphRunFixture
        {
            ComPtr<IDWriteFontFace> fontFace;
            std::vector<uint16_t> glyphIndices;
            std::vector<float> advances;
            float emSize = 0;
            float ascent = 0;  // DIPs above the baseline
            float descent = 0; // DIPs below the baseline
            float width = 0;   // sum of advances

            void Create(const wchar_t* text, float fontEmSize)
            {
                emSize = fontEmSize;
                ComPtr<IDWriteFactory> factory;
                VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())));
                ComPtr<IDWriteFontCollection> collection;
                VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection, FALSE));
                UINT32 familyIndex = 0;
                BOOL exists = FALSE;
                VERIFY_SUCCEEDED(collection->FindFamilyName(L"Segoe UI", &familyIndex, &exists));
                VERIFY_IS_TRUE(!!exists, L"Segoe UI is installed");
                ComPtr<IDWriteFontFamily> family;
                VERIFY_SUCCEEDED(collection->GetFontFamily(familyIndex, &family));
                ComPtr<IDWriteFont> font;
                VERIFY_SUCCEEDED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font));
                VERIFY_SUCCEEDED(font->CreateFontFace(&fontFace));

                std::vector<UINT32> codePoints;
                for (const wchar_t* c = text; *c; ++c)
                {
                    codePoints.push_back(*c);
                }
                glyphIndices.resize(codePoints.size());
                VERIFY_SUCCEEDED(fontFace->GetGlyphIndices(codePoints.data(), static_cast<UINT32>(codePoints.size()), glyphIndices.data()));

                DWRITE_FONT_METRICS fontMetrics{};
                fontFace->GetMetrics(&fontMetrics);
                const float scale = emSize / fontMetrics.designUnitsPerEm;
                ascent = fontMetrics.ascent * scale;
                descent = fontMetrics.descent * scale;

                std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(glyphIndices.size());
                VERIFY_SUCCEEDED(fontFace->GetDesignGlyphMetrics(glyphIndices.data(), static_cast<UINT32>(glyphIndices.size()), glyphMetrics.data(), FALSE));
                width = 0;
                for (const auto& metrics : glyphMetrics)
                {
                    advances.push_back(metrics.advanceWidth * scale);
                    width += advances.back();
                }
                Log::Comment(String().Format(L"glyph run: %u glyphs, width %.2f, ascent %.2f, descent %.2f",
                    static_cast<uint32_t>(glyphIndices.size()), width, ascent, descent));
            }

            GlyphRunDesc Run(PointF baselineOrigin, uint32_t bidiLevel = 0) const
            {
                GlyphRunDesc run{};
                run.fontFace = fontFace.Get();
                run.fontEmSize = emSize;
                run.glyphIndices = glyphIndices.data();
                run.glyphAdvances = advances.data();
                run.glyphCount = static_cast<uint32_t>(glyphIndices.size());
                run.baselineOrigin = baselineOrigin;
                run.bidiLevel = bidiLevel;
                return run;
            }
        };

        struct InkCount
        {
            uint32_t inside = 0;
            uint32_t outside = 0;
            int32_t minX = INT32_MAX, maxX = INT32_MIN;
        };

        // Counts ink pixels inside / outside the box [left, right) x [top, bottom) (box edges in pixels, rounded out).
        InkCount CountInk(const Image& image, float left, float top, float right, float bottom)
        {
            InkCount count;
            const int l = static_cast<int>(std::floor(left));
            const int t = static_cast<int>(std::floor(top));
            const int r = static_cast<int>(std::ceil(right));
            const int b = static_cast<int>(std::ceil(bottom));
            for (uint32_t y = 0; y < image.height; ++y)
            {
                for (uint32_t x = 0; x < image.width; ++x)
                {
                    if (!IsInk(image.At(x, y))) continue;
                    const int ix = static_cast<int>(x), iy = static_cast<int>(y);
                    const bool inside = ix >= l && ix < r && iy >= t && iy < b;
                    ++(inside ? count.inside : count.outside);
                    count.minX = std::min(count.minX, ix);
                    count.maxX = std::max(count.maxX, ix);
                }
            }
            Log::Comment(String().Format(L"ink inside [%d,%d)x[%d,%d): %u, outside: %u, x range [%d,%d]",
                l, r, t, b, count.inside, count.outside, count.minX, count.maxX));
            return count;
        }

        // ---- Bitmaps ----

        constexpr uint32_t c_bitmapSize = 16;

        Pixel PremultipliedHalfRed() { return { 0, 0, 128, 128 }; }

        // 16x16 premultiplied BGRA8 with 8x8 quadrants: red, green / blue, 50% red (premultiplied). The stride is padded
        // and the padding holds garbage that must never be drawn.
        struct TestBitmap
        {
            std::vector<uint8_t> bytes;
            BitmapSource source{};

            static Pixel Quadrant(uint32_t x, uint32_t y)
            {
                const bool right = x >= c_bitmapSize / 2;
                const bool bottom = y >= c_bitmapSize / 2;
                if (!bottom) return right ? Pixel{ 0, 255, 0, 255 } : Pixel{ 0, 0, 255, 255 };
                return right ? PremultipliedHalfRed() : Pixel{ 255, 0, 0, 255 };
            }

            void Create(uint64_t key, uint64_t generation, const Pixel* solid = nullptr)
            {
                const uint32_t stride = c_bitmapSize * 4 + 16;
                bytes.assign(static_cast<size_t>(stride) * c_bitmapSize, 0x7F);
                for (uint32_t y = 0; y < c_bitmapSize; ++y)
                {
                    for (uint32_t x = 0; x < c_bitmapSize; ++x)
                    {
                        const Pixel p = solid ? *solid : Quadrant(x, y);
                        uint8_t* dst = &bytes[static_cast<size_t>(y) * stride + x * 4];
                        dst[0] = p.b; dst[1] = p.g; dst[2] = p.r; dst[3] = p.a;
                    }
                }
                source.key = key;
                source.generation = generation;
                source.width = c_bitmapSize;
                source.height = c_bitmapSize;
                source.stride = stride;
                source.pixels = bytes.data();
            }
        };

        // Source-over a premultiplied pixel onto opaque white.
        Pixel OverWhite(const Pixel& p)
        {
            const int inverse = 255 - p.a;
            return { static_cast<uint8_t>(p.b + inverse), static_cast<uint8_t>(p.g + inverse), static_cast<uint8_t>(p.r + inverse), 255 };
        }

        constexpr uint64_t c_bitmapBytes = static_cast<uint64_t>(c_bitmapSize) * c_bitmapSize * 4;

        // ---- Scrolling ----

        // 4x4 blocks whose red encodes the block column and green the block row, so any displacement is visible.
        void PaintBlockPattern(IImmediatePainter& p)
        {
            p.Clear(c_white);
            for (uint32_t by = 0; by < c_size / 4; ++by)
            {
                for (uint32_t bx = 0; bx < c_size / 4; ++bx)
                {
                    const ColorF color{ bx * 16 / 255.0f, by * 16 / 255.0f, 128 / 255.0f, 1 };
                    const float x = static_cast<float>(bx * 4), y = static_cast<float>(by * 4);
                    p.FillRectangle({ x, y, x + 4, y + 4 }, color);
                }
            }
        }

        ComPtr<IDXGISwapChain1> CreateCompositionSwapChain(ID3D11Device* device, uint32_t width, uint32_t height)
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
            desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
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

        // A painter for the current mode plus a D3D device a swap chain for it can be created on (the painter's own
        // device for D2D; a WARP device for GDI+, which accepts any device). Returns false after logging Skipped when
        // the mode needs hardware this machine does not have.
        bool CreatePresentablePainter(std::unique_ptr<IImmediatePainter>& painter, ComPtr<ID3D11Device>& device)
        {
            const Mode mode = CurrentMode();
            const Mode deviceMode = mode == Mode::ImmediateGdiPlus ? Mode::ImmediateD2DSoftware : mode;
            DeviceIdentity identity{};
            const HRESULT hr = CreateD3DDeviceForMode(deviceMode, &device, &identity);
            if (mode == Mode::ImmediateD2DHardware && hr == DXGI_ERROR_NOT_FOUND)
            {
                LogNoHardwareSkip();
                return false;
            }
            VERIFY_SUCCEEDED(hr, L"CreateD3DDeviceForMode");
            const PainterOptions options{ c_size, c_size, 1.0f };
            if (mode == Mode::ImmediateGdiPlus)
            {
                VERIFY_SUCCEEDED(CreateGdiPlusPainter(options, painter));
            }
            else
            {
                VERIFY_SUCCEEDED(CreateD2DPainter(mode, device.Get(), options, painter));
            }
            VERIFY_IS_NOT_NULL(painter.get());
            return true;
        }
    }

    // ---- Rounded-rectangle clips ----

    void ExtendedPainterConformanceTests::RoundedClipRestrictsDrawingToRoundedRect()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 16, 16);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.PopRoundedRectangleClip();
            p.FillRectangle({ 0, 0, 4, 4 }, c_red);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        VERIFY_IS_TRUE(IsColor(image.At(32, 32), c_black, tolerance), L"center is inside the clip");
        VERIFY_IS_TRUE(IsColor(image.At(32, 8), c_black, tolerance), L"straight edge is inside the clip");
        VERIFY_IS_TRUE(IsColor(image.At(8, 32), c_black, tolerance), L"straight edge is inside the clip");
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(55, 55), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(4, 32), c_white, tolerance), L"outside the clip rect is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(60, 32), c_white, tolerance), L"outside the clip rect is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(1, 1), c_red), L"pop restores unclipped drawing");

        // The arc crosses the diagonal near (12.7, 12.7); at least one pixel there must be partially covered.
        bool foundPartial = false;
        for (uint32_t i = 8; i < 24 && !foundPartial; ++i)
        {
            const auto p = image.At(i, i);
            LogPixel(L"diagonal", i, i, p);
            foundPartial = p.r > 24 && p.r < 231;
        }
        VERIFY_IS_TRUE(foundPartial, L"expected an antialiased clip edge on the rounded corner");
    }

    void ExtendedPainterConformanceTests::RoundedClipNestsWithAxisAlignedClipAndOpacityLayer()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 32, 64 });
            p.PushRoundedRectangleClip({ 8, 8, 56, 56 }, 12, 12);
            p.PushOpacityLayer(0.5f);
            p.FillRectangle({ 0, 0, 64, 64 }, c_black);
            p.FillRectangle({ 16, 16, 48, 48 }, c_black);
            p.PopOpacityLayer();
            p.PopRoundedRectangleClip();
            p.PopAxisAlignedClip();
            p.FillRectangle({ 56, 56, 64, 64 }, c_red);
        });
        const auto image = Read(*painter);
        const auto single = image.At(12, 32);
        const auto overlap = image.At(24, 32);
        LogPixel(L"single", 12, 32, single);
        LogPixel(L"overlap", 24, 32, overlap);
        VERIFY_IS_TRUE(Near(single.r, 128, Tolerance(8)), L"opacity layer applies inside the rounded clip");
        VERIFY_IS_TRUE(Near(overlap.r, single.r, Tolerance(3)), L"group opacity: overlap is blended once");
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_white, Tolerance(3)), L"rounded corner clipped");
        VERIFY_IS_TRUE(IsColor(image.At(4, 32), c_white, Tolerance(3)), L"outside the rounded clip");
        VERIFY_IS_TRUE(IsColor(image.At(40, 32), c_white, Tolerance(3)), L"outside the axis-aligned clip");
        VERIFY_IS_TRUE(IsColor(image.At(60, 60), c_red), L"balanced pops restore unclipped drawing");
    }

    // ---- Paths ----

    void ExtendedPainterConformanceTests::FillPathFillsTriangle()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const auto triangle = Triangle();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillPath(triangle.Data(), c_red);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        VERIFY_IS_TRUE(IsColor(image.At(16, 16), c_red, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(40, 10), c_red, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(10, 40), c_red, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(40, 40), c_white, tolerance), L"beyond the hypotenuse");
        VERIFY_IS_TRUE(IsColor(image.At(4, 32), c_white, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 4), c_white, tolerance));
    }

    void ExtendedPainterConformanceTests::FillPathHonorsFillRule()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const int tolerance = Tolerance(3);

        // Two overlapping squares with the same winding: the overlap has winding 2.
        PathBuilder squares;
        squares.Move(8, 8).Line(40, 8).Line(40, 40).Line(8, 40).Close();
        squares.Move(24, 24).Line(56, 24).Line(56, 56).Line(24, 56).Close();
        for (const bool nonZero : { false, true })
        {
            Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.FillPath(squares.Data(nonZero), c_red);
            });
            const auto image = Read(*painter);
            Log::Comment(nonZero ? L"squares, nonzero" : L"squares, even-odd");
            LogPixel(L"overlap", 32, 32, image.At(32, 32));
            VERIFY_IS_TRUE(IsColor(image.At(12, 12), c_red, tolerance));
            VERIFY_IS_TRUE(IsColor(image.At(50, 50), c_red, tolerance));
            VERIFY_IS_TRUE(IsColor(image.At(32, 32), nonZero ? c_red : c_white, tolerance));
            VERIFY_IS_TRUE(IsColor(image.At(50, 12), c_white, tolerance));
        }

        const auto star = Star();
        for (const bool nonZero : { false, true })
        {
            Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.FillPath(star.Data(nonZero), c_red);
            });
            const auto image = Read(*painter);
            Log::Comment(nonZero ? L"star, nonzero" : L"star, even-odd");
            LogPixel(L"center", 32, 32, image.At(32, 32));
            VERIFY_IS_TRUE(IsColor(image.At(32, 10), c_red, tolerance), L"star tip is filled");
            VERIFY_IS_TRUE(IsColor(image.At(32, 32), nonZero ? c_red : c_white, tolerance), L"center pentagon depends on fill rule");
            VERIFY_IS_TRUE(IsColor(image.At(2, 2), c_white, tolerance));
        }
    }

    void ExtendedPainterConformanceTests::FillPathFillsCubicCurve()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // A dome: the curve peaks at y = 20 for x = 32 and at x = 12 passes y ~= 35. The control polygon would cover
        // the top corners, the curve does not.
        PathBuilder dome;
        dome.Move(8, 56).Cubic(8, 8, 56, 8, 56, 56).Close();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillPath(dome.Data(), c_blue);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        VERIFY_IS_TRUE(IsColor(image.At(32, 40), c_blue, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 24), c_blue, tolerance), L"below the peak");
        VERIFY_IS_TRUE(IsColor(image.At(12, 45), c_blue, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 14), c_white, tolerance), L"above the peak");
        VERIFY_IS_TRUE(IsColor(image.At(12, 28), c_white, tolerance), L"curve, not the control polygon");
        VERIFY_IS_TRUE(IsColor(image.At(10, 12), c_white, tolerance), L"curve, not the control polygon");
        VERIFY_IS_TRUE(IsColor(image.At(32, 60), c_white, tolerance), L"below the closing line");
    }

    void ExtendedPainterConformanceTests::StrokePathIsCenteredOnPath()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        PathBuilder line;
        line.Move(8, 32).Line(56, 32);
        PathBuilder square;
        square.Move(16, 8).Line(48, 8).Line(48, 24).Line(16, 24).Close();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokePath(line.Data(), 4, c_black);
            p.StrokePath(square.Data(), 2, c_black);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(8);

        // 4 DIP line centered on y = 32 covers rows 30..33; flat caps end exactly at x = 8 and x = 56.
        VERIFY_IS_TRUE(IsColor(image.At(32, 30), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 33), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 28), c_white, tolerance), L"above the stroke");
        VERIFY_IS_TRUE(IsColor(image.At(32, 35), c_white, tolerance), L"below the stroke");
        VERIFY_IS_TRUE(IsColor(image.At(9, 32), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(54, 32), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(6, 32), c_white, tolerance), L"flat cap: no extension past the start");
        VERIFY_IS_TRUE(IsColor(image.At(57, 32), c_white, tolerance), L"flat cap: no extension past the end");

        // 2 DIP closed stroke on y = 8 covers rows 7..8; the interior is not filled.
        VERIFY_IS_TRUE(IsColor(image.At(32, 7), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 8), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(15, 16), c_black, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(32, 16), c_white, tolerance), L"stroke does not fill the interior");
        VERIFY_IS_TRUE(IsColor(image.At(32, 5), c_white, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(15, 7), c_black, tolerance), L"miter join fills the corner");
    }

    void ExtendedPainterConformanceTests::FillPathCacheKeyReuseIsStable()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const auto star = Star();
        auto paintStar = [&](uint64_t cacheKey)
        {
            Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.FillPath(star.Data(true, cacheKey), c_red);
            });
            return Read(*painter);
        };

        const auto first = paintStar(42);
        VERIFY_IS_TRUE(IsColor(first.At(32, 32), c_red, Tolerance(3)), L"star is drawn");
        VERIFY_IS_TRUE(IsColor(first.At(32, 10), c_red, Tolerance(3)), L"star is drawn");
        const auto cached = paintStar(42);
        const auto uncached = paintStar(0);
        int cachedDifference = 0, uncachedDifference = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                cachedDifference = std::max(cachedDifference, MaxChannelDifference(first.At(x, y), cached.At(x, y)));
                uncachedDifference = std::max(uncachedDifference, MaxChannelDifference(first.At(x, y), uncached.At(x, y)));
            }
        }
        Log::Comment(String().Format(L"max difference: cached %d, uncached %d", cachedDifference, uncachedDifference));
        VERIFY_ARE_EQUAL(0, cachedDifference, L"reusing a cache key reproduces the same pixels");
        VERIFY_IS_TRUE(uncachedDifference <= 1, L"cached and uncached geometry render the same");
    }

    void ExtendedPainterConformanceTests::InvalidPathDataIsNoOp()
    {
        auto painter = CreatePainter();
        if (!painter) return;

        // Would draw in the bottom-right quadrant if partially honored.
        PathBuilder truncated;
        truncated.Move(36, 36).Line(60, 36).Line(60, 60).Cubic(50, 60, 40, 60, 36, 60).Close();
        PathData missingPoints = truncated.Data();
        missingPoints.pointCount = 4; // the cubic needs 3 more points
        PathData nullPoints = truncated.Data();
        nullPoints.points = nullptr;
        PathData nullVerbs = truncated.Data();
        nullVerbs.verbs = nullptr;
        const PathData empty{};
        const PathVerb closeOnly[] = { PathVerb::Close };
        PathData noMove{};
        noMove.verbs = closeOnly;
        noMove.verbCount = 1;

        const auto triangle = Triangle();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            for (const PathData* path : std::initializer_list<const PathData*>{ &empty, &missingPoints, &nullPoints, &nullVerbs, &noMove })
            {
                p.FillPath(*path, c_black);
                p.StrokePath(*path, 2, c_black);
            }
            PathData triangleData = triangle.Data();
            triangleData.verbCount = 0;
            p.FillPath(triangleData, c_black);
            p.FillPath(triangle.Data(), c_red);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(16, 16), c_red, Tolerance(3)), L"a valid path after invalid ones still draws");
        for (uint32_t y = 36; y < c_size; ++y)
        {
            for (uint32_t x = 36; x < c_size; ++x)
            {
                if (!IsColor(image.At(x, y), c_white))
                {
                    LogPixel(L"unexpected ink", x, y, image.At(x, y));
                    VERIFY_FAIL(L"invalid path data must not draw");
                }
            }
        }
    }

    // ---- Glyph runs ----

    void ExtendedPainterConformanceTests::GlyphRunInkStaysWithinRunBounds()
    {
        constexpr uint32_t width = 128;
        auto painter = CreatePainter(width, c_size);
        if (!painter) return;
        GlyphRunFixture fixture;
        fixture.Create(L"Hello", 20);
        const PointF origin{ 8, 40 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(fixture.Run(origin), c_red);
        });
        const auto image = Read(*painter, width, c_size);
        const auto ink = CountInk(image, origin.x - 1, origin.y - fixture.ascent - 1, origin.x + fixture.width + 1, origin.y + fixture.descent + 1);
        VERIFY_IS_TRUE(ink.inside > 40, L"the run produces ink");
        VERIFY_ARE_EQUAL(0u, ink.outside, L"no ink outside [origin, origin + advances] x [baseline - ascent, baseline + descent]");
        VERIFY_IS_TRUE(ink.maxX > origin.x + fixture.width * 0.75f, L"ink spans the advances");

        // Ink is the requested color with grayscale coverage: red over white keeps R at 255 and G == B.
        bool foundSolid = false;
        uint32_t offColor = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                const auto p = image.At(x, y);
                if (!IsInk(p)) continue;
                if (p.r < 230 || std::abs(p.g - p.b) > 16)
                {
                    if (offColor++ == 0) LogPixel(L"off-color ink", x, y, p);
                }
                foundSolid = foundSolid || IsColor(p, c_red, Tolerance(24));
            }
        }
        VERIFY_ARE_EQUAL(0u, offColor, L"ink matches the requested color");
        VERIFY_IS_TRUE(foundSolid, L"fully covered stems are the requested color");
    }

    void ExtendedPainterConformanceTests::GlyphRunEmptyOrNullFontFaceIsNoOp()
    {
        constexpr uint32_t width = 128;
        constexpr uint32_t height = 96;
        auto painter = CreatePainter(width, height);
        if (!painter) return;
        GlyphRunFixture fixture;
        fixture.Create(L"Hello", 20);
        const auto stats = Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            auto empty = fixture.Run({ 8, 30 });
            empty.glyphCount = 0;
            p.DrawGlyphRun(empty, c_black);
            auto noFont = fixture.Run({ 8, 30 });
            noFont.fontFace = nullptr;
            p.DrawGlyphRun(noFont, c_black);
            p.DrawGlyphRun(fixture.Run({ 8, 80 }), c_black);
        });
        const auto image = Read(*painter, width, height);
        const auto ink = CountInk(image, 0, 48, static_cast<float>(width), static_cast<float>(height));
        VERIFY_ARE_EQUAL(0u, ink.outside, L"empty and null-font runs draw nothing");
        VERIFY_IS_TRUE(ink.inside > 40, L"a valid run after them still draws");
        Log::Comment(String().Format(L"primitiveCount %u", stats.primitiveCount));
    }

    void ExtendedPainterConformanceTests::GlyphRunRightToLeftExtendsLeftOfOrigin()
    {
        constexpr uint32_t width = 128;
        auto painter = CreatePainter(width, c_size);
        if (!painter) return;
        GlyphRunFixture fixture;
        fixture.Create(L"Hello", 20);
        const PointF origin{ 100, 40 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(fixture.Run(origin, 1), c_black);
        });
        const auto image = Read(*painter, width, c_size);
        const auto ink = CountInk(image, origin.x - fixture.width - 1, origin.y - fixture.ascent - 1, origin.x + 1, origin.y + fixture.descent + 1);
        VERIFY_IS_TRUE(ink.inside > 40, L"the run produces ink");
        VERIFY_ARE_EQUAL(0u, ink.outside, L"right-to-left ink stays in [origin - advances, origin]");
        VERIFY_IS_TRUE(ink.minX < origin.x - fixture.width * 0.75f, L"ink extends left of the origin");
    }

    // ---- Bitmaps ----

    void ExtendedPainterConformanceTests::BitmapDrawsOneToOne()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        uint32_t mismatches = 0;
        for (uint32_t y = 1; y + 1 < c_bitmapSize; ++y)
        {
            for (uint32_t x = 1; x + 1 < c_bitmapSize; ++x)
            {
                const auto expected = OverWhite(TestBitmap::Quadrant(x, y));
                const auto actual = image.At(8 + x, 8 + y);
                if (!SamePixel(actual, expected, tolerance) && mismatches++ == 0)
                {
                    LogPixel(L"first mismatch", 8 + x, 8 + y, actual);
                    LogPixel(L"expected", 8 + x, 8 + y, expected);
                }
            }
        }
        VERIFY_ARE_EQUAL(0u, mismatches, L"1:1 draw reproduces the (premultiplied) source pixels");
        VERIFY_IS_TRUE(IsColor(image.At(7, 7), c_white), L"outside the destination");
        VERIFY_IS_TRUE(IsColor(image.At(24, 24), c_white), L"outside the destination");
        VERIFY_IS_TRUE(IsColor(image.At(40, 12), c_white), L"outside the destination");
    }

    void ExtendedPainterConformanceTests::BitmapScalesWithLinearFiltering()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 0, 0, 32, 32 }, nullptr, 1.0f);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(4);
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_red, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(24, 8), c_green, tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(8, 24), c_blue, tolerance));
        VERIFY_IS_TRUE(SamePixel(image.At(24, 24), OverWhite(PremultipliedHalfRed()), tolerance));
        VERIFY_IS_TRUE(IsColor(image.At(40, 8), c_white), L"outside the destination");

        // Linear filtering blends across the red/green boundary (between dest x = 15 and 16); nearest would not.
        bool foundBlend = false;
        for (uint32_t x = 14; x <= 17; ++x)
        {
            const auto p = image.At(x, 8);
            LogPixel(L"boundary", x, 8, p);
            foundBlend = foundBlend || (p.r > 24 && p.r < 231 && p.g > 24 && p.g < 231);
        }
        VERIFY_IS_TRUE(foundBlend, L"expected linear filtering at the scaled quadrant boundary");
    }

    void ExtendedPainterConformanceTests::BitmapSourceRectSelectsSubRegion()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        const RectF greenQuadrant{ 8, 0, 16, 8 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 40, 40 }, &greenQuadrant, 1.0f);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(6);
        for (uint32_t y = 12; y < 36; y += 4)
        {
            for (uint32_t x = 12; x < 36; x += 4)
            {
                if (!IsColor(image.At(x, y), c_green, tolerance))
                {
                    LogPixel(L"not green", x, y, image.At(x, y));
                    VERIFY_FAIL(L"source rect selects only the green quadrant");
                }
            }
        }
        VERIFY_IS_TRUE(IsColor(image.At(4, 4), c_white));
        VERIFY_IS_TRUE(IsColor(image.At(44, 44), c_white));
    }

    void ExtendedPainterConformanceTests::BitmapOpacityScalesPremultipliedValues()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 0.5f);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        const auto red = image.At(11, 11);
        const auto halfRed = image.At(20, 20);
        LogPixel(L"red at 50%", 11, 11, red);
        LogPixel(L"50% red at 50%", 20, 20, halfRed);
        VERIFY_IS_TRUE(SamePixel(red, { 0, 0, 128, 128 }, tolerance), L"opaque red at opacity 0.5");
        VERIFY_IS_TRUE(SamePixel(halfRed, { 0, 0, 64, 64 }, tolerance), L"premultiplied 50% red at opacity 0.5");
        VERIFY_IS_TRUE(SamePixel(image.At(4, 4), { 0, 0, 0, 0 }, 0), L"outside stays transparent");
    }

    void ExtendedPainterConformanceTests::BitmapUploadsOncePerGeneration()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        const uint64_t cacheBefore = painter->GetResourceInventory().cacheBytes;

        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source), L"outside a frame");
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        auto inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"after first upload: cache %llu (was %llu), upload %llu", inventory.cacheBytes, cacheBefore, inventory.uploadBytesThisFrame));
        VERIFY_IS_TRUE(inventory.uploadBytesThisFrame >= c_bitmapBytes, L"an upload outside a frame counts in the next frame");
        VERIFY_IS_TRUE(inventory.cacheBytes >= cacheBefore + c_bitmapBytes, L"bitmap storage counts in cacheBytes");
        VERIFY_IS_TRUE(IsColor(Read(*painter).At(10, 10), c_red, Tolerance(3)));

        Paint(*painter, [&](IImmediatePainter& p)
        {
            VERIFY_SUCCEEDED(p.EnsureBitmap(bitmap.source), L"same (key, generation) again");
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        inventory = painter->GetResourceInventory();
        VERIFY_ARE_EQUAL(0ull, inventory.uploadBytesThisFrame, L"same (key, generation) uploads nothing");

        const Pixel blue{ 255, 0, 0, 255 };
        TestBitmap updated;
        updated.Create(1, 2, &blue);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            VERIFY_SUCCEEDED(p.EnsureBitmap(updated.source), L"new generation inside a frame");
            p.Clear(c_white);
            p.DrawBitmap(1, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        inventory = painter->GetResourceInventory();
        Log::Comment(String().Format(L"after new generation: cache %llu, upload %llu", inventory.cacheBytes, inventory.uploadBytesThisFrame));
        VERIFY_IS_TRUE(inventory.uploadBytesThisFrame >= c_bitmapBytes, L"a new generation re-uploads in the same frame");
        VERIFY_IS_TRUE(IsColor(Read(*painter).At(10, 10), c_blue, Tolerance(3)), L"the new generation's pixels are drawn");
    }

    void ExtendedPainterConformanceTests::BitmapReleaseFreesCacheAndDisablesDraw()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(7, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(7, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        VERIFY_IS_TRUE(IsColor(Read(*painter).At(10, 10), c_red, Tolerance(3)));
        const uint64_t cacheWith = painter->GetResourceInventory().cacheBytes;

        painter->ReleaseBitmap(7);
        const uint64_t cacheAfter = painter->GetResourceInventory().cacheBytes;
        Log::Comment(String().Format(L"cache with bitmap %llu, after release %llu", cacheWith, cacheAfter));
        VERIFY_IS_TRUE(cacheWith >= c_bitmapBytes && cacheAfter <= cacheWith - c_bitmapBytes, L"release frees the bitmap storage");

        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(7, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        VerifyAllPixels(Read(*painter), c_white, L"drawing a released bitmap is a no-op");
        painter->ReleaseBitmap(7); // releasing twice is harmless

        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source), L"the same (key, generation) can be re-created after release");
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(7, { 8, 8, 24, 24 }, nullptr, 1.0f);
        });
        VERIFY_IS_TRUE(painter->GetResourceInventory().uploadBytesThisFrame >= c_bitmapBytes, L"re-created after release uploads again");
        VERIFY_IS_TRUE(IsColor(Read(*painter).At(10, 10), c_red, Tolerance(3)));
    }

    void ExtendedPainterConformanceTests::BitmapUnknownKeyIsNoOp()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        painter->ReleaseBitmap(12345); // unknown key: harmless
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(999, { 0, 0, 32, 64 }, nullptr, 1.0f);
            p.DrawBitmap(1, { 40, 8, 56, 24 }, nullptr, 1.0f);
        });
        const auto image = Read(*painter);
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < 32; ++x)
            {
                if (!IsColor(image.At(x, y), c_white))
                {
                    LogPixel(L"unexpected", x, y, image.At(x, y));
                    VERIFY_FAIL(L"unknown key draws nothing");
                }
            }
        }
        VERIFY_IS_TRUE(IsColor(image.At(42, 10), c_red, Tolerance(3)), L"a known key in the same frame still draws");
    }

    // ---- Scrolling ----

    void ExtendedPainterConformanceTests::ScrollPixelsMovesContentVertically()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, PaintBlockPattern);
        const auto before = Read(*painter);

        const RectF scrollRect{ 8, 8, 56, 56 };
        VERIFY_SUCCEEDED(painter->ScrollPixels(scrollRect, 0, -10));
        const RectF exposed{ 8, 46, 56, 56 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); }, &exposed, 1);
        const auto after = Read(*painter);

        uint32_t moved = 0, strip = 0, outside = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                const bool inRect = x >= 8 && x < 56 && y >= 8 && y < 56;
                const auto actual = after.At(x, y);
                if (!inRect)
                {
                    if (!SamePixel(actual, before.At(x, y), 1) && outside++ == 0) LogPixel(L"outside changed", x, y, actual);
                }
                else if (y < 46)
                {
                    if (!SamePixel(actual, before.At(x, y + 10), 1) && moved++ == 0) LogPixel(L"moved mismatch", x, y, actual);
                }
                else if (!IsColor(actual, c_red) && strip++ == 0)
                {
                    LogPixel(L"exposed strip mismatch", x, y, actual);
                }
            }
        }
        VERIFY_ARE_EQUAL(0u, moved, L"pixels inside the rect move up by 10");
        VERIFY_ARE_EQUAL(0u, strip, L"the exposed strip is repainted");
        VERIFY_ARE_EQUAL(0u, outside, L"pixels outside the rect are unchanged");
    }

    void ExtendedPainterConformanceTests::ScrollPixelsMovesContentHorizontally()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, PaintBlockPattern);
        const auto before = Read(*painter);

        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 16, 64, 48 }, 6, 0));
        const RectF exposed{ 0, 16, 6, 48 };
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); }, &exposed, 1);
        const auto after = Read(*painter);

        uint32_t mismatches = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                const bool inRect = y >= 16 && y < 48;
                const auto actual = after.At(x, y);
                bool ok;
                if (!inRect) ok = SamePixel(actual, before.At(x, y), 1);
                else if (x >= 6) ok = SamePixel(actual, before.At(x - 6, y), 1);
                else ok = IsColor(actual, c_red);
                if (!ok && mismatches++ == 0) LogPixel(L"first mismatch", x, y, actual);
            }
        }
        VERIFY_ARE_EQUAL(0u, mismatches, L"content inside the rect moves right by 6, clipped to the rect");
    }

    void ExtendedPainterConformanceTests::ScrollPixelsFailsInsideFrame()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        VERIFY_SUCCEEDED(painter->BeginFrame(nullptr, 0));
        VERIFY_ARE_EQUAL(E_ILLEGAL_METHOD_CALL, painter->ScrollPixels({ 0, 0, 64, 64 }, 0, -10));
        FrameStats stats{};
        VERIFY_SUCCEEDED(painter->EndFrame(&stats));
        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 64, 64 }, 0, -10), L"allowed again after EndFrame");
    }

    void ExtendedPainterConformanceTests::ScrollPixelsPresentedAreaCountsAsUpload()
    {
        std::unique_ptr<IImmediatePainter> painter;
        ComPtr<ID3D11Device> device;
        if (!CreatePresentablePainter(painter, device)) return;
        auto swapChain = CreateCompositionSwapChain(device.Get(), c_size, c_size);
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(swapChain.Get()));
        Paint(*painter, PaintBlockPattern);

        VERIFY_SUCCEEDED(painter->ScrollPixels({ 0, 0, 64, 64 }, 0, -10));
        const RectF exposed{ 0, 54, 64, 64 };
        const auto stats = Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_red); }, &exposed, 1);
        const uint64_t upload = painter->GetResourceInventory().uploadBytesThisFrame;
        const uint64_t movedBytes = static_cast<uint64_t>(c_size) * (c_size - 10) * 4;
        Log::Comment(String().Format(L"damaged %llu px, upload %llu bytes, moved area %llu bytes", stats.damagedPixels, upload, movedBytes));
        VERIFY_IS_TRUE(upload >= movedBytes, L"the moved area is presented with the frame");
        VERIFY_IS_TRUE(upload >= stats.damagedPixels * 4, L"the damage is presented too");
        VERIFY_SUCCEEDED(painter->SetPresentationSwapChain(nullptr));
    }

    // ---- Stats ----

    void ExtendedPainterConformanceTests::ExtendedPrimitivesCountInStats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        GlyphRunFixture fixture;
        fixture.Create(L"Hello", 12);
        TestBitmap bitmap;
        bitmap.Create(1, 1);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(bitmap.source));
        const auto triangle = Triangle();
        const auto stats = Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushRoundedRectangleClip({ 0, 0, 64, 64 }, 4, 4);
            p.FillPath(triangle.Data(), c_red);
            p.StrokePath(triangle.Data(), 1, c_black);
            p.DrawGlyphRun(fixture.Run({ 4, 50 }), c_black);
            p.DrawBitmap(1, { 40, 40, 56, 56 }, nullptr, 1.0f);
            p.PopRoundedRectangleClip();
        });
        VERIFY_ARE_EQUAL(4u, stats.primitiveCount, L"FillPath, StrokePath, one glyph run, DrawBitmap; clips do not count");
    }
}
