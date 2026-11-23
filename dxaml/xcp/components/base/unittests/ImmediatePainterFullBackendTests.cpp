// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediatePainterFullBackendTests.h"
#include "ImmediatePainterTestHelpers.h"
#include <ImmediatePainter.h>
#include <dwrite_3.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <initializer_list>
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

        // GDI+ antialiasing, gradient sampling, and filtering differ slightly from D2D's; allow it a little more slack.
        int Tolerance(int base)
        {
            return CurrentMode() == Mode::ImmediateGdiPlus ? base + 4 : base;
        }

        int MaxChannelDifference(const Pixel& a, const Pixel& b)
        {
            return std::max({ std::abs(a.b - b.b), std::abs(a.g - b.g), std::abs(a.r - b.r), std::abs(a.a - b.a) });
        }

        // Largest difference between color channels: 0 for gray, large for saturated colors.
        int Chroma(const Pixel& p)
        {
            return std::max({ std::abs(p.r - p.g), std::abs(p.g - p.b), std::abs(p.r - p.b) });
        }

        void LogExpected(const wchar_t* label, uint32_t x, uint32_t y, const Pixel& actual, const ColorF& expected)
        {
            Log::Comment(String().Format(L"%s (%u,%u) = B%u G%u R%u A%u, expected premultiplied B%.0f G%.0f R%.0f A%.0f", label, x, y,
                actual.b, actual.g, actual.r, actual.a,
                expected.b * expected.a * 255, expected.g * expected.a * 255, expected.r * expected.a * 255, expected.a * 255));
        }

        // Counts pixels of the region [x0, x1) x [y0, y1) for which include(x, y) holds and that differ from color.
        template <typename Include>
        uint32_t CountMismatches(const Image& image, const ColorF& color, int tolerance, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1, Include&& include)
        {
            uint32_t mismatches = 0;
            for (uint32_t y = y0; y < y1; ++y)
            {
                for (uint32_t x = x0; x < x1; ++x)
                {
                    if (!include(x, y)) continue;
                    if (!IsColor(image.At(x, y), color, tolerance) && mismatches++ == 0)
                    {
                        LogExpected(L"first mismatch", x, y, image.At(x, y), color);
                    }
                }
            }
            return mismatches;
        }

        uint32_t CountMismatches(const Image& image, const ColorF& color, int tolerance, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
        {
            return CountMismatches(image, color, tolerance, x0, y0, x1, y1, [](uint32_t, uint32_t) { return true; });
        }

        // Counts pixels for which include(x, y) holds that differ from the reference image by more than tolerance.
        template <typename Include>
        uint32_t CountChanged(const Image& actual, const Image& reference, int tolerance, Include&& include)
        {
            uint32_t changed = 0;
            for (uint32_t y = 0; y < actual.height; ++y)
            {
                for (uint32_t x = 0; x < actual.width; ++x)
                {
                    if (!include(x, y)) continue;
                    if (MaxChannelDifference(actual.At(x, y), reference.At(x, y)) > tolerance && changed++ == 0)
                    {
                        LogPixel(L"first changed pixel", x, y, actual.At(x, y));
                        LogPixel(L"  reference", x, y, reference.At(x, y));
                    }
                }
            }
            return changed;
        }

        // ---- Gradients ----

        // Owns the stops of a LinearGradientDesc. Not copyable: desc points into stops.
        struct Gradient
        {
            std::vector<GradientStop> stops;
            LinearGradientDesc desc{};

            Gradient(PointF start, PointF end, std::initializer_list<GradientStop> list, float opacity = 1)
                : stops(list)
            {
                desc.start = start;
                desc.end = end;
                desc.stops = stops.data();
                desc.stopCount = static_cast<uint32_t>(stops.size());
                desc.opacity = opacity;
            }
            Gradient(const Gradient&) = delete;
            Gradient& operator=(const Gradient&) = delete;

            // Reference: clamp extend mode, straight interpolation of the (sRGB) stop values, opacity applied to alpha.
            // (x, y) is in the gradient's own DIP space.
            ColorF At(float x, float y) const
            {
                const float dx = desc.end.x - desc.start.x;
                const float dy = desc.end.y - desc.start.y;
                float t = ((x - desc.start.x) * dx + (y - desc.start.y) * dy) / (dx * dx + dy * dy);
                t = std::clamp(t, 0.0f, 1.0f);
                ColorF color = stops.front().color;
                if (t >= stops.back().offset)
                {
                    color = stops.back().color;
                }
                else
                {
                    for (size_t i = 0; i + 1 < stops.size(); ++i)
                    {
                        const auto& a = stops[i];
                        const auto& b = stops[i + 1];
                        if (t >= a.offset && t <= b.offset)
                        {
                            const float f = b.offset > a.offset ? (t - a.offset) / (b.offset - a.offset) : 0.0f;
                            color = { a.color.r + (b.color.r - a.color.r) * f, a.color.g + (b.color.g - a.color.g) * f,
                                a.color.b + (b.color.b - a.color.b) * f, a.color.a + (b.color.a - a.color.a) * f };
                            break;
                        }
                    }
                }
                color.a *= desc.opacity;
                return color;
            }
        };

        // Counts pixels of [x0, x1) x [y0, y1) (with include(x, y)) whose color differs from the reference gradient
        // sampled at the pixel center mapped back to gradient space by (pixel - translate) / scale.
        template <typename Include>
        uint32_t CountGradientMismatches(const Image& image, const Gradient& gradient, int tolerance, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1,
            Include&& include, float scale = 1, PointF translate = {})
        {
            uint32_t mismatches = 0;
            for (uint32_t y = y0; y < y1; ++y)
            {
                for (uint32_t x = x0; x < x1; ++x)
                {
                    if (!include(x, y)) continue;
                    const ColorF expected = gradient.At((x + 0.5f - translate.x) / scale, (y + 0.5f - translate.y) / scale);
                    if (!IsColor(image.At(x, y), expected, tolerance) && mismatches++ == 0)
                    {
                        LogExpected(L"first gradient mismatch", x, y, image.At(x, y), expected);
                    }
                }
            }
            Log::Comment(String().Format(L"gradient mismatches in [%u,%u)x[%u,%u): %u", x0, x1, y0, y1, mismatches));
            return mismatches;
        }

        uint32_t CountGradientMismatches(const Image& image, const Gradient& gradient, int tolerance, uint32_t x0, uint32_t y0, uint32_t x1, uint32_t y1)
        {
            return CountGradientMismatches(image, gradient, tolerance, x0, y0, x1, y1, [](uint32_t, uint32_t) { return true; });
        }

        const RectF c_fullRect{ 0, 0, static_cast<float>(c_size), static_cast<float>(c_size) };

        // ---- Paths ----

        struct PathBuilder
        {
            std::vector<PathVerb> verbs;
            std::vector<PointF> points;

            PathBuilder& Move(float x, float y) { verbs.push_back(PathVerb::MoveTo); points.push_back({ x, y }); return *this; }
            PathBuilder& Line(float x, float y) { verbs.push_back(PathVerb::LineTo); points.push_back({ x, y }); return *this; }
            PathBuilder& Close() { verbs.push_back(PathVerb::Close); return *this; }

            PathData Data() const
            {
                PathData data{};
                data.verbs = verbs.data();
                data.verbCount = static_cast<uint32_t>(verbs.size());
                data.points = points.data();
                data.pointCount = static_cast<uint32_t>(points.size());
                return data;
            }
        };

        // ---- Backdrop ----

        // 2px black / 2px white vertical stripes over the whole target.
        void PaintStripes(IImmediatePainter& p)
        {
            p.Clear(c_white);
            for (uint32_t x = 0; x < c_size; x += 4)
            {
                p.FillRectangle({ static_cast<float>(x), 0, static_cast<float>(x + 2), static_cast<float>(c_size) }, c_black);
            }
        }

        // Black left of edgeX, white right of it.
        void PaintEdge(IImmediatePainter& p, float edgeX)
        {
            p.Clear(c_white);
            p.FillRectangle({ 0, 0, edgeX, static_cast<float>(c_size) }, c_black);
        }

        bool IsGray(const Pixel& p, int low = 40, int high = 215)
        {
            return p.r > low && p.r < high;
        }

        // ---- Glyph runs ----

        ComPtr<IDWriteFactory> CreateDWriteFactory()
        {
            ComPtr<IDWriteFactory> factory;
            VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())));
            return factory;
        }

        // nullptr when the family is not installed.
        ComPtr<IDWriteFontFace> FindFontFace(const wchar_t* familyName, DWRITE_FONT_WEIGHT weight = DWRITE_FONT_WEIGHT_NORMAL)
        {
            auto factory = CreateDWriteFactory();
            ComPtr<IDWriteFontCollection> collection;
            VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection, FALSE));
            UINT32 familyIndex = 0;
            BOOL exists = FALSE;
            VERIFY_SUCCEEDED(collection->FindFamilyName(familyName, &familyIndex, &exists));
            if (!exists) return nullptr;
            ComPtr<IDWriteFontFamily> family;
            VERIFY_SUCCEEDED(collection->GetFontFamily(familyIndex, &family));
            ComPtr<IDWriteFont> font;
            VERIFY_SUCCEEDED(family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font));
            ComPtr<IDWriteFontFace> face;
            VERIFY_SUCCEEDED(font->CreateFontFace(&face));
            return face;
        }

        bool HasFontTable(IDWriteFontFace* face, UINT32 tag)
        {
            const void* data = nullptr;
            UINT32 size = 0;
            void* context = nullptr;
            BOOL exists = FALSE;
            if (FAILED(face->TryGetFontTable(tag, &data, &size, &context, &exists))) return false;
            if (exists) face->ReleaseFontTable(context);
            return !!exists;
        }

        struct GlyphFixture
        {
            ComPtr<IDWriteFontFace> fontFace;
            std::vector<uint16_t> glyphIndices;
            std::vector<float> advances;
            float emSize = 0;
            float ascent = 0;  // DIPs above the baseline
            float descent = 0; // DIPs below the baseline
            float width = 0;   // sum of advances

            // False when the font lacks a glyph for one of the code points.
            bool Create(IDWriteFontFace* face, std::initializer_list<UINT32> codePoints, float fontEmSize)
            {
                fontFace = face;
                emSize = fontEmSize;
                std::vector<UINT32> points(codePoints);
                glyphIndices.assign(points.size(), 0);
                VERIFY_SUCCEEDED(face->GetGlyphIndices(points.data(), static_cast<UINT32>(points.size()), glyphIndices.data()));
                if (std::find(glyphIndices.begin(), glyphIndices.end(), static_cast<uint16_t>(0)) != glyphIndices.end()) return false;

                DWRITE_FONT_METRICS fontMetrics{};
                face->GetMetrics(&fontMetrics);
                const float scale = emSize / fontMetrics.designUnitsPerEm;
                ascent = fontMetrics.ascent * scale;
                descent = fontMetrics.descent * scale;

                std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(glyphIndices.size());
                VERIFY_SUCCEEDED(face->GetDesignGlyphMetrics(glyphIndices.data(), static_cast<UINT32>(glyphIndices.size()), glyphMetrics.data(), FALSE));
                advances.clear();
                width = 0;
                for (const auto& metrics : glyphMetrics)
                {
                    advances.push_back(metrics.advanceWidth * scale);
                    width += advances.back();
                }
                Log::Comment(String().Format(L"glyph run: %u glyphs, width %.2f, ascent %.2f, descent %.2f",
                    static_cast<uint32_t>(glyphIndices.size()), width, ascent, descent));
                return true;
            }

            GlyphRunDesc Run(PointF baselineOrigin) const
            {
                GlyphRunDesc run{};
                run.fontFace = fontFace.Get();
                run.fontEmSize = emSize;
                run.glyphIndices = glyphIndices.data();
                run.glyphAdvances = advances.data();
                run.glyphCount = static_cast<uint32_t>(glyphIndices.size());
                run.baselineOrigin = baselineOrigin;
                return run;
            }
        };

        const std::initializer_list<UINT32> c_hello{ L'H', L'e', L'l', L'l', L'o' };

        struct InkCount
        {
            uint32_t inside = 0;
            uint32_t outside = 0;
        };

        template <typename IsInkFn>
        InkCount CountInk(const Image& image, float left, float top, float right, float bottom, IsInkFn&& isInk)
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
                    if (!isInk(image.At(x, y))) continue;
                    const int ix = static_cast<int>(x), iy = static_cast<int>(y);
                    const bool inside = ix >= l && ix < r && iy >= t && iy < b;
                    if (!inside && count.outside == 0) LogPixel(L"first ink outside the run box", x, y, image.At(x, y));
                    ++(inside ? count.inside : count.outside);
                }
            }
            Log::Comment(String().Format(L"ink inside [%d,%d)x[%d,%d): %u, outside: %u", l, r, t, b, count.inside, count.outside));
            return count;
        }

        // Draws "Hello" with the fixture on white and verifies ink lands at the glyph positions.
        void VerifyHelloRunDrawsInk(GlyphFixture& fixture)
        {
            constexpr uint32_t width = 160;
            auto painter = CreatePainter(width, c_size);
            if (!painter) return;
            const PointF origin{ 8, 40 };
            Paint(*painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.DrawGlyphRun(fixture.Run(origin), c_black);
            });
            const auto image = Read(*painter, width, c_size);
            const auto ink = CountInk(image, origin.x - 2, origin.y - fixture.ascent - 2, origin.x + fixture.width + 2, origin.y + fixture.descent + 2, IsInk);
            VERIFY_IS_TRUE(ink.inside > 40, L"the run produces ink");
            VERIFY_ARE_EQUAL(0u, ink.outside, L"no ink outside the run's advance/ascent/descent box");
        }
    }

    // ---- Gradient fills ----

    void FullBackendPainterConformanceTests::GradientFillHorizontalTwoStops()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const Gradient gradient({ 8, 0 }, { 56, 0 }, { { 0, c_red }, { 1, c_blue } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
        });
        const auto image = Read(*painter);
        LogPixel(L"start", 8, 32, image.At(8, 32));
        LogPixel(L"middle", 32, 32, image.At(32, 32));
        LogPixel(L"end", 55, 32, image.At(55, 32));
        VERIFY_IS_TRUE(IsColor(image.At(8, 32), c_red, Tolerance(8)), L"start point is ~stop 0");
        VERIFY_IS_TRUE(IsColor(image.At(55, 32), c_blue, Tolerance(8)), L"end point is ~stop 1");
        VERIFY_IS_TRUE(IsColor(image.At(31, 32), { 0.5f, 0, 0.5f, 1 }, Tolerance(6)) || IsColor(image.At(32, 32), { 0.5f, 0, 0.5f, 1 }, Tolerance(6)),
            L"middle is ~the average of the stops");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 0, 0, c_size, c_size), L"every pixel matches the reference gradient");
    }

    void FullBackendPainterConformanceTests::GradientFillVerticalTwoStops()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const Gradient gradient({ 0, 8 }, { 0, 56 }, { { 0, c_white }, { 1, c_black } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_red);
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
        });
        const auto image = Read(*painter);
        VERIFY_IS_TRUE(IsColor(image.At(32, 8), c_white, Tolerance(8)), L"start point is ~stop 0");
        VERIFY_IS_TRUE(IsColor(image.At(32, 55), c_black, Tolerance(8)), L"end point is ~stop 1");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 0, 0, c_size, c_size), L"every pixel matches the reference gradient");
    }

    void FullBackendPainterConformanceTests::GradientFillClampsBeyondEndPoints()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const Gradient gradient({ 24, 0 }, { 40, 0 }, { { 0, c_red }, { 1, c_green } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_red, Tolerance(2), 0, 0, 24, c_size), L"before the start point: stop 0 (clamp, no repeat)");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_green, Tolerance(2), 40, 0, c_size, c_size), L"after the end point: stop 1 (clamp, no repeat)");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 24, 0, 40, c_size), L"between the points: interpolated");
    }

    void FullBackendPainterConformanceTests::GradientFillThreeStops()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const Gradient gradient({ 0, 0 }, { 64, 0 }, { { 0, c_red }, { 0.25f, c_green }, { 1, c_blue } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
        });
        const auto image = Read(*painter);
        LogPixel(L"middle stop", 16, 32, image.At(16, 32));
        VERIFY_IS_TRUE(IsColor(image.At(16, 32), c_green, Tolerance(8)), L"the middle stop's color is reached at its offset");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 0, 0, c_size, c_size), L"every pixel matches the reference gradient");
    }

    void FullBackendPainterConformanceTests::GradientFillOpacityScalesColor()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Non-opaque stops are only checked in the clamped regions, where premultiplied and straight interpolation agree.
        const Gradient gradient({ 16, 0 }, { 48, 0 }, { { 0, c_red }, { 1, { 0, 0, 1, 0.5f } } }, 0.5f);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
        });
        const auto image = Read(*painter);
        LogPixel(L"stop 0 side", 4, 32, image.At(4, 32));
        LogPixel(L"stop 1 side", 60, 32, image.At(60, 32));
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, { 1, 0, 0, 0.5f }, Tolerance(2), 0, 0, 16, c_size), L"opaque stop 0 at opacity 0.5");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, { 0, 0, 1, 0.25f }, Tolerance(2), 48, 0, c_size, c_size), L"50% stop 1 at opacity 0.5");
    }

    void FullBackendPainterConformanceTests::GradientFillRoundedCornersAreAntialiased()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const Gradient gradient({ 8, 0 }, { 56, 0 }, { { 0, c_red }, { 1, c_blue } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangleGradient({ 8, 8, 56, 56 }, 16, 16, gradient.desc);
        });
        const auto image = Read(*painter);
        const int tolerance = Tolerance(3);
        VERIFY_IS_TRUE(IsColor(image.At(8, 8), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(9, 9), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(55, 55), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(8, 55), c_white, tolerance), L"corner outside the radius is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(4, 32), c_white, tolerance), L"outside the rect is untouched");
        VERIFY_IS_TRUE(IsColor(image.At(60, 32), c_white, tolerance), L"outside the rect is untouched");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 9, 24, 55, 40), L"the straight-edged band is the gradient");
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 24, 9, 40, 55), L"the straight-edged band is the gradient");

        // The arc crosses the diagonal near (12.7, 12.7); white has G = 255 and the gradient G = 0 there.
        bool foundPartial = false;
        for (uint32_t i = 8; i < 24 && !foundPartial; ++i)
        {
            const auto p = image.At(i, i);
            LogPixel(L"diagonal", i, i, p);
            foundPartial = p.g > 24 && p.g < 231;
        }
        VERIFY_IS_TRUE(foundPartial, L"expected an antialiased edge on the rounded corner");
    }

    void FullBackendPainterConformanceTests::GradientFillRespectsTransform()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Gradient points are in the shape's space: scale 2 + translate (16, 16) maps [0,16]^2 to [16,48]^2.
        const Gradient gradient({ 0, 0 }, { 16, 0 }, { { 0, c_red }, { 1, c_blue } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ 2, 0, 0, 2, 16, 16 });
            p.FillRoundedRectangleGradient({ 0, 0, 16, 16 }, 0, 0, gradient.desc);
            p.SetTransform({});
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 17, 17, 47, 47, [](uint32_t, uint32_t) { return true; }, 2, { 16, 16 }),
            L"the gradient follows the transform");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, Tolerance(2), 0, 0, c_size, c_size,
            [](uint32_t x, uint32_t y) { return x < 15 || x > 48 || y < 15 || y > 48; }), L"nothing outside the transformed rect");
    }

    void FullBackendPainterConformanceTests::GradientFillRespectsClipAndDamage()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        const Gradient gradient({ 0, 0 }, { 64, 0 }, { { 0, c_red }, { 1, c_blue } });
        const RectF damage{ 0, 0, 32, 64 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 64, 32 });
            p.FillRoundedRectangleGradient(c_fullRect, 0, 0, gradient.desc);
            p.PopAxisAlignedClip();
        }, &damage, 1);
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 0, 0, 32, 32), L"inside clip and damage: gradient");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, 2, 0, 32, 32, c_size), L"clipped out: untouched");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, 2, 32, 0, c_size, c_size), L"outside the damage: untouched");
    }

    void FullBackendPainterConformanceTests::GradientPathFillsTriangle()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        PathBuilder triangle;
        triangle.Move(8, 8).Line(56, 8).Line(8, 56).Close();
        const Gradient gradient({ 8, 0 }, { 56, 0 }, { { 0, c_red }, { 1, c_blue } });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillPathGradient(triangle.Data(), gradient.desc);
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountGradientMismatches(image, gradient, Tolerance(4), 9, 9, c_size, c_size,
            [](uint32_t x, uint32_t y) { return x + y + 2 <= 62; }), L"inside the triangle: gradient");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, Tolerance(2), 0, 0, c_size, c_size,
            [](uint32_t x, uint32_t y) { return x < 7 || y < 7 || x + y >= 66; }), L"outside the triangle: untouched");
    }

    void FullBackendPainterConformanceTests::GradientFillsCountInStats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        PathBuilder triangle;
        triangle.Move(8, 8).Line(56, 8).Line(8, 56).Close();
        const Gradient gradient({ 0, 0 }, { 64, 0 }, { { 0, c_red }, { 1, c_blue } });
        const auto stats = Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 48, 48 });
            p.FillRoundedRectangleGradient({ 4, 4, 60, 60 }, 4, 4, gradient.desc);
            p.FillPathGradient(triangle.Data(), gradient.desc);
            p.PopAxisAlignedClip();
        });
        VERIFY_ARE_EQUAL(2u, stats.primitiveCount, L"each gradient fill is one primitive; clips and Clear do not count");

        // Not specified by the contract (FrameStats counts "fills, strokes, and text runs"); logged for comparison.
        const auto effectStats = Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawRoundedRectangleShadow({ 16, 16, 48, 48 }, 4, 4, 8, { 0, 4 }, c_black);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 4, 4, 8);
        });
        Log::Comment(String().Format(L"shadow + backdrop blur primitiveCount: %u", effectStats.primitiveCount));
    }

    // ---- Drop shadows ----

    void FullBackendPainterConformanceTests::ShadowIsDarkerOnOffsetSide()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawRoundedRectangleShadow({ 20, 20, 44, 44 }, 4, 4, 8, { 0, 6 }, c_black);
        });
        const auto image = Read(*painter);
        const auto below = image.At(32, 47);
        const auto above = image.At(32, 17);
        LogPixel(L"below (offset side)", 32, 47, below);
        LogPixel(L"above", 32, 17, above);
        VERIFY_IS_TRUE(below.r < 160, L"a shadow is drawn outside the shape on the offset side");
        VERIFY_IS_TRUE(below.r + 60 < above.r, L"the offset side is darker than the opposite side");
        VERIFY_IS_TRUE(Chroma(below) <= 2 && below.a == 255, L"black shadow over white is gray and opaque");
    }

    void FullBackendPainterConformanceTests::ShadowFalloffIsMonotonicAndVanishesBeyondThreeSigma()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // blurRadius 8 => sigma 4; 3 sigma = 12 DIPs beyond the shape edge at 48 (and 16).
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawRoundedRectangleShadow({ 16, 16, 48, 48 }, 0, 0, 8, { 0, 0 }, c_black);
        });
        const auto image = Read(*painter);
        for (uint32_t x = 44; x < c_size; ++x)
        {
            LogPixel(L"row 32", x, 32, image.At(x, 32));
        }
        const auto edge = image.At(48, 32);
        VERIFY_IS_TRUE(edge.a > 50 && edge.a < 200, L"about half coverage just outside the edge (Q(0.5/sigma) ~ 0.45)");
        uint32_t increases = 0;
        for (uint32_t x = 49; x < c_size; ++x)
        {
            if (image.At(x, 32).a > image.At(x - 1, 32).a + 2) ++increases;
        }
        VERIFY_ARE_EQUAL(0u, increases, L"alpha never increases moving away from the shape");
        VERIFY_IS_TRUE(image.At(52, 32).a < edge.a, L"alpha falls off with distance");
        for (uint32_t x = 61; x < c_size; ++x)
        {
            VERIFY_IS_TRUE(image.At(x, 32).a <= 3, L"~no shadow beyond 3 sigma");
        }
        VERIFY_IS_TRUE(image.At(32, 2).a <= 3 && image.At(2, 32).a <= 3, L"~no shadow beyond 3 sigma");
        VERIFY_IS_TRUE(std::abs(image.At(15, 32).a - edge.a) <= Tolerance(12), L"no offset: left falloff mirrors the right");
        VERIFY_IS_TRUE(std::abs(image.At(32, 48).a - edge.a) <= Tolerance(12), L"no offset: bottom falloff mirrors the right");
    }

    void FullBackendPainterConformanceTests::ShadowKnockoutLeavesShapeAreaUntouched()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawRoundedRectangleShadow({ 20, 20, 44, 44 }, 4, 4, 8, { 0, 4 }, c_black);
        });
        const auto image = Read(*painter);
        const auto outside = image.At(32, 46);
        LogPixel(L"outside", 32, 46, outside);
        VERIFY_IS_TRUE(outside.a > 100, L"the shadow is drawn outside the shape");
        // The shape minus its corner squares, inset 1px for antialiasing.
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_transparent, 2, 21, 21, 43, 43, [](uint32_t x, uint32_t y)
        {
            const bool cornerX = x < 24 || x >= 40;
            const bool cornerY = y < 24 || y >= 40;
            return !(cornerX && cornerY);
        }), L"knockout: the shadow is not drawn under the shape, and the shape itself is not drawn");
    }

    void FullBackendPainterConformanceTests::ShadowColorAndAlphaAreRespected()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const RectF shape{ 20, 20, 44, 44 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawRoundedRectangleShadow(shape, 4, 4, 8, { 0, 6 }, c_red);
        });
        const auto opaque = Read(*painter);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            p.DrawRoundedRectangleShadow(shape, 4, 4, 8, { 0, 6 }, { 1, 0, 0, 0.5f });
        });
        const auto half = Read(*painter);
        const auto a = opaque.At(32, 47);
        const auto b = half.At(32, 47);
        LogPixel(L"alpha 1", 32, 47, a);
        LogPixel(L"alpha 0.5", 32, 47, b);
        VERIFY_IS_TRUE(a.a > 100, L"the shadow is drawn");
        VERIFY_IS_TRUE(std::abs(a.r - a.a) <= 2 && a.g <= 2 && a.b <= 2, L"the shadow has the requested color (premultiplied red)");
        VERIFY_IS_TRUE(std::abs(b.a * 2 - a.a) <= Tolerance(6), L"color alpha scales the shadow");
        VERIFY_IS_TRUE(std::abs(b.r - b.a) <= 2 && b.g <= 2 && b.b <= 2, L"the shadow has the requested color (premultiplied red)");
    }

    void FullBackendPainterConformanceTests::ShadowZeroBlurIsHardEdgeOffsetShape()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawRoundedRectangleShadow({ 16, 16, 40, 40 }, 0, 0, 0, { 8, 8 }, c_black);
        });
        const auto image = Read(*painter);
        // Shadow shape: [24, 48) x [24, 48), minus the knocked-out shape [16, 40) x [16, 40).
        const int tolerance = Tolerance(3);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_black, tolerance, 24, 24, 48, 48,
            [](uint32_t x, uint32_t y) { return x >= 40 || y >= 40; }), L"the offset shape outside the knockout is solid shadow");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, tolerance, 0, 0, c_size, c_size,
            [](uint32_t x, uint32_t y) { return x >= 48 || y >= 48 || x < 24 || y < 24 || (x < 40 && y < 40); }),
            L"hard edge: nothing outside the offset shape, nothing under the shape");
    }

    void FullBackendPainterConformanceTests::ShadowRespectsTransformAndDamage()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        // Translate (8, 8) maps the shape [12, 36]^2 to [20, 44]^2. blurRadius 4 => sigma 2. Damage: left half only.
        const RectF damage{ 0, 0, 32, 64 };
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ 1, 0, 0, 1, 8, 8 });
            p.DrawRoundedRectangleShadow({ 12, 12, 36, 36 }, 0, 0, 4, { 0, 0 }, c_black);
            p.SetTransform({});
        }, &damage, 1);
        const auto image = Read(*painter);
        LogPixel(L"left of translated shape", 19, 32, image.At(19, 32));
        LogPixel(L"left of untranslated shape", 11, 24, image.At(11, 24));
        VERIFY_IS_TRUE(image.At(19, 32).r < 220, L"shadow next to the translated shape");
        VERIFY_IS_TRUE(image.At(11, 24).r > 240, L"no shadow next to the untranslated shape");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, 2, 32, 0, c_size, c_size), L"outside the damage: untouched");
    }

    // ---- Backdrop blur ----

    void FullBackendPainterConformanceTests::BlurBackdropSoftensEdgeInsideRect()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { PaintEdge(p, 32); });
        const auto before = Read(*painter);
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintEdge(p, 32);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 0, 0, 8);
        });
        const auto image = Read(*painter);
        for (uint32_t x = 24; x < 40; ++x)
        {
            LogPixel(L"row 32", x, 32, image.At(x, 32));
        }
        VERIFY_IS_TRUE(IsGray(image.At(31, 32)), L"the black side of the edge is blurred");
        VERIFY_IS_TRUE(IsGray(image.At(32, 32)), L"the white side of the edge is blurred");
        VERIFY_IS_TRUE(image.At(36, 32).r > image.At(27, 32).r + 100, L"the edge becomes a ramp");
        uint32_t decreases = 0;
        for (uint32_t x = 17; x < 47; ++x)
        {
            if (image.At(x + 1, 32).r + 2 < image.At(x, 32).r) ++decreases;
        }
        VERIFY_ARE_EQUAL(0u, decreases, L"the ramp is monotonic");
        VERIFY_IS_TRUE(image.At(32, 32).a == 255 && image.At(31, 32).a == 255, L"opaque content stays opaque");
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, 1, [](uint32_t x, uint32_t y) { return x < 16 || x >= 48 || y < 16 || y >= 48; }),
            L"pixels outside the rect are unchanged");
    }

    void FullBackendPainterConformanceTests::BlurBackdropLeavesPixelsOutsideRoundedRectUnchanged()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, PaintStripes);
        const auto before = Read(*painter);
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintStripes(p);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 12, 12, 8);
        });
        const auto image = Read(*painter);
        LogPixel(L"center", 32, 32, image.At(32, 32));
        VERIFY_IS_TRUE(IsGray(image.At(32, 32), 100, 156) && IsGray(image.At(33, 32), 100, 156), L"stripes blur to ~50% gray inside");
        VERIFY_IS_TRUE(IsGray(image.At(20, 32), 100, 156), L"stripes blur to ~50% gray near the straight edge");
        // Corner circle centers are 12 in from each corner; pixels further than radius + 1 from them are outside.
        const auto outsideRoundedRect = [](uint32_t x, uint32_t y)
        {
            if (x < 16 || x >= 48 || y < 16 || y >= 48) return true;
            const float cx = std::clamp(x + 0.5f, 28.0f, 36.0f);
            const float cy = std::clamp(y + 0.5f, 28.0f, 36.0f);
            const float dx = x + 0.5f - cx, dy = y + 0.5f - cy;
            return std::sqrt(dx * dx + dy * dy) > 13.0f;
        };
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, 1, outsideRoundedRect), L"pixels outside the rounded rect are unchanged");
    }

    void FullBackendPainterConformanceTests::BlurBackdropZeroRadiusIsNoOp()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, PaintStripes);
        const auto before = Read(*painter);
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintStripes(p);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 4, 4, 0);
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, 1, [](uint32_t, uint32_t) { return true; }), L"blurRadius 0 changes nothing");
    }

    void FullBackendPainterConformanceTests::BlurBackdropKeepsUniformRegionUniform()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const ColorF color{ 0.2f, 0.4f, 0.6f, 1 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(color);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 8, 8, 8);
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, color, 3, 0, 0, c_size, c_size), L"blurring a uniform color keeps it, including near the rect edges");
    }

    void FullBackendPainterConformanceTests::BlurBackdropRespectsTranslation()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Translate (16, 0) maps the blur rect [16, 40) x [16, 48) to [32, 56) x [16, 48), which contains the edge at 40.
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintEdge(p, 40);
            p.SetTransform({ 1, 0, 0, 1, 16, 0 });
            p.BlurBackdrop({ 16, 16, 40, 48 }, 0, 0, 8);
            p.SetTransform({});
        });
        const auto image = Read(*painter);
        LogPixel(L"edge, black side", 39, 32, image.At(39, 32));
        LogPixel(L"edge, white side", 40, 32, image.At(40, 32));
        VERIFY_IS_TRUE(IsGray(image.At(39, 32)) && IsGray(image.At(40, 32)), L"the edge inside the translated rect is blurred");
        VERIFY_IS_TRUE(IsColor(image.At(30, 32), c_black, 1), L"left of the translated rect is unchanged");
        VERIFY_IS_TRUE(IsColor(image.At(58, 32), c_white, 1), L"right of the translated rect is unchanged");
    }

    void FullBackendPainterConformanceTests::BlurBackdropRespectsDamage()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { PaintEdge(p, 32); });
        const auto before = Read(*painter);
        // Only the top half is damaged; the blur reads the (previous frame's) pixels below it but writes only the damage.
        const RectF damage{ 0, 0, 64, 32 };
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintEdge(p, 32);
            p.BlurBackdrop({ 16, 16, 48, 48 }, 0, 0, 8);
        }, &damage, 1);
        const auto image = Read(*painter);
        LogPixel(L"damaged edge", 31, 24, image.At(31, 24));
        VERIFY_IS_TRUE(IsGray(image.At(31, 24)) && IsGray(image.At(32, 24)), L"the edge inside the damage is blurred");
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, 1, [](uint32_t, uint32_t y) { return y >= 32; }), L"outside the damage: unchanged");
    }

    // ---- Glyph-run hints and fallback fonts ----

    void FullBackendPainterConformanceTests::ColorGlyphRunDrawsColorLayers()
    {
        auto face = FindFontFace(L"Segoe UI Emoji");
        if (!face)
        {
            Log::Result(TestResults::Skipped, L"Segoe UI Emoji is not installed.");
            return;
        }
        GlyphFixture fixture;
        VERIFY_IS_TRUE(fixture.Create(face.Get(), { 0x1F600 }, 48), L"Segoe UI Emoji has U+1F600");
        constexpr uint32_t size = 96;
        auto painter = CreatePainter(size, size);
        if (!painter) return;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            auto run = fixture.Run({ 16, 64 });
            run.useColorFont = true;
            p.DrawGlyphRun(run, c_black);
        });
        const auto image = Read(*painter, size, size);
        uint32_t colored = 0, ink = 0;
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                const auto p = image.At(x, y);
                if (IsInk(p)) ++ink;
                if (Chroma(p) > 64 && colored++ == 0) LogPixel(L"first colored pixel", x, y, p);
            }
        }
        Log::Comment(String().Format(L"ink %u, colored %u", ink, colored));
        VERIFY_IS_TRUE(ink > 100, L"the emoji draws");
        VERIFY_IS_TRUE(colored > 100, L"useColorFont draws the COLR/CPAL layers (saturated pixels), not a monochrome outline");
    }

    void FullBackendPainterConformanceTests::ColorGlyphRunIsMonochromeWhenColorFontDisabled()
    {
        auto face = FindFontFace(L"Segoe UI Emoji");
        if (!face)
        {
            Log::Result(TestResults::Skipped, L"Segoe UI Emoji is not installed.");
            return;
        }
        GlyphFixture fixture;
        VERIFY_IS_TRUE(fixture.Create(face.Get(), { 0x1F600 }, 48), L"Segoe UI Emoji has U+1F600");
        constexpr uint32_t size = 96;
        auto painter = CreatePainter(size, size);
        if (!painter) return;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            auto run = fixture.Run({ 16, 64 });
            run.useColorFont = false;
            p.DrawGlyphRun(run, c_black);
        });
        const auto image = Read(*painter, size, size);
        uint32_t colored = 0, ink = 0;
        for (uint32_t y = 0; y < size; ++y)
        {
            for (uint32_t x = 0; x < size; ++x)
            {
                const auto p = image.At(x, y);
                if (IsInk(p)) ++ink;
                if (Chroma(p) > 16 && colored++ == 0) LogPixel(L"first colored pixel", x, y, p);
            }
        }
        Log::Comment(String().Format(L"ink %u, colored %u", ink, colored));
        VERIFY_IS_TRUE(ink > 100, L"the monochrome outline draws");
        VERIFY_ARE_EQUAL(0u, colored, L"useColorFont=false draws only the run color (black => gray coverage)");
    }

    void FullBackendPainterConformanceTests::ClearTypeGlyphRunOnOpaqueBackgroundDrawsInk()
    {
        auto face = FindFontFace(L"Segoe UI");
        VERIFY_IS_NOT_NULL(face.Get(), L"Segoe UI is installed");
        GlyphFixture fixture;
        VERIFY_IS_TRUE(fixture.Create(face.Get(), c_hello, 20));
        constexpr uint32_t width = 128;
        auto painter = CreatePainter(width, c_size);
        if (!painter) return;
        const PointF origin{ 8, 40 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            auto run = fixture.Run(origin);
            run.clearType = true;
            p.DrawGlyphRun(run, c_black);
        });
        const auto image = Read(*painter, width, c_size);
        const auto ink = CountInk(image, origin.x - 2, origin.y - fixture.ascent - 2, origin.x + fixture.width + 2, origin.y + fixture.descent + 2, IsInk);
        VERIFY_IS_TRUE(ink.inside > 40, L"the ClearType run produces ink");
        VERIFY_ARE_EQUAL(0u, ink.outside, L"no ink outside the run box");
        VERIFY_IS_TRUE(image.At(0, 0).a == 255, L"the target stays opaque");
    }

    void FullBackendPainterConformanceTests::ClearTypeGlyphRunOnTransparentTargetFallsBackToGrayscale()
    {
        auto face = FindFontFace(L"Segoe UI");
        VERIFY_IS_NOT_NULL(face.Get(), L"Segoe UI is installed");
        GlyphFixture fixture;
        VERIFY_IS_TRUE(fixture.Create(face.Get(), c_hello, 20));
        constexpr uint32_t width = 128;
        auto painter = CreatePainter(width, c_size);
        if (!painter) return;
        const PointF origin{ 8, 40 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_transparent);
            auto run = fixture.Run(origin);
            run.clearType = true;
            p.DrawGlyphRun(run, c_white);
        });
        const auto image = Read(*painter, width, c_size);
        const auto ink = CountInk(image, origin.x - 2, origin.y - fixture.ascent - 2, origin.x + fixture.width + 2, origin.y + fixture.descent + 2,
            [](const Pixel& p) { return p.a > 64; });
        VERIFY_IS_TRUE(ink.inside > 40, L"the run produces ink on a transparent target");
        VERIFY_ARE_EQUAL(0u, ink.outside, L"no ink outside the run box");
        uint32_t invalid = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < width; ++x)
            {
                const auto p = image.At(x, y);
                // White grayscale coverage on transparent: premultiplied R == G == B == A.
                const bool ok = Chroma(p) <= 8 && p.r <= p.a + 2 && p.g <= p.a + 2 && p.b <= p.a + 2;
                if (!ok && invalid++ == 0) LogPixel(L"first non-grayscale pixel", x, y, p);
            }
        }
        VERIFY_ARE_EQUAL(0u, invalid, L"grayscale antialiasing (no subpixel color fringes) and valid premultiplied pixels");
    }

    void FullBackendPainterConformanceTests::GlyphRunWithCffFontDrawsInk()
    {
        auto factory = CreateDWriteFactory();
        ComPtr<IDWriteFontCollection> collection;
        VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection, FALSE));
        GlyphFixture fixture;
        bool found = false;
        const UINT32 familyCount = collection->GetFontFamilyCount();
        for (UINT32 i = 0; i < familyCount && !found; ++i)
        {
            ComPtr<IDWriteFontFamily> family;
            if (FAILED(collection->GetFontFamily(i, &family))) continue;
            const UINT32 fontCount = family->GetFontCount();
            for (UINT32 j = 0; j < fontCount && !found; ++j)
            {
                ComPtr<IDWriteFont> font;
                ComPtr<IDWriteFontFace> face;
                if (FAILED(family->GetFont(j, &font)) || font->IsSymbolFont() || font->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE) continue;
                if (FAILED(font->CreateFontFace(&face))) continue;
                // CFF-flavored OpenType (.otf), including ones inside collections, which report OPENTYPE_COLLECTION.
                const bool cff = face->GetType() == DWRITE_FONT_FACE_TYPE_CFF ||
                    HasFontTable(face.Get(), DWRITE_MAKE_OPENTYPE_TAG('C', 'F', 'F', ' ')) ||
                    HasFontTable(face.Get(), DWRITE_MAKE_OPENTYPE_TAG('C', 'F', 'F', '2'));
                if (!cff) continue;
                if (!fixture.Create(face.Get(), c_hello, 20)) continue;
                ComPtr<IDWriteLocalizedStrings> names;
                wchar_t name[128] = L"?";
                if (SUCCEEDED(family->GetFamilyNames(&names)) && names->GetCount() > 0)
                {
                    names->GetString(0, name, ARRAYSIZE(name));
                }
                Log::Comment(String().Format(L"CFF font: %s (font %u)", name, j));
                found = true;
            }
        }
        if (!found)
        {
            Log::Result(TestResults::Skipped, L"No CFF (OpenType PostScript outline) font with Latin glyphs is installed.");
            return;
        }
        VerifyHelloRunDrawsInk(fixture);
    }

    void FullBackendPainterConformanceTests::GlyphRunWithVariableFontDrawsInk()
    {
        ComPtr<IDWriteFontFace> face;
        for (const wchar_t* name : { L"Segoe UI Variable Text", L"Segoe UI Variable", L"Segoe UI Variable Display", L"Segoe UI Variable Small" })
        {
            // Semibold is a named instance away from the default axis values, which GDI+ cannot select by itself.
            face = FindFontFace(name, DWRITE_FONT_WEIGHT_SEMI_BOLD);
            if (face)
            {
                Log::Comment(String().Format(L"variable font family: %s", name));
                break;
            }
        }
        ComPtr<IDWriteFontFace5> face5;
        if (!face || FAILED(face.As(&face5)) || !face5->HasVariations())
        {
            Log::Result(TestResults::Skipped, L"Segoe UI Variable (a variable font) is not installed.");
            return;
        }
        GlyphFixture fixture;
        VERIFY_IS_TRUE(fixture.Create(face.Get(), c_hello, 20));
        VerifyHelloRunDrawsInk(fixture);
    }

    // ---- text: InvertRectangle (CompositeMode DestInvert caret) ----

    void FullBackendPainterConformanceTests::InvertRectangleInvertsOpaquePixels()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const ColorF background{ 0.2f, 0.4f, 0.6f, 1 };
        Paint(*painter, [&](IImmediatePainter& p) { p.Clear(background); });
        const auto before = Read(*painter);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(background);
            p.InvertRectangle({ 16, 16, 48, 48 }, c_white);
        });
        const auto image = Read(*painter);
        LogPixel(L"inside", 32, 32, image.At(32, 32));
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, { 0.8f, 0.6f, 0.4f, 1 }, Tolerance(2), 16, 16, 48, 48), L"inside: 1 - destination, opaque");
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, 1, [](uint32_t x, uint32_t y) { return x < 16 || x >= 48 || y < 16 || y >= 48; }),
            L"outside: unchanged");
    }

    void FullBackendPainterConformanceTests::InvertRectangleTwiceRestoresPixels()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, PaintStripes);
        const auto before = Read(*painter);
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintStripes(p);
            p.InvertRectangle({ 10, 10, 50, 50 }, c_white);
            p.InvertRectangle({ 10, 10, 50, 50 }, c_white);
        });
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountChanged(image, before, Tolerance(2), [](uint32_t, uint32_t) { return true; }), L"invert is an involution");
    }

    void FullBackendPainterConformanceTests::InvertRectangleWithTranslucentSourceBlends()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // S = white at 50%: O = 0.5 * (1 - D) + 0.5 * D = 0.5 on both sides of the edge.
        Paint(*painter, [](IImmediatePainter& p)
        {
            PaintEdge(p, 32);
            p.InvertRectangle({ 16, 16, 48, 48 }, { 1, 1, 1, 0.5f });
        });
        const auto image = Read(*painter);
        LogPixel(L"black side", 24, 32, image.At(24, 32));
        LogPixel(L"white side", 40, 32, image.At(40, 32));
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, { 0.5f, 0.5f, 0.5f, 1 }, Tolerance(3), 16, 16, 48, 48), L"inside: mid gray, opaque");
        VERIFY_IS_TRUE(IsColor(image.At(8, 32), c_black, 1) && IsColor(image.At(56, 32), c_white, 1), L"outside: unchanged");
    }

    void FullBackendPainterConformanceTests::InvertRectangleAntialiasesFractionalEdges()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_black);
            p.InvertRectangle({ 16.5f, 16, 47.5f, 48 }, c_white);
        });
        const auto image = Read(*painter);
        LogPixel(L"left edge", 16, 32, image.At(16, 32));
        LogPixel(L"right edge", 47, 32, image.At(47, 32));
        VERIFY_IS_TRUE(IsGray(image.At(16, 32), 90, 166) && IsGray(image.At(47, 32), 90, 166), L"half-covered edge pixels are half inverted");
        VERIFY_IS_TRUE(IsColor(image.At(32, 32), c_white, 1), L"fully covered pixels are inverted");
        VERIFY_IS_TRUE(IsColor(image.At(15, 32), c_black, 1) && IsColor(image.At(48, 32), c_black, 1), L"uncovered pixels are unchanged");
    }

    void FullBackendPainterConformanceTests::InvertRectangleRespectsTransformClipAndDamage()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        Paint(*painter, [](IImmediatePainter& p) { p.Clear(c_white); });
        // Translate (8, 8) maps [8, 40)^2 to [16, 48)^2; the clip keeps y < 32; the damage keeps x < 32.
        const RectF damage{ 0, 0, 32, 64 };
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 64, 32 });
            p.SetTransform({ 1, 0, 0, 1, 8, 8 });
            p.InvertRectangle({ 8, 8, 40, 40 }, c_white);
            p.SetTransform({});
            p.PopAxisAlignedClip();
        }, &damage, 1);
        const auto image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_black, Tolerance(2), 16, 16, 32, 32), L"inside transform, clip and damage: inverted");
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_white, 2, 0, 0, c_size, c_size,
            [](uint32_t x, uint32_t y) { return x < 16 || x >= 32 || y < 16 || y >= 32; }), L"elsewhere: untouched");
    }

    void FullBackendPainterConformanceTests::InvertRectangleCountsInStats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const auto stats = Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.InvertRectangle({ 16, 16, 18, 48 }, c_white);
        });
        VERIFY_ARE_EQUAL(1u, stats.primitiveCount, L"one invert is one primitive");
    }

    // ---- imaging ----

    namespace
    {
        // A width x height premultiplied BGRA image with `padding` unused pixels per row: left half red, right half blue,
        // padding filled with green (must never be drawn).
        std::vector<uint8_t> MakeSplitBitmap(uint32_t width, uint32_t height, uint32_t padding, const ColorF& left, const ColorF& right)
        {
            const uint32_t stride = (width + padding) * 4;
            std::vector<uint8_t> bytes(static_cast<size_t>(stride) * height);
            for (uint32_t y = 0; y < height; ++y)
            {
                for (uint32_t x = 0; x < width + padding; ++x)
                {
                    const ColorF& c = x >= width ? c_green : (x < width / 2 ? left : right);
                    uint8_t* p = &bytes[static_cast<size_t>(y) * stride + x * 4];
                    p[0] = static_cast<uint8_t>(c.b * c.a * 255);
                    p[1] = static_cast<uint8_t>(c.g * c.a * 255);
                    p[2] = static_cast<uint8_t>(c.r * c.a * 255);
                    p[3] = static_cast<uint8_t>(c.a * 255);
                }
            }
            return bytes;
        }

        BitmapSource MakeSource(uint64_t key, uint64_t generation, uint32_t width, uint32_t height, uint32_t padding, const std::vector<uint8_t>& bytes, bool borrow)
        {
            BitmapSource source;
            source.key = key;
            source.generation = generation;
            source.width = width;
            source.height = height;
            source.stride = (width + padding) * 4;
            source.pixels = bytes.data();
            source.borrowPixels = borrow;
            return source;
        }
    }

    void FullBackendPainterConformanceTests::BorrowedBitmapDrawsLikeCopiedBitmap()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const std::vector<uint8_t> copied = MakeSplitBitmap(16, 16, 0, c_red, c_blue);
        const std::vector<uint8_t> borrowed = MakeSplitBitmap(16, 16, 3, c_red, c_blue);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(1, 1, 16, 16, 0, copied, false)));
        VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(2, 1, 16, 16, 3, borrowed, true)));
        Paint(*painter, [](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(1, { 0, 0, 32, 32 }, nullptr, 1.0f);
            p.DrawBitmap(2, { 32, 32, 64, 64 }, nullptr, 1.0f);
        });
        const Image image = Read(*painter);
        // Same pixels in both quadrants (bilinear seams included), no padding (green) leaked into the borrowed draw.
        uint32_t differences = 0;
        for (uint32_t y = 0; y < 32; ++y)
        {
            for (uint32_t x = 0; x < 32; ++x)
            {
                if (MaxChannelDifference(image.At(x, y), image.At(x + 32, y + 32)) > 2 && differences++ == 0)
                {
                    LogPixel(L"copied", x, y, image.At(x, y));
                    LogPixel(L"borrowed", x + 32, y + 32, image.At(x + 32, y + 32));
                }
            }
        }
        VERIFY_ARE_EQUAL(0u, differences);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_red, Tolerance(2), 34, 34, 46, 62));
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_blue, Tolerance(2), 50, 34, 62, 62));
        painter->ReleaseBitmap(1);
        painter->ReleaseBitmap(2);
    }

    void FullBackendPainterConformanceTests::BorrowedBitmapSourceRectHonorsStride()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const std::vector<uint8_t> borrowed = MakeSplitBitmap(16, 8, 5, c_red, c_blue);
        VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(7, 1, 16, 8, 5, borrowed, true)));
        // Two nine-grid-like cells from one bitmap: the right (blue) half, then the left (red) half.
        const RectF right{ 8, 0, 16, 8 };
        const RectF left{ 0, 0, 8, 8 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawBitmap(7, { 0, 0, 32, 64 }, &right, 1.0f);
            p.DrawBitmap(7, { 32, 0, 64, 64 }, &left, 1.0f);
        });
        const Image image = Read(*painter);
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_blue, Tolerance(2), 2, 2, 30, 62));
        VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_red, Tolerance(2), 34, 2, 62, 62));
        painter->ReleaseBitmap(7);
    }

    void FullBackendPainterConformanceTests::BitmapNewGenerationRereadsPixels()
    {
        for (const bool borrow : { false, true })
        {
            Log::Comment(borrow ? L"borrowed" : L"copied");
            auto painter = CreatePainter();
            if (!painter) return;
            std::vector<uint8_t> bytes = MakeSplitBitmap(8, 8, 0, c_red, c_red);
            VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(3, 1, 8, 8, 0, bytes, borrow)));
            Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.DrawBitmap(3, { 0, 0, 64, 64 }, nullptr, 1.0f);
            });
            VERIFY_ARE_EQUAL(0u, CountMismatches(Read(*painter), c_red, Tolerance(2), 0, 0, 64, 64));

            // Same key, new generation: an animated GIF frame or a WriteableBitmap.Invalidate in the same buffer.
            bytes = MakeSplitBitmap(8, 8, 0, c_blue, c_blue);
            VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(3, 2, 8, 8, 0, bytes, borrow)));
            Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.DrawBitmap(3, { 0, 0, 64, 64 }, nullptr, 1.0f);
            });
            VERIFY_ARE_EQUAL(0u, CountMismatches(Read(*painter), c_blue, Tolerance(2), 0, 0, 64, 64));

            // New generation with a new size (a re-decode at another DecodePixelWidth).
            const std::vector<uint8_t> larger = MakeSplitBitmap(16, 4, 0, c_red, c_blue);
            VERIFY_SUCCEEDED(painter->EnsureBitmap(MakeSource(3, 3, 16, 4, 0, larger, borrow)));
            Paint(*painter, [](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.DrawBitmap(3, { 0, 0, 64, 64 }, nullptr, 1.0f);
            });
            const Image image = Read(*painter);
            VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_red, Tolerance(2), 0, 0, 28, 64));
            VERIFY_ARE_EQUAL(0u, CountMismatches(image, c_blue, Tolerance(2), 36, 0, 64, 64));
            painter->ReleaseBitmap(3);
        }
    }
}
