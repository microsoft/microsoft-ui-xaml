// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// clips3d (F3 clips and transforms, F4 3D). Included by ImmediateRenderer.cpp before ImmediateRenderer::Walk.
//
// Clips: rectangle clips under scale/translate stay axis-aligned clip rects (ctx.clip); every other clip (rectangles
// under rotation or skew, geometry clips, hand-off visual rounded/elliptic clips) becomes a PushGeometryClip group
// (antialiased painter clip) plus its bounding box in ctx.clip for culling and damage.
//
// 3D: the element-to-island transform is tracked as a full 4x4 (WalkContext::world4, row vectors, never flattened,
// like Composition and CUIElement::GetLocalTransform4x4). While the 2D homography it induces on the element plane is
// affine, the walk keeps drawing with 2D transforms. An element whose own 3D makes it non-affine opens a perspective
// layer: its subtree is drawn into an offscreen layer (layer DIPs) and composited through the 4x4 on pop. Nested 3D
// elements open nested layers whose matrices are computed from the full 4x4 chain, so nesting stays exact (up to the
// resampling of each layer).

namespace ImmediateRenderingClips3D
{
    struct H3
    {
        double m[3][3] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };
    };

    void Mul4(const double a[4][4], const double b[4][4], double r[4][4]) noexcept
    {
        double t[4][4];
        for (int i = 0; i < 4; ++i)
        {
            for (int j = 0; j < 4; ++j)
            {
                t[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j] + a[i][3] * b[3][j];
            }
        }
        memcpy(r, t, sizeof(t));
    }

    void From2D(const CMILMatrix& m, double r[4][4]) noexcept
    {
        const double v[4][4] = {
            { m.GetM11(), m.GetM12(), 0, 0 },
            { m.GetM21(), m.GetM22(), 0, 0 },
            { 0, 0, 1, 0 },
            { m.GetDx(), m.GetDy(), 0, 1 },
        };
        memcpy(r, v, sizeof(v));
    }

    void FromMil4(const CMILMatrix4x4& m, double r[4][4]) noexcept
    {
        const double v[4][4] = {
            { m.GetM11(), m.GetM12(), m.GetM13(), m.GetM14() },
            { m.GetM21(), m.GetM22(), m.GetM23(), m.GetM24() },
            { m.GetM31(), m.GetM32(), m.GetM33(), m.GetM34() },
            { m.GetM41(), m.GetM42(), m.GetM43(), m.GetM44() },
        };
        memcpy(r, v, sizeof(v));
    }

    bool Is2D(const double m[4][4]) noexcept
    {
        return m[0][2] == 0 && m[0][3] == 0 && m[1][2] == 0 && m[1][3] == 0 && m[2][0] == 0 && m[2][1] == 0 && m[2][2] == 1
            && m[2][3] == 0 && m[3][2] == 0 && m[3][3] == 1;
    }

    CMILMatrix To2D(const double m[4][4]) noexcept
    {
        CMILMatrix r(true);
        r.SetM11(static_cast<float>(m[0][0]));
        r.SetM12(static_cast<float>(m[0][1]));
        r.SetM21(static_cast<float>(m[1][0]));
        r.SetM22(static_cast<float>(m[1][1]));
        r.SetDx(static_cast<float>(m[3][0]));
        r.SetDy(static_cast<float>(m[3][1]));
        return r;
    }

    CMILMatrix FromMatrix(const Matrix3x2F& m) noexcept
    {
        CMILMatrix r(true);
        r.SetM11(m.m11);
        r.SetM12(m.m12);
        r.SetM21(m.m21);
        r.SetM22(m.m22);
        r.SetDx(m.dx);
        r.SetDy(m.dy);
        return r;
    }

    // The hand-off visual transform when it keeps the element plane in 2D (folded into the 2D local transform).
    bool HandOffIs2D(const float m[16]) noexcept
    {
        return m[2] == 0 && m[3] == 0 && m[6] == 0 && m[7] == 0 && m[8] == 0 && m[9] == 0 && m[11] == 0 && m[14] == 0
            && m[15] == 1;
    }

    // The element plane (z = 0) homography: rows/columns x, y, w of the 4x4.
    H3 Homography(const double m[4][4]) noexcept
    {
        static const int idx[3] = { 0, 1, 3 };
        H3 h;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                h.m[i][j] = m[idx[i]][idx[j]];
            }
        }
        return h;
    }

    H3 Mul3(const H3& a, const H3& b) noexcept
    {
        H3 r;
        for (int i = 0; i < 3; ++i)
        {
            for (int j = 0; j < 3; ++j)
            {
                r.m[i][j] = a.m[i][0] * b.m[0][j] + a.m[i][1] * b.m[1][j] + a.m[i][2] * b.m[2][j];
            }
        }
        return r;
    }

    H3 Translation(double x, double y) noexcept
    {
        H3 r;
        r.m[2][0] = x;
        r.m[2][1] = y;
        return r;
    }

    bool Invert3(const H3& a, _Out_ H3* r) noexcept
    {
        const auto& m = a.m;
        const double c00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
        const double c01 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
        const double c02 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
        const double det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
        if (!(std::fabs(det) > 1e-18) || !std::isfinite(det))
        {
            return false;
        }
        const double inv = 1.0 / det;
        r->m[0][0] = c00 * inv;
        r->m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * inv;
        r->m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * inv;
        r->m[1][0] = c01 * inv;
        r->m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * inv;
        r->m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * inv;
        r->m[2][0] = c02 * inv;
        r->m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * inv;
        r->m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * inv;
        return true;
    }

    // False when the point is at or behind the camera (w <= 0).
    bool MapPoint(const H3& h, double x, double y, _Out_ double* ox, _Out_ double* oy) noexcept
    {
        const double w = x * h.m[0][2] + y * h.m[1][2] + h.m[2][2];
        if (!(w > 1e-9))
        {
            *ox = *oy = 0;
            return false;
        }
        *ox = (x * h.m[0][0] + y * h.m[1][0] + h.m[2][0]) / w;
        *oy = (x * h.m[0][1] + y * h.m[1][1] + h.m[2][1]) / w;
        return std::isfinite(*ox) && std::isfinite(*oy);
    }

    // Bounds of the mapped corners of r; false when any corner has w <= 0 (the mapped bounds are then unbounded).
    bool MapBounds(const H3& h, const RectF& r, _Out_ RectF* out) noexcept
    {
        const double xs[4] = { r.left, r.right, r.right, r.left };
        const double ys[4] = { r.top, r.top, r.bottom, r.bottom };
        double minX = 0, minY = 0, maxX = 0, maxY = 0;
        for (int i = 0; i < 4; ++i)
        {
            double x, y;
            if (!MapPoint(h, xs[i], ys[i], &x, &y))
            {
                *out = {};
                return false;
            }
            minX = i == 0 ? x : std::min(minX, x);
            minY = i == 0 ? y : std::min(minY, y);
            maxX = i == 0 ? x : std::max(maxX, x);
            maxY = i == 0 ? y : std::max(maxY, y);
        }
        *out = { static_cast<float>(minX), static_cast<float>(minY), static_cast<float>(maxX), static_cast<float>(maxY) };
        return true;
    }

    // w is constant (to float precision) over r: the homography acts as a 2D affine transform there.
    bool IsAffine(const H3& h, const RectF& r) noexcept
    {
        const double extentX = std::max(std::fabs(r.left), std::fabs(r.right)) + 1;
        const double extentY = std::max(std::fabs(r.top), std::fabs(r.bottom)) + 1;
        return std::fabs(h.m[0][2]) * extentX + std::fabs(h.m[1][2]) * extentY <= 1e-6 * std::fabs(h.m[2][2]);
    }

    CMILMatrix ToAffine(const H3& h) noexcept
    {
        const double w = h.m[2][2];
        CMILMatrix r(true);
        r.SetM11(static_cast<float>(h.m[0][0] / w));
        r.SetM12(static_cast<float>(h.m[0][1] / w));
        r.SetM21(static_cast<float>(h.m[1][0] / w));
        r.SetM22(static_cast<float>(h.m[1][1] / w));
        r.SetDx(static_cast<float>(h.m[2][0] / w));
        r.SetDy(static_cast<float>(h.m[2][1] / w));
        return r;
    }

    IImmediatePainter::Matrix4x4F ToPainterMatrix(const H3& h) noexcept
    {
        IImmediatePainter::Matrix4x4F r;
        const float v[4][4] = {
            { static_cast<float>(h.m[0][0]), static_cast<float>(h.m[0][1]), 0, static_cast<float>(h.m[0][2]) },
            { static_cast<float>(h.m[1][0]), static_cast<float>(h.m[1][1]), 0, static_cast<float>(h.m[1][2]) },
            { 0, 0, 1, 0 },
            { static_cast<float>(h.m[2][0]), static_cast<float>(h.m[2][1]), 0, static_cast<float>(h.m[2][2]) },
        };
        memcpy(r.m, v, sizeof(v));
        return r;
    }

    // The element's local transform as a 4x4 (CUIElement::GetLocalTransform4x4 order: hand-off visual, projection,
    // Transform3D, 3D facades, then the 2D local transform). Returns true when any 3D source contributed.
    bool ComputeLocal4(_In_ CUIElement* element, const CMILMatrix& local2D, _In_opt_ const float* handOff, double out[4][4])
    {
        From2D(local2D, out);
        bool any = false;
        double m[4][4];
        if (element->AreFacadesInUse())
        {
            // XAML-evaluated transitions (ImmediateAnimations) animate facades in immediate mode: read their current value.
            const ImmediateAnimations::TransitionStore& transitions = ImmediateAnimations::GetThreadStore();
            const bool preferAnimating = !transitions.Empty() && transitions.HasOwner(element);
            const FacadeTransformInfo facades = element->GetFacadeTransformInfo(preferAnimating);
            if (!facades.Is2D())
            {
                FromMil4(facades.GetMatrix4x4(), m);
                Mul4(m, out, out);
                any = true;
            }
        }
        if (const xref_ptr<CTransform3D> transform3D = element->GetTransform3D())
        {
            // HWCompNodeWinRT updates the matrix from the render size; no comp node runs in immediate modes.
            transform3D->UpdateTransformMatrix(element->GetActualWidth(), element->GetActualHeight());
            FromMil4(transform3D->GetTransformMatrix(), m);
            Mul4(m, out, out);
            any = true;
        }
        if (element->HasActiveProjection())
        {
            XSIZEF size{};
            if (SUCCEEDED(element->GetElementSizeForProjection(&size)))
            {
                FromMil4(element->GetProjection()->GetOverallProjectionMatrix(size), m);
                Mul4(m, out, out);
                any = true;
            }
        }
        if (handOff != nullptr)
        {
            for (int i = 0; i < 4; ++i)
            {
                for (int j = 0; j < 4; ++j)
                {
                    m[i][j] = handOff[i * 4 + j];
                }
            }
            Mul4(m, out, out);
            any = true;
        }
        return any;
    }

    void AppendRect(const RectF& r, _Inout_ PathGeometryData& path)
    {
        path.verbs.insert(path.verbs.end(), { PathVerb::MoveTo, PathVerb::LineTo, PathVerb::LineTo, PathVerb::LineTo, PathVerb::Close });
        path.points.insert(path.points.end(), { { r.left, r.top }, { r.right, r.top }, { r.right, r.bottom }, { r.left, r.bottom } });
    }

    RectF PointBounds(const std::vector<PointF>& points) noexcept
    {
        if (points.empty())
        {
            return {};
        }
        RectF r{ points[0].x, points[0].y, points[0].x, points[0].y };
        for (const PointF& p : points)
        {
            r.left = std::min(r.left, p.x);
            r.top = std::min(r.top, p.y);
            r.right = std::max(r.right, p.x);
            r.bottom = std::max(r.bottom, p.y);
        }
        return r;
    }

    // A clip waiting to be pushed (after the element's opacity layer, so independent layers move it with the element).
    struct PendingClip
    {
        PathGeometryData path;
        CMILMatrix world{ true };
        RectF bounds;
        size_t index = SIZE_MAX;
    };

    Command MakeGeometryClipPush(_Inout_ DisplayList& list, _In_ const CUIElement* element, PathGeometryData&& path,
        const CMILMatrix& world, const RectF& bounds)
    {
        Command push;
        push.type = CommandType::PushGeometryClip;
        push.transform = ToMatrix(world);
        push.rect = bounds;
        push.key = list.paths.size();
        push.owner = element;
        list.paths.push_back(std::move(path));
        return push;
    }

    // An open perspective layer: drawing after pushIndex is in layer DIPs/pixels until FinishPerspective maps the
    // recorded device bounds into the enclosing surface.
    struct PerspectiveScope
    {
        size_t pushIndex = SIZE_MAX;
        uint32_t recordBegin = 0;
        size_t scrollerBegin = 0;
        H3 toOuter;          // layer DIPs -> enclosing DIPs
        RectF outerClip;     // enclosing DIPs
        float scale = 1;
    };

    RectF MapDeviceBounds(const PerspectiveScope& scope, const RectF& r) noexcept
    {
        if (IsEmptyRect(r))
        {
            return r;
        }
        const RectF outerClipPx = ScaleRect(scope.outerClip, scope.scale);
        RectF mapped;
        if (!MapBounds(scope.toOuter, ScaleRect(r, 1 / scope.scale), &mapped))
        {
            return outerClipPx;
        }
        mapped = ScaleRect(mapped, scope.scale);
        mapped = { std::floor(mapped.left) - 1, std::floor(mapped.top) - 1, std::ceil(mapped.right) + 1, std::ceil(mapped.bottom) + 1 };
        return IntersectRect(mapped, outerClipPx);
    }

    // Maps everything recorded inside the layer (command and record bounds, in layer pixels) into the enclosing surface.
    void FinishPerspective(_Inout_ DisplayList& list, const PerspectiveScope& scope)
    {
        for (size_t i = scope.pushIndex + 1; i < list.commands.size(); ++i)
        {
            list.commands[i].deviceBounds = MapDeviceBounds(scope, list.commands[i].deviceBounds);
        }
        for (size_t i = scope.recordBegin; i < list.recordOrder.size(); ++i)
        {
            auto found = list.records.find(list.recordOrder[i]);
            if (found == list.records.end())
            {
                continue;
            }
            ElementRecord& r = found->second;
            r.bounds = MapDeviceBounds(scope, r.bounds);
            r.relBounds = r.bounds;
            r.cover = {};
            r.scroller = nullptr;
            r.groupBounds = MapDeviceBounds(scope, r.groupBounds);
            if (!IsEmptyRect(r.roundedClip))
            {
                r.roundedClip = MapDeviceBounds(scope, r.roundedClip);
                r.roundedRadius = std::max(r.roundedClip.right - r.roundedClip.left, r.roundedClip.bottom - r.roundedClip.top);
            }
        }
        // Scrollers inside the layer: their pixels are resampled, never blitted.
        for (size_t i = scope.scrollerBegin; i < list.scrollers.size(); ++i)
        {
            list.scrollers[i].hasContent = false;
        }
    }
}
