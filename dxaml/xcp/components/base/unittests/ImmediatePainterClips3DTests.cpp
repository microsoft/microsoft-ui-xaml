// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImmediatePainterClips3DTests.h"
#include "ImmediatePainterTestHelpers.h"
#include <ImmediatePainter.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace RenderingBackendTests
{
    using namespace RenderingBackend;
    using namespace PainterTestHelpers;
    using namespace WEX::Common;
    using namespace WEX::Logging;

    namespace
    {
        using Matrix4x4F = IImmediatePainter::Matrix4x4F;

        // GDI+ antialiasing and resampling differ slightly from D2D's; allow it a little more slack.
        int Tolerance(int base)
        {
            return CurrentMode() == Mode::ImmediateGdiPlus ? base + 4 : base;
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

        PathBuilder Rectangle(float l, float t, float r, float b)
        {
            PathBuilder path;
            path.Move(l, t).Line(r, t).Line(r, b).Line(l, b).Close();
            return path;
        }

        // Right triangle with the hypotenuse on x + y = 64.
        PathBuilder Triangle()
        {
            PathBuilder path;
            path.Move(8, 8).Line(56, 8).Line(8, 56).Close();
            return path;
        }

        const RectF c_everything{ -1000, -1000, 1000, 1000 };

        void VerifyColor(const Image& image, uint32_t x, uint32_t y, const ColorF& color, int tolerance, const wchar_t* message)
        {
            const Pixel p = image.At(x, y);
            if (!IsColor(p, color, tolerance))
            {
                LogPixel(message, x, y, p);
            }
            VERIFY_IS_TRUE(IsColor(p, color, tolerance), String().Format(L"%s (%u,%u)", message, x, y));
        }

        void VerifyRed(const Image& image, uint32_t x, uint32_t y) { VerifyColor(image, x, y, c_red, Tolerance(3), L"expected red"); }
        void VerifyWhite(const Image& image, uint32_t x, uint32_t y) { VerifyColor(image, x, y, c_white, Tolerance(3), L"expected white"); }

        // x' = x / (1 + 0.01 x), y' = y / (1 + 0.01 x): the far (right) side recedes.
        Matrix4x4F Receding()
        {
            Matrix4x4F m;
            m.m[0][3] = 0.01f;
            return m;
        }
    }

    void Clips3DPainterConformanceTests::GeometryClipKeepsInsideAndRemovesOutside()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder triangle = Triangle();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushGeometryClip(triangle.Data());
            p.FillRectangle(c_everything, c_red);
            p.PopGeometryClip();
        });
        const Image image = Read(*painter);
        VerifyRed(image, 16, 16);
        VerifyRed(image, 10, 50);
        VerifyRed(image, 50, 10);
        VerifyWhite(image, 40, 40);
        VerifyWhite(image, 4, 4);
        VerifyWhite(image, 60, 32);
    }

    void Clips3DPainterConformanceTests::GeometryClipEdgeIsAntialiased()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder triangle = Triangle();
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushGeometryClip(triangle.Data());
            p.FillRectangle(c_everything, c_red);
            p.PopGeometryClip();
        });
        const Image image = Read(*painter);
        // Pixel (31, 32) is cut in half by the hypotenuse: green and blue ~ 128 (half red over white).
        const Pixel edge = image.At(31, 32);
        LogPixel(L"edge", 31, 32, edge);
        VERIFY_IS_TRUE(edge.r > 240);
        VERIFY_IS_TRUE(edge.g > 64 && edge.g < 192, L"the clip edge is antialiased, not aliased");
        VERIFY_IS_TRUE(edge.b > 64 && edge.b < 192, L"the clip edge is antialiased, not aliased");
    }

    void Clips3DPainterConformanceTests::GeometryClipRespectsRotation()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        // A 16x16 square around (32, 32) rotated 45 degrees: a diamond with |dx| + |dy| <= 11.3.
        const PathBuilder square = Rectangle(-8, -8, 8, 8);
        const float c = std::cos(3.14159265f / 4), s = std::sin(3.14159265f / 4);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ c, s, -s, c, 32, 32 });
            p.PushGeometryClip(square.Data());
            p.SetTransform({});
            p.FillRectangle(c_everything, c_red);
            p.PopGeometryClip();
        });
        const Image image = Read(*painter);
        VerifyRed(image, 32, 32);
        VerifyRed(image, 32, 23); // dy = -8.5: inside the diamond, inside the unrotated square too
        VerifyRed(image, 32, 22); // dy = -9.5: inside the diamond, outside the unrotated square
        VerifyWhite(image, 25, 25); // a corner of the unrotated square, outside the diamond
        VerifyWhite(image, 38, 38);
    }

    void Clips3DPainterConformanceTests::NestedGeometryClipsIntersect()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder triangle = Triangle();
        const PathBuilder left = Rectangle(0, 0, 24, 64);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushGeometryClip(triangle.Data());
            p.PushGeometryClip(left.Data());
            p.FillRectangle(c_everything, c_red);
            p.PopGeometryClip();
            p.PopGeometryClip();
        });
        const Image image = Read(*painter);
        VerifyRed(image, 16, 16);
        VerifyRed(image, 12, 48);
        VerifyWhite(image, 40, 12); // in the triangle, right of the rectangle
        VerifyWhite(image, 20, 60); // in the rectangle, below the triangle
    }

    void Clips3DPainterConformanceTests::GeometryClipPopRestoresDrawing()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder smallRect = Rectangle(8, 8, 16, 16);
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 48, 64 });
            p.PushGeometryClip(smallRect.Data());
            p.FillRectangle(c_everything, c_blue);
            p.PopGeometryClip();
            p.FillRectangle({ 32, 32, 64, 64 }, c_red);
            p.PopAxisAlignedClip();
        });
        const Image image = Read(*painter);
        VerifyColor(image, 12, 12, c_blue, Tolerance(3), L"expected blue");
        VerifyWhite(image, 20, 20);
        VerifyRed(image, 40, 40);
        VerifyWhite(image, 52, 40); // the outer axis-aligned clip is still in effect
    }

    void Clips3DPainterConformanceTests::EmptyGeometryClipClipsEverything()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder empty;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushGeometryClip(empty.Data());
            p.FillRectangle(c_everything, c_red);
            p.PopGeometryClip();
            p.FillRectangle({ 0, 0, 8, 8 }, c_blue);
        });
        const Image image = Read(*painter);
        VerifyWhite(image, 32, 32);
        VerifyColor(image, 4, 4, c_blue, Tolerance(3), L"expected blue after pop");
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerIdentityMatchesFlatDrawing()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.SetTransform({ 1, 0, 0, 1, 50, 50 }); // reset inside the layer, applied again when compositing
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, Matrix4x4F{});
            p.FillRectangle({ 16, 16, 48, 48 }, c_red);
            p.PopPerspectiveLayer();
            p.FillRectangle({ 0, 0, 4, 4 }, c_blue); // the push transform is restored: lands at (50, 50)
        });
        const Image image = Read(*painter);
        VerifyWhite(image, 65, 65);
        VerifyRed(image, 66, 66);
        VerifyRed(image, 97, 97);
        VerifyRed(image, 90, 70);
        VerifyWhite(image, 98, 98);
        VerifyColor(image, 51, 51, c_blue, Tolerance(3), L"expected blue");
        VerifyWhite(image, 49, 49);
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerAffineMatrixMapsCorners()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        // Scale by 0.5 and translate by (8, 16): the layer (0, 0)-(64, 64) maps to (8, 16)-(40, 48).
        Matrix4x4F m;
        m.m[0][0] = 0.5f;
        m.m[1][1] = 0.5f;
        m.m[3][0] = 8;
        m.m[3][1] = 16;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, m);
            p.FillRectangle({ 0, 0, 64, 64 }, c_red);
            p.PopPerspectiveLayer();
        });
        const Image image = Read(*painter);
        VerifyRed(image, 9, 17);
        VerifyRed(image, 38, 46);
        VerifyWhite(image, 6, 20);
        VerifyWhite(image, 42, 20);
        VerifyWhite(image, 20, 14);
        VerifyWhite(image, 20, 50);
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerProjectsWithDepth()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, Receding());
            p.FillRectangle({ 0, 0, 64, 64 }, c_red);
            p.PopPerspectiveLayer();
        });
        const Image image = Read(*painter);
        // Corners: (0,0)->(0,0), (0,64)->(0,64), (64,0)->(39,0), (64,64)->(39,39). The bottom edge rises to the right.
        VerifyRed(image, 2, 60);
        VerifyRed(image, 10, 50);  // source (11.7, 56.4)
        VerifyRed(image, 36, 20);  // source (57.5, 32.2)
        VerifyRed(image, 36, 2);
        VerifyWhite(image, 36, 50); // source y 79.5: below the layer; a flat draw would cover it
        VerifyWhite(image, 45, 10); // source x 83: right of the layer
        VerifyWhite(image, 30, 56); // source (42.9, 80.1)
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerRespectsPushTransformAndClip()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushAxisAlignedClip({ 0, 0, 40, 64 });
            p.SetTransform({ 1, 0, 0, 1, 10, 0 });
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, Receding());
            p.FillRectangle({ 0, 0, 64, 64 }, c_red);
            p.PopPerspectiveLayer();
            p.SetTransform({});
            p.PopAxisAlignedClip();
        });
        const Image image = Read(*painter);
        VerifyWhite(image, 6, 30);  // left of the translated layer
        VerifyRed(image, 12, 30);
        VerifyRed(image, 36, 10);   // local x 26.5: source (36.1, 14.3)
        VerifyWhite(image, 44, 10); // inside the projected layer, outside the clip
        VerifyWhite(image, 36, 60); // source (36.1, 82.3)
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerRespectsDamage()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        Paint(*painter, [&](IImmediatePainter& p) { p.Clear(c_white); });
        const RectF damage{ 0, 0, 32, 64 };
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, Matrix4x4F{});
            p.FillRectangle({ 0, 0, 64, 64 }, c_red);
            p.PopPerspectiveLayer();
        }, &damage, 1);
        const Image image = Read(*painter);
        VerifyRed(image, 10, 10);
        VerifyRed(image, 31, 60);
        VerifyWhite(image, 32, 10);
        VerifyWhite(image, 60, 60);
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerRespectsDpiScale()
    {
        constexpr uint32_t size = 128;
        auto painter = CreatePainter(size, size, 2.0f);
        if (!painter)
        {
            return;
        }
        Matrix4x4F m;
        m.m[3][0] = 8; // DIPs: 16 pixels
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, m);
            p.FillRectangle({ 0, 0, 16, 16 }, c_red);
            p.PopPerspectiveLayer();
        });
        const Image image = Read(*painter, size, size);
        VerifyWhite(image, 12, 10);
        VerifyRed(image, 18, 2);
        VerifyRed(image, 46, 30);
        VerifyWhite(image, 50, 10);
        VerifyWhite(image, 20, 34);
    }

    void Clips3DPainterConformanceTests::PerspectiveLayerNestsOpacityAndClips()
    {
        auto painter = CreatePainter();
        if (!painter)
        {
            return;
        }
        const PathBuilder triangle = Triangle();
        Matrix4x4F m;
        m.m[3][0] = 4;
        Paint(*painter, [&](IImmediatePainter& p)
        {
            p.Clear(c_white);
            p.PushPerspectiveLayer({ 0, 0, 64, 64 }, m);
            p.PushGeometryClip(triangle.Data());
            p.PushOpacityLayer(0.5f);
            p.FillRectangle(c_everything, c_red);
            p.PopOpacityLayer();
            p.PopGeometryClip();
            p.PopPerspectiveLayer();
        });
        const Image image = Read(*painter);
        // Half red over white, inside the triangle shifted right by 4.
        const ColorF pink{ 1, 0.5f, 0.5f, 1 };
        VerifyColor(image, 20, 16, pink, Tolerance(4), L"expected pink");
        VerifyWhite(image, 10, 16); // left of the shifted triangle
        VerifyWhite(image, 40, 40);
    }
}
