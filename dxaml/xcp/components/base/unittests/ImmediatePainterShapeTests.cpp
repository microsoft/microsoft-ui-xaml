// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediatePainterShapeTests.h"
#include "ImmediatePainterTestHelpers.h"
#include <ImmediatePainter.h>
#include <dwrite_3.h>
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <functional>
#include <initializer_list>
#include <string>
#include <utility>
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
        using Points = std::initializer_list<std::pair<uint32_t, uint32_t>>;

        int Tolerance(int base)
        {
            return CurrentMode() == Mode::ImmediateGdiPlus ? base + 4 : base;
        }

        int MaxChannelDifference(const Pixel& a, const Pixel& b)
        {
            return std::max({ std::abs(a.b - b.b), std::abs(a.g - b.g), std::abs(a.r - b.r), std::abs(a.a - b.a) });
        }

        void ExpectColor(const Image& image, uint32_t x, uint32_t y, const ColorF& expected, int tolerance, const wchar_t* label)
        {
            const Pixel p = image.At(x, y);
            Log::Comment(String().Format(L"%s (%u,%u) = B%u G%u R%u A%u, expected B%.0f G%.0f R%.0f A%.0f", label, x, y, p.b, p.g, p.r, p.a,
                expected.b * expected.a * 255, expected.g * expected.a * 255, expected.r * expected.a * 255, expected.a * 255));
            VERIFY_IS_TRUE(IsColor(p, expected, tolerance), label);
        }

        // Black ink on white: the red channel measures coverage (0 = fully inked, 255 = untouched).
        void ExpectInk(const Image& image, uint32_t x, uint32_t y, const wchar_t* label, int maxRed = 64)
        {
            const Pixel p = image.At(x, y);
            LogPixel(label, x, y, p);
            VERIFY_IS_TRUE(p.r <= maxRed, String().Format(L"%s: expected ink at (%u,%u)", label, x, y));
        }

        void ExpectBlank(const Image& image, uint32_t x, uint32_t y, const wchar_t* label, int minRed = 200)
        {
            const Pixel p = image.At(x, y);
            LogPixel(label, x, y, p);
            VERIFY_IS_TRUE(p.r >= minRed, String().Format(L"%s: expected no ink at (%u,%u)", label, x, y));
        }

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

        BrushDesc Solid(const ColorF& color, float opacity = 1)
        {
            BrushDesc brush{};
            brush.kind = BrushKind::Solid;
            brush.color = color;
            brush.opacity = opacity;
            return brush;
        }

        const BrushDesc c_blackBrush = Solid(c_black);

        // Owns its stops. Not copyable: desc points into stops.
        struct Brush
        {
            std::vector<GradientStop> stops;
            BrushDesc desc{};

            explicit Brush(std::vector<GradientStop> list) : stops(std::move(list))
            {
                desc.stops = stops.data();
                desc.stopCount = static_cast<uint32_t>(stops.size());
            }
            Brush(const Brush&) = delete;
            Brush& operator=(const Brush&) = delete;

            static std::vector<GradientStop> RedBlue() { return { { 0, c_red }, { 1, c_blue } }; }
        };

        struct LinearBrush : Brush
        {
            LinearBrush(PointF start, PointF end, std::vector<GradientStop> list = RedBlue(), GradientSpread spread = GradientSpread::Pad)
                : Brush(std::move(list))
            {
                desc.kind = BrushKind::Linear;
                desc.start = start;
                desc.end = end;
                desc.spread = spread;
            }
        };

        struct RadialBrush : Brush
        {
            RadialBrush(PointF center, PointF origin, float radiusX, float radiusY, GradientSpread spread = GradientSpread::Pad)
                : Brush(RedBlue())
            {
                desc.kind = BrushKind::Radial;
                desc.center = center;
                desc.origin = origin;
                desc.radiusX = radiusX;
                desc.radiusY = radiusY;
                desc.spread = spread;
            }
        };

        float Spread(float t, GradientSpread spread)
        {
            switch (spread)
            {
                case GradientSpread::Repeat: return t - std::floor(t);
                case GradientSpread::Reflect:
                {
                    const float m = std::fmod(std::abs(t), 2.0f);
                    return m > 1 ? 2 - m : m;
                }
                default: return std::clamp(t, 0.0f, 1.0f);
            }
        }

        ColorF RedBlueAt(float t)
        {
            return { 1 - t, 0, t, 1 };
        }

        float SRgbToLinear(float c) { return c <= 0.04045f ? c / 12.92f : std::pow((c + 0.055f) / 1.055f, 2.4f); }
        float LinearToSRgb(float c) { return c <= 0.0031308f ? c * 12.92f : 1.055f * std::pow(c, 1 / 2.4f) - 0.055f; }

        const RectF c_full{ 0, 0, static_cast<float>(c_size), static_cast<float>(c_size) };

        Image PaintStroke(IImmediatePainter& painter, const PathBuilder& path, float thickness, const StrokeStyleDesc* style, const BrushDesc& brush = c_blackBrush)
        {
            Paint(painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.StrokePathBrush(path.Data(), thickness, brush, style);
            });
            return Read(painter);
        }

        Image PaintFill(IImmediatePainter& painter, const BrushDesc& brush)
        {
            Paint(painter, [&](IImmediatePainter& p)
            {
                p.Clear(c_white);
                p.FillRoundedRectangleBrush(c_full, 0, 0, brush);
            });
            return Read(painter);
        }

        // Horizontal line through y = 32 (rows 30..33 fully covered at thickness 4).
        PathBuilder HorizontalLine(float x0, float x1, float y = 32)
        {
            PathBuilder path;
            path.Move(x0, y).Line(x1, y);
            return path;
        }

        // ---- Glyph runs ----

        ComPtr<IDWriteFontFace> FindFontFace(const wchar_t* familyName)
        {
            ComPtr<IDWriteFactory> factory;
            VERIFY_SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(factory.GetAddressOf())));
            ComPtr<IDWriteFontCollection> collection;
            VERIFY_SUCCEEDED(factory->GetSystemFontCollection(&collection, FALSE));
            UINT32 familyIndex = 0;
            BOOL exists = FALSE;
            VERIFY_SUCCEEDED(collection->FindFamilyName(familyName, &familyIndex, &exists));
            if (!exists) return nullptr;
            ComPtr<IDWriteFontFamily> family;
            VERIFY_SUCCEEDED(collection->GetFontFamily(familyIndex, &family));
            ComPtr<IDWriteFont> font;
            VERIFY_SUCCEEDED(family->GetFirstMatchingFont(DWRITE_FONT_WEIGHT_BOLD, DWRITE_FONT_STRETCH_NORMAL, DWRITE_FONT_STYLE_NORMAL, &font));
            ComPtr<IDWriteFontFace> face;
            VERIFY_SUCCEEDED(font->CreateFontFace(&face));
            return face;
        }

        struct GlyphFixture
        {
            ComPtr<IDWriteFontFace> fontFace;
            std::vector<uint16_t> glyphIndices;
            std::vector<float> advances;
            float emSize = 0;
            float width = 0;

            bool Create(const wchar_t* text, float fontEmSize)
            {
                fontFace = FindFontFace(L"Segoe UI");
                if (!fontFace) return false;
                emSize = fontEmSize;
                std::vector<UINT32> points;
                for (const wchar_t* c = text; *c; ++c) points.push_back(*c);
                glyphIndices.assign(points.size(), 0);
                VERIFY_SUCCEEDED(fontFace->GetGlyphIndices(points.data(), static_cast<UINT32>(points.size()), glyphIndices.data()));
                DWRITE_FONT_METRICS metrics{};
                fontFace->GetMetrics(&metrics);
                std::vector<DWRITE_GLYPH_METRICS> glyphMetrics(glyphIndices.size());
                VERIFY_SUCCEEDED(fontFace->GetDesignGlyphMetrics(glyphIndices.data(), static_cast<UINT32>(glyphIndices.size()), glyphMetrics.data(), FALSE));
                width = 0;
                for (const auto& m : glyphMetrics)
                {
                    advances.push_back(m.advanceWidth * emSize / metrics.designUnitsPerEm);
                    width += advances.back();
                }
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

        // ---- Cross-backend comparison ----

        struct Difference
        {
            double meanAbs = 0;    // mean absolute channel difference over all pixels
            uint32_t large = 0;    // pixels with a channel difference above 64
        };

        Difference Compare(const Image& a, const Image& b, const wchar_t* label)
        {
            Difference result;
            uint64_t sum = 0;
            std::wstring map;
            for (uint32_t y = 0; y < a.height; ++y)
            {
                for (uint32_t x = 0; x < a.width; ++x)
                {
                    const Pixel pa = a.At(x, y), pb = b.At(x, y);
                    sum += std::abs(pa.b - pb.b) + std::abs(pa.g - pb.g) + std::abs(pa.r - pb.r) + std::abs(pa.a - pb.a);
                    if (MaxChannelDifference(pa, pb) > 64) ++result.large;
                }
            }
            result.meanAbs = static_cast<double>(sum) / (static_cast<double>(a.width) * a.height * 4);
            Log::Comment(String().Format(L"%s: mean abs difference %.3f, %u pixels differ by more than 64", label, result.meanAbs, result.large));
            if (result.large > 0)
            {
                // 4x4-pixel cells: '#' large difference, 'g' GDI+ ink only, 'd' D2D ink only, '.' same.
                for (uint32_t y = 0; y < a.height; y += 4)
                {
                    map.clear();
                    for (uint32_t x = 0; x < a.width; x += 4)
                    {
                        wchar_t c = L'.';
                        for (uint32_t dy = 0; dy < 4 && c == L'.'; ++dy)
                        {
                            for (uint32_t dx = 0; dx < 4 && c == L'.'; ++dx)
                            {
                                const Pixel pa = a.At(x + dx, y + dy), pb = b.At(x + dx, y + dy);
                                if (MaxChannelDifference(pa, pb) > 64)
                                {
                                    const int inkA = 765 - (pa.r + pa.g + pa.b), inkB = 765 - (pb.r + pb.g + pb.b);
                                    c = inkA > inkB + 96 ? L'g' : inkB > inkA + 96 ? L'd' : L'#';
                                }
                            }
                        }
                        map.push_back(c);
                    }
                    Log::Comment(map.c_str());
                }
            }
            return result;
        }

        constexpr uint32_t c_sceneSize = 128;

        template <typename PaintFn>
        void CompareBackends(const wchar_t* label, PaintFn&& paint, double maxMean, uint32_t maxLarge)
        {
            std::unique_ptr<IImmediatePainter> gdiplus, d2d;
            VERIFY_SUCCEEDED(CreateImmediatePainter(Mode::ImmediateGdiPlus, PainterOptions{ c_sceneSize, c_sceneSize, 1.0f }, gdiplus));
            VERIFY_SUCCEEDED(CreateImmediatePainter(Mode::ImmediateD2DSoftware, PainterOptions{ c_sceneSize, c_sceneSize, 1.0f }, d2d));
            Paint(*gdiplus, paint);
            Paint(*d2d, paint);
            const Difference difference = Compare(Read(*gdiplus, c_sceneSize, c_sceneSize), Read(*d2d, c_sceneSize, c_sceneSize), label);
            VERIFY_IS_TRUE(difference.meanAbs <= maxMean, String().Format(L"%s: mean difference %.3f > %.3f", label, difference.meanAbs, maxMean));
            VERIFY_IS_TRUE(difference.large <= maxLarge, String().Format(L"%s: %u large differences > %u", label, difference.large, maxLarge));
        }
    }

    // ---- Stroke styles ----

    void ShapeBrushPainterConformanceTests::DashArrayAlternatesOnAndOff()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 8, 8 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        // Dashes [4,12) [20,28) [36,44) [52,60).
        const Image image = PaintStroke(*painter, HorizontalLine(4, 60), 4, &style);
        for (uint32_t x : { 7u, 23u, 39u, 55u }) ExpectInk(image, x, 31, L"dash");
        for (uint32_t x : { 15u, 31u, 47u, 61u }) ExpectBlank(image, x, 31, L"gap");
    }

    void ShapeBrushPainterConformanceTests::DashOffsetShiftsPatternTowardStart()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 8, 8 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        style.dashOffset = 4;
        // The stroke starts 4 DIPs into the first dash: [4,8) [16,24) [32,40) [48,56).
        const Image image = PaintStroke(*painter, HorizontalLine(4, 60), 4, &style);
        for (uint32_t x : { 5u, 19u, 35u, 51u }) ExpectInk(image, x, 31, L"dash");
        for (uint32_t x : { 11u, 27u, 43u, 58u }) ExpectBlank(image, x, 31, L"gap");
    }

    void ShapeBrushPainterConformanceTests::OddDashArrayRepeats()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 8, 4, 4 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 3;
        // {8,4,4} repeats as {8,4,4,8,4,4}: on [4,12) off [12,16) on [16,20) off [20,28) on [28,32) off [32,36) on [36,44).
        const Image image = PaintStroke(*painter, HorizontalLine(4, 60), 4, &style);
        for (uint32_t x : { 9u, 17u, 29u, 39u }) ExpectInk(image, x, 31, L"dash");
        for (uint32_t x : { 13u, 21u, 25u, 33u }) ExpectBlank(image, x, 31, L"gap");
    }

    void ShapeBrushPainterConformanceTests::DashesScaleWithTransform()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 4, 4 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform(Matrix3x2F{ 2, 0, 0, 2, 0, 0 });
            p.StrokePathBrush(HorizontalLine(2, 30, 16).Data(), 2, c_blackBrush, &style);
        });
        // Device space: thickness 4 at y = 32, dashes [4,12) [20,28) [36,44) [52,60).
        const Image image = Read(*painter);
        for (uint32_t x : { 7u, 23u, 39u, 55u }) ExpectInk(image, x, 31, L"dash");
        for (uint32_t x : { 15u, 31u, 47u }) ExpectBlank(image, x, 31, L"gap");
    }

    void ShapeBrushPainterConformanceTests::StartAndEndCapsExtendBeyondEndPoints()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Thickness 8 from x = 16 to x = 48 at y = 32: rows 28..35.
        const auto paint = [&](LineCap start, LineCap end)
        {
            StrokeStyleDesc style{};
            style.startCap = start;
            style.endCap = end;
            return PaintStroke(*painter, HorizontalLine(16, 48), 8, &style);
        };

        Log::Comment(L"Flat: nothing beyond the end points");
        Image image = paint(LineCap::Flat, LineCap::Flat);
        ExpectInk(image, 20, 32, L"body");
        ExpectBlank(image, 14, 32, L"before start");
        ExpectBlank(image, 49, 32, L"after end");

        Log::Comment(L"Square start: a half-thickness square before the start, flat end");
        image = paint(LineCap::Square, LineCap::Flat);
        ExpectInk(image, 13, 32, L"square cap");
        ExpectInk(image, 12, 28, L"square cap corner");
        ExpectBlank(image, 10, 32, L"beyond square cap");
        ExpectBlank(image, 49, 32, L"flat end");

        Log::Comment(L"Round start: a half circle, corners stay blank");
        image = paint(LineCap::Round, LineCap::Flat);
        ExpectInk(image, 13, 32, L"round cap");
        ExpectInk(image, 12, 31, L"round cap near tip", 100);
        ExpectBlank(image, 12, 28, L"round cap corner");

        Log::Comment(L"Triangle start: a point of height thickness/2 (x = 12)");
        image = paint(LineCap::Triangle, LineCap::Flat);
        ExpectInk(image, 14, 31, L"triangle cap body", 100);
        ExpectBlank(image, 13, 29, L"triangle cap side");
        ExpectBlank(image, 12, 28, L"triangle cap corner");
        {
            const Pixel tip = image.At(12, 31);
            LogPixel(L"triangle tip (partial coverage)", 12, 31, tip);
            VERIFY_IS_TRUE(tip.r > 40 && tip.r < 230);
        }

        Log::Comment(L"End caps use endCap");
        image = paint(LineCap::Flat, LineCap::Square);
        ExpectBlank(image, 14, 32, L"flat start");
        ExpectInk(image, 50, 32, L"square end cap");
        ExpectBlank(image, 53, 32, L"beyond square end cap");
    }

    void ShapeBrushPainterConformanceTests::DashCapsApplyBetweenDashes()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 4, 12 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        // Dashes [8,12) [24,28) [40,44), thickness 4.
        Log::Comment(L"Flat dash caps: gaps are blank right next to the dashes");
        Image image = PaintStroke(*painter, HorizontalLine(8, 54), 4, &style);
        ExpectInk(image, 9, 32, L"dash");
        ExpectBlank(image, 13, 32, L"after dash");
        ExpectBlank(image, 22, 32, L"before dash");

        Log::Comment(L"Round dash caps: radius-2 half circles reach into the gaps");
        style.dashCap = LineCap::Round;
        image = PaintStroke(*painter, HorizontalLine(8, 54), 4, &style);
        ExpectInk(image, 12, 32, L"round cap after dash", 100);
        ExpectInk(image, 23, 32, L"round cap before dash", 100);
        ExpectBlank(image, 18, 32, L"middle of gap");
        ExpectBlank(image, 15, 32, L"beyond round cap");

        Log::Comment(L"Square dash caps");
        style.dashCap = LineCap::Square;
        image = PaintStroke(*painter, HorizontalLine(8, 54), 4, &style);
        ExpectInk(image, 13, 30, L"square cap after dash");
        ExpectInk(image, 22, 33, L"square cap before dash");
        ExpectBlank(image, 18, 32, L"middle of gap");
    }

    void ShapeBrushPainterConformanceTests::ZeroLengthDashWithRoundCapDrawsDot()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 0, 16 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        style.dashCap = LineCap::Round;
        style.startCap = LineCap::Round;
        style.endCap = LineCap::Round;
        // Dots of radius 4 at x = 8, 24, 40.
        Image image = PaintStroke(*painter, HorizontalLine(8, 50), 8, &style);
        ExpectInk(image, 24, 32, L"dot center");
        ExpectInk(image, 24, 29, L"dot top", 100);
        ExpectInk(image, 40, 34, L"dot");
        ExpectBlank(image, 31, 32, L"between dots");
        ExpectBlank(image, 24, 26, L"above dot");

        Log::Comment(L"Flat caps: zero-length dashes draw nothing");
        style.dashCap = style.startCap = style.endCap = LineCap::Flat;
        image = PaintStroke(*painter, HorizontalLine(8, 50), 8, &style);
        ExpectBlank(image, 24, 32, L"no dot");
        ExpectBlank(image, 40, 32, L"no dot");
    }

    void ShapeBrushPainterConformanceTests::LineJoinsShapeSharpCorner()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Apex (32,12) between two segments of half-angle 26.57 degrees, thickness 8: the miter tip reaches y = 3.06,
        // the round join y = 8, the bevel y = 10.21.
        PathBuilder path;
        path.Move(12, 52).Line(32, 12).Line(52, 52);
        StrokeStyleDesc style{};

        Log::Comment(L"Miter");
        style.lineJoin = LineJoin::Miter;
        Image image = PaintStroke(*painter, path, 8, &style);
        ExpectInk(image, 31, 6, L"miter tip");
        ExpectInk(image, 31, 9, L"below tip");

        Log::Comment(L"Round");
        style.lineJoin = LineJoin::Round;
        image = PaintStroke(*painter, path, 8, &style);
        ExpectBlank(image, 31, 6, L"above round join");
        ExpectBlank(image, 31, 7, L"above round join");
        ExpectInk(image, 31, 9, L"round join");

        Log::Comment(L"Bevel");
        style.lineJoin = LineJoin::Bevel;
        image = PaintStroke(*painter, path, 8, &style);
        ExpectBlank(image, 31, 6, L"above bevel");
        ExpectBlank(image, 31, 9, L"above bevel");
        ExpectInk(image, 31, 11, L"bevel");
    }

    void ShapeBrushPainterConformanceTests::MiterLimitClipsSharpJoin()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        PathBuilder path;
        path.Move(12, 52).Line(32, 12).Line(52, 52);
        StrokeStyleDesc style{};
        style.lineJoin = LineJoin::Miter;
        // The miter needs 2.236 half-thicknesses; a limit of 1 clips it 4 DIPs from the apex (y = 8), unlike a bevel.
        style.miterLimit = 1;
        const Image image = PaintStroke(*painter, path, 8, &style);
        ExpectBlank(image, 31, 6, L"clipped miter tip");
        ExpectInk(image, 31, 9, L"clipped miter body");
    }

    void ShapeBrushPainterConformanceTests::DashedRectangleStartsAtTopLeftClockwise()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 8, 8 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 8, 8, 56, 56 }, 0, 0, 2, c_blackBrush, &style);
        });
        const Image image = Read(*painter);
        // Top: right from (8,8); right: down from (56,8) at 48 (on); bottom: left from (56,56); left: up from (8,56).
        for (uint32_t x : { 12u, 28u, 44u }) ExpectInk(image, x, 8, L"top dash");
        for (uint32_t x : { 20u, 36u, 52u }) ExpectBlank(image, x, 8, L"top gap");
        ExpectInk(image, 56, 12, L"right dash");
        ExpectBlank(image, 56, 20, L"right gap");
        ExpectInk(image, 51, 56, L"bottom dash");
        ExpectBlank(image, 43, 56, L"bottom gap");
        ExpectInk(image, 8, 51, L"left dash");
        ExpectBlank(image, 8, 43, L"left gap");
    }

    void ShapeBrushPainterConformanceTests::DashedRoundedRectangleStartsAtTopLeftArc()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float dashes[] = { 4, 4 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 8, 8, 56, 56 }, 8, 8, 2, c_blackBrush, &style);
        });
        const Image image = Read(*painter);
        // Starting at (8,16) up the top-left arc (length 4π = 12.57): on [0,4) off [4,8) on [8,12) off [12,16) on [16,20),
        // so along the top edge (x = 16 + d - 12.57): on [19.4,23.4) off [23.4,27.4) on [27.4,31.4) off [31.4,35.4).
        ExpectInk(image, 21, 8, L"first top dash");
        ExpectBlank(image, 25, 8, L"first top gap");
        ExpectInk(image, 29, 8, L"second top dash");
        ExpectBlank(image, 33, 8, L"second top gap");
    }

    void ShapeBrushPainterConformanceTests::DashedEllipseStartsAtRightmostPointClockwise()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        const float quarter = 3.14159265f * 20 / 2;
        const float dashes[] = { quarter, quarter };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 12, 12, 52, 52 }, 20, 20, 4, c_blackBrush, &style);
        });
        const Image image = Read(*painter);
        // Quarters clockwise (y down) from the rightmost point: right->bottom on, bottom->left off, left->top on.
        ExpectInk(image, 46, 46, L"bottom-right quarter", 80);
        ExpectBlank(image, 17, 46, L"bottom-left quarter", 180);
        ExpectInk(image, 17, 17, L"top-left quarter", 80);
        ExpectBlank(image, 46, 17, L"top-right quarter", 180);
    }

    // ---- Brushes ----

    void ShapeBrushPainterConformanceTests::SolidBrushMatchesColorPrimitives()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        PathBuilder triangle;
        triangle.Move(8, 56).Line(32, 30).Line(56, 56).Close();
        const ColorF halfRed{ 1, 0, 0, 0.5f };
        const ColorF blue = c_blue;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangle({ 4, 4, 40, 28 }, 6, 6, halfRed);
            p.StrokeRoundedRectangle({ 30, 6, 60, 26 }, 4, 4, 3, blue);
            p.FillPath(triangle.Data(), c_green);
            p.StrokePath(triangle.Data(), 2, c_black);
        });
        const Image expected = Read(*painter);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.FillRoundedRectangleBrush({ 4, 4, 40, 28 }, 6, 6, Solid(c_red, 0.5f));
            p.StrokeRoundedRectangleBrush({ 30, 6, 60, 26 }, 4, 4, 3, Solid(blue), nullptr);
            p.FillPathBrush(triangle.Data(), Solid(c_green));
            p.StrokePathBrush(triangle.Data(), 2, Solid(c_black), nullptr);
        });
        const Image actual = Read(*painter);
        uint32_t changed = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                if (MaxChannelDifference(actual.At(x, y), expected.At(x, y)) > Tolerance(4) && changed++ == 0)
                {
                    LogPixel(L"first difference", x, y, actual.At(x, y));
                    LogPixel(L"  color primitive", x, y, expected.At(x, y));
                }
            }
        }
        Log::Comment(String().Format(L"%u pixels differ", changed));
        // GDI+ builds the rounded stroke outline from four arcs either way, but the segment order differs.
        VERIFY_IS_LESS_THAN_OR_EQUAL(changed, CurrentMode() == Mode::ImmediateGdiPlus ? 6u : 0u);
    }

    void ShapeBrushPainterConformanceTests::LinearGradientSpreadModes()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // t = (x + 0.5 - 16) / 16 at pixel x.
        for (GradientSpread spread : { GradientSpread::Pad, GradientSpread::Reflect, GradientSpread::Repeat })
        {
            Log::Comment(String().Format(L"spread %d", static_cast<int>(spread)));
            LinearBrush brush({ 16, 0 }, { 32, 0 }, Brush::RedBlue(), spread);
            const Image image = PaintFill(*painter, brush.desc);
            for (uint32_t x : { 4u, 20u, 27u, 40u, 56u })
            {
                const float t = Spread((x + 0.5f - 16) / 16, spread);
                ExpectColor(image, x, 10, RedBlueAt(t), Tolerance(5), L"gradient");
            }
        }
    }

    void ShapeBrushPainterConformanceTests::LinearGradientInterpolationModes()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        LinearBrush brush({ 0, 0 }, { 64, 0 }, { { 0, c_black }, { 1, c_white } });
        for (GradientInterpolation interpolation : { GradientInterpolation::SRgb, GradientInterpolation::Linear })
        {
            brush.desc.interpolation = interpolation;
            const Image image = PaintFill(*painter, brush.desc);
            for (uint32_t x : { 8u, 31u, 50u })
            {
                const float t = (x + 0.5f) / 64;
                const float v = interpolation == GradientInterpolation::SRgb ? t : LinearToSRgb(t);
                ExpectColor(image, x, 20, { v, v, v, 1 }, Tolerance(5), interpolation == GradientInterpolation::SRgb ? L"sRGB" : L"linear light");
            }
        }
    }

    void ShapeBrushPainterConformanceTests::LinearGradientBrushTransform()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        LinearBrush brush({ 0, 0 }, { 64, 0 });
        Log::Comment(L"Rotated 90 degrees: the gradient runs top to bottom");
        brush.desc.transform = Matrix3x2F{ 0, 1, -1, 0, 0, 0 };
        Image image = PaintFill(*painter, brush.desc);
        ExpectColor(image, 5, 10, RedBlueAt(10.5f / 64), Tolerance(5), L"rotated");
        ExpectColor(image, 50, 50, RedBlueAt(50.5f / 64), Tolerance(5), L"rotated");

        Log::Comment(L"Scaled by 0.5 horizontally: the ramp ends at x = 32");
        brush.desc.transform = Matrix3x2F{ 0.5f, 0, 0, 1, 0, 0 };
        image = PaintFill(*painter, brush.desc);
        ExpectColor(image, 8, 5, RedBlueAt(8.5f / 32), Tolerance(5), L"scaled");
        ExpectColor(image, 40, 5, c_blue, Tolerance(5), L"scaled, padded");
    }

    void ShapeBrushPainterConformanceTests::RadialGradientCenterAndEdge()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        RadialBrush brush({ 32, 32 }, { 32, 32 }, 24, 24);
        const Image image = PaintFill(*painter, brush.desc);
        for (auto [x, y] : Points{ { 31, 31 }, { 44, 31 }, { 31, 20 }, { 40, 40 }, { 2, 2 }, { 60, 33 } })
        {
            const float t = std::clamp(std::hypot(x + 0.5f - 32, y + 0.5f - 32) / 24, 0.0f, 1.0f);
            ExpectColor(image, x, y, RedBlueAt(t), Tolerance(5), L"radial");
        }
    }

    void ShapeBrushPainterConformanceTests::RadialGradientSpreadModes()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        for (GradientSpread spread : { GradientSpread::Pad, GradientSpread::Reflect, GradientSpread::Repeat })
        {
            Log::Comment(String().Format(L"spread %d", static_cast<int>(spread)));
            RadialBrush brush({ 32, 32 }, { 32, 32 }, 16, 16, spread);
            const Image image = PaintFill(*painter, brush.desc);
            for (auto [x, y] : Points{ { 40, 31 }, { 52, 31 }, { 61, 31 }, { 2, 2 }, { 31, 58 } })
            {
                const float t = Spread(std::hypot(x + 0.5f - 32, y + 0.5f - 32) / 16, spread);
                ExpectColor(image, x, y, RedBlueAt(t), Tolerance(5), L"radial spread");
            }
        }
    }

    void ShapeBrushPainterConformanceTests::RadialGradientEllipticRadii()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        RadialBrush brush({ 32, 32 }, { 32, 32 }, 24, 12);
        const Image image = PaintFill(*painter, brush.desc);
        for (auto [x, y] : Points{ { 31, 37 }, { 43, 31 }, { 38, 36 }, { 31, 46 }, { 58, 31 } })
        {
            const float dx = (x + 0.5f - 32) / 24, dy = (y + 0.5f - 32) / 12;
            ExpectColor(image, x, y, RedBlueAt(std::min(std::hypot(dx, dy), 1.0f)), Tolerance(5), L"elliptic");
        }
    }

    void ShapeBrushPainterConformanceTests::RadialGradientFocalOrigin()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        // Circle centered (32.5,32.5), radius 24 (x from 8.5 to 56.5 on row 32), focal point (20.5,32.5): along the
        // row, offset = distance from the focal point / distance from the focal point to the circle.
        RadialBrush brush({ 32.5f, 32.5f }, { 20.5f, 32.5f }, 24, 24);
        const Image image = PaintFill(*painter, brush.desc);
        ExpectColor(image, 20, 32, RedBlueAt(0), Tolerance(5), L"focal point");
        ExpectColor(image, 14, 32, RedBlueAt(0.5f), Tolerance(5), L"halfway to the near edge");
        ExpectColor(image, 38, 32, RedBlueAt(0.5f), Tolerance(5), L"halfway to the far edge");
        ExpectColor(image, 50, 32, RedBlueAt(30.0f / 36), Tolerance(5), L"far side");
        ExpectColor(image, 4, 32, c_blue, Tolerance(5), L"outside (pad)");
    }

    void ShapeBrushPainterConformanceTests::GradientFillRespectsTransformClipAndDpi()
    {
        auto painter = CreatePainter(c_size, c_size, 2.0f);
        if (!painter) return;
        // DIPs: radius-12 radial at (12,12) under a (4,4) translation -> device center (32,32), radius 24 px. The
        // clip ends at DIP x = 20 (device 40).
        RadialBrush brush({ 12, 12 }, { 12, 12 }, 12, 12);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 20, 32 });
            p.SetTransform(Matrix3x2F{ 1, 0, 0, 1, 4, 4 });
            p.FillRoundedRectangleBrush({ 0, 0, 24, 24 }, 0, 0, brush.desc);
            p.SetTransform(Matrix3x2F{});
            p.PopAxisAlignedClip();
        });
        const Image image = Read(*painter);
        for (auto [x, y] : Points{ { 31, 31 }, { 20, 31 }, { 31, 12 }, { 37, 37 } })
        {
            const float t = std::min(std::hypot(x + 0.5f - 32, y + 0.5f - 32) / 24, 1.0f);
            ExpectColor(image, x, y, RedBlueAt(t), Tolerance(5), L"inside clip");
        }
        ExpectColor(image, 44, 31, c_white, 2, L"clipped");
        ExpectColor(image, 31, 60, c_white, 2, L"outside the rect");
    }

    void ShapeBrushPainterConformanceTests::GradientStrokePaintsOnlyTheStroke()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        LinearBrush brush({ 0, 0 }, { 64, 0 });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 8, 8, 56, 56 }, 0, 0, 4, brush.desc, nullptr);
        });
        const Image image = Read(*painter);
        ExpectColor(image, 8, 32, RedBlueAt(8.5f / 64), Tolerance(5), L"left edge");
        ExpectColor(image, 55, 32, RedBlueAt(55.5f / 64), Tolerance(5), L"right edge");
        ExpectColor(image, 32, 8, RedBlueAt(32.5f / 64), Tolerance(5), L"top edge");
        ExpectColor(image, 32, 32, c_white, 2, L"interior");
        ExpectColor(image, 2, 32, c_white, 2, L"outside");
    }

    void ShapeBrushPainterConformanceTests::GradientDashedStrokeKeepsGaps()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        LinearBrush brush({ 0, 0 }, { 64, 0 });
        const float dashes[] = { 8, 8 };
        StrokeStyleDesc style{};
        style.dashes = dashes;
        style.dashCount = 2;
        style.lineJoin = LineJoin::Round;
        const Image image = PaintStroke(*painter, HorizontalLine(4, 60), 4, &style, brush.desc);
        for (uint32_t x : { 7u, 23u, 39u, 55u }) ExpectColor(image, x, 31, RedBlueAt((x + 0.5f) / 64), Tolerance(5), L"dash");
        for (uint32_t x : { 15u, 31u, 47u }) ExpectColor(image, x, 31, c_white, 2, L"gap");
    }

    void ShapeBrushPainterConformanceTests::GradientGlyphRunInkFollowsGradient()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        GlyphFixture glyphs;
        if (!glyphs.Create(L"HHH", 22))
        {
            Log::Result(TestResults::Skipped, L"Segoe UI is not installed.");
            return;
        }
        const float left = 2, baseline = 40;
        LinearBrush brush({ left, 0 }, { left + glyphs.width, 0 });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRunBrush(glyphs.Run({ left, baseline }), brush.desc);
        });
        const Image image = Read(*painter);
        // Fully inked pixels (green ~0, the gradient has none) take the gradient color at their x.
        uint32_t inked = 0, wrong = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                const Pixel p = image.At(x, y);
                if (p.g > 8) continue;
                ++inked;
                const float t = std::clamp((x + 0.5f - left) / glyphs.width, 0.0f, 1.0f);
                if (!IsColor(p, RedBlueAt(t), Tolerance(8)) && wrong++ == 0)
                {
                    LogPixel(L"first wrong ink", x, y, p);
                }
            }
        }
        Log::Comment(String().Format(L"%u fully inked pixels, %u off the gradient", inked, wrong));
        VERIFY_IS_GREATER_THAN(inked, 60u);
        VERIFY_IS_LESS_THAN_OR_EQUAL(wrong, inked / 20);
        // The ink stays within the run.
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                if (y < 40 - 22 || y > 44)
                {
                    VERIFY_IS_TRUE(IsColor(image.At(x, y), c_white, 2), String().Format(L"no ink at (%u,%u)", x, y));
                }
            }
        }
    }

    void ShapeBrushPainterConformanceTests::SolidBrushGlyphRunMatchesColorGlyphRun()
    {
        auto painter = CreatePainter();
        if (!painter) return;
        GlyphFixture glyphs;
        if (!glyphs.Create(L"Hello", 20))
        {
            Log::Result(TestResults::Skipped, L"Segoe UI is not installed.");
            return;
        }
        const GlyphRunDesc run = glyphs.Run({ 4, 36 });
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRun(run, c_blue);
        });
        const Image expected = Read(*painter);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.DrawGlyphRunBrush(run, Solid(c_blue));
        });
        const Image actual = Read(*painter);
        uint32_t changed = 0, ink = 0;
        for (uint32_t y = 0; y < c_size; ++y)
        {
            for (uint32_t x = 0; x < c_size; ++x)
            {
                ink += IsInk(expected.At(x, y)) ? 1 : 0;
                changed += MaxChannelDifference(actual.At(x, y), expected.At(x, y)) > 2 ? 1 : 0;
            }
        }
        Log::Comment(String().Format(L"%u ink pixels, %u differ", ink, changed));
        VERIFY_IS_GREATER_THAN(ink, 30u);
        VERIFY_ARE_EQUAL(changed, 0u);
    }

    // ---- Cross-backend ----

    void ShapeBrushCrossBackendTests::StyledStrokeScenesMatch()
    {
        const float dashes[] = { 6, 4 };
        const float dots[] = { 0, 8 };
        StrokeStyleDesc dashed{};
        dashed.dashes = dashes;
        dashed.dashCount = 2;
        dashed.dashOffset = 2;

        CompareBackends(L"dashed rectangle", [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 10, 10, 118, 70 }, 0, 0, 3, c_blackBrush, &dashed);
        }, 1.0, 20);

        CompareBackends(L"dashed rounded rectangle", [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            StrokeStyleDesc style = dashed;
            style.dashCap = LineCap::Round;
            p.StrokeRoundedRectangleBrush({ 10, 10, 118, 100 }, 20, 14, 3, c_blackBrush, &style);
        }, 1.0, 20);

        CompareBackends(L"dashed ellipse", [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            StrokeStyleDesc style = dashed;
            style.dashCap = LineCap::Square;
            p.StrokeRoundedRectangleBrush({ 10, 20, 118, 108 }, 54, 44, 4, c_blackBrush, &style);
        }, 1.0, 20);

        CompareBackends(L"dashed closed triangle", [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            PathBuilder triangle;
            triangle.Move(64, 10).Line(118, 110).Line(10, 110).Close();
            StrokeStyleDesc style = dashed;
            style.lineJoin = LineJoin::Round;
            p.StrokePathBrush(triangle.Data(), 4, c_blackBrush, &style);
        }, 1.0, 20);

        CompareBackends(L"polyline with joins and caps", [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            PathBuilder zigzag;
            zigzag.Move(12, 100).Line(40, 20).Line(64, 100).Line(88, 30).Line(116, 90);
            StrokeStyleDesc style{};
            style.startCap = LineCap::Triangle;
            style.endCap = LineCap::Round;
            style.lineJoin = LineJoin::Miter;
            style.miterLimit = 2;
            p.StrokePathBrush(zigzag.Data(), 10, c_blackBrush, &style);
            PathBuilder line;
            // Ends between dots: a zero-length dash exactly at an open end is backend-defined.
            line.Move(20, 116).Line(106, 116);
            StrokeStyleDesc dotStyle{};
            dotStyle.dashes = dots;
            dotStyle.dashCount = 2;
            dotStyle.dashCap = LineCap::Round;
            p.StrokePathBrush(line.Data(), 6, c_blackBrush, &dotStyle);
        }, 1.0, 20);
    }

    void ShapeBrushCrossBackendTests::GradientScenesMatch()
    {
        CompareBackends(L"focal elliptic radial, reflect", [&](IImmediatePainter& p)
        {
            RadialBrush brush({ 64, 64 }, { 44, 54 }, 40, 28, GradientSpread::Reflect);
            p.Clear(c_white);
            p.FillRoundedRectangleBrush({ 0, 0, 128, 128 }, 0, 0, brush.desc);
        }, 1.5, 40);

        CompareBackends(L"rotated repeating linear, linear light", [&](IImmediatePainter& p)
        {
            LinearBrush brush({ 0, 0 }, { 24, 0 }, { { 0, c_black }, { 0.5f, c_green }, { 1, c_white } }, GradientSpread::Repeat);
            brush.desc.interpolation = GradientInterpolation::Linear;
            brush.desc.transform = Matrix3x2F{ 0.8f, 0.6f, -0.6f, 0.8f, 20, 10 };
            p.Clear(c_white);
            p.FillRoundedRectangleBrush({ 8, 8, 120, 120 }, 16, 16, brush.desc);
        }, 1.5, 40);

        CompareBackends(L"gradient dashed stroke and text", [&](IImmediatePainter& p)
        {
            LinearBrush brush({ 0, 0 }, { 128, 128 }, Brush::RedBlue(), GradientSpread::Pad);
            brush.desc.opacity = 0.8f;
            const float dashes[] = { 12, 6 };
            StrokeStyleDesc style{};
            style.dashes = dashes;
            style.dashCount = 2;
            style.dashCap = LineCap::Round;
            p.Clear(c_white);
            p.StrokeRoundedRectangleBrush({ 8, 8, 120, 120 }, 12, 12, 5, brush.desc, &style);
            GlyphFixture glyphs;
            if (glyphs.Create(L"Gradient", 24))
            {
                p.DrawGlyphRunBrush(glyphs.Run({ 16, 72 }), brush.desc);
            }
        }, 2.0, 160);
    }
}
