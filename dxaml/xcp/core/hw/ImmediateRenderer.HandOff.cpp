// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// clips3d: reads the static state of a hand-off visual (see ImmediateRenderer.HandOff.h).

#include "precomp.h"
#include "ImmediateRenderer.HandOff.h"
#include <algorithm>
#include <cmath>

using namespace RenderingBackend;
using Microsoft::WRL::ComPtr;

namespace
{
    constexpr float c_kappa = 0.5522847498f; // cubic Bezier control distance for a quarter ellipse

    bool IsAnimating(_In_ IInspectable* object, _In_reads_(count) const wchar_t* const* properties, size_t count,
        _Out_ const wchar_t** animated) noexcept
    {
        *animated = nullptr;
        ComPtr<WUComp::ICompositionObject4> object4;
        if (object == nullptr || FAILED(object->QueryInterface(IID_PPV_ARGS(&object4))))
        {
            return false;
        }
        for (size_t i = 0; i < count; ++i)
        {
            ComPtr<WUComp::IAnimationController> controller;
            if (SUCCEEDED(object4->TryGetAnimationController(wrl_wrappers::HStringReference(properties[i]).Get(), &controller))
                && controller != nullptr)
            {
                *animated = properties[i];
                return true;
            }
        }
        return false;
    }

    // Clip-space -> element-space transform of a CompositionClip (ICompositionClip2), in Composition's order:
    // -CenterPoint, Scale, RotationAngle, +CenterPoint, TransformMatrix, Offset. False if it cannot be represented.
    bool ReadClipTransform(_In_ WUComp::ICompositionClip* clip, _Out_ Matrix3x2F* transform) noexcept
    {
        *transform = {};
        ComPtr<WUComp::ICompositionClip2> clip2;
        if (FAILED(clip->QueryInterface(IID_PPV_ARGS(&clip2))))
        {
            return true;
        }
        wfn::Vector2 anchor{}, center{}, offset{}, scale{ 1, 1 };
        float angle = 0;
        wfn::Matrix3x2 tm{ 1, 0, 0, 1, 0, 0 };
        IGNOREHR(clip2->get_AnchorPoint(&anchor));
        IGNOREHR(clip2->get_CenterPoint(&center));
        IGNOREHR(clip2->get_Offset(&offset));
        IGNOREHR(clip2->get_Scale(&scale));
        IGNOREHR(clip2->get_RotationAngle(&angle));
        IGNOREHR(clip2->get_TransformMatrix(&tm));
        if (anchor.X != 0 || anchor.Y != 0)
        {
            return false; // relative to a size the clip does not have
        }
        const float c = std::cos(angle), s = std::sin(angle);
        // Linear part: Scale, then Rotate, then TransformMatrix.
        const float a11 = scale.X * c, a12 = scale.X * s, a21 = -scale.Y * s, a22 = scale.Y * c;
        Matrix3x2F m;
        m.m11 = a11 * tm.M11 + a12 * tm.M21;
        m.m12 = a11 * tm.M12 + a12 * tm.M22;
        m.m21 = a21 * tm.M11 + a22 * tm.M21;
        m.m22 = a21 * tm.M12 + a22 * tm.M22;
        // Translation: (-center * S * R + center) * TM + TM.translation + offset.
        const float px = -center.X * a11 - center.Y * a21 + center.X;
        const float py = -center.X * a12 - center.Y * a22 + center.Y;
        m.dx = px * tm.M11 + py * tm.M21 + tm.M31 + offset.X;
        m.dy = px * tm.M12 + py * tm.M22 + tm.M32 + offset.Y;
        *transform = m;
        return true;
    }

    void AppendEllipse(float cx, float cy, float rx, float ry, std::vector<PathVerb>& verbs, std::vector<PointF>& points)
    {
        const float kx = rx * c_kappa, ky = ry * c_kappa;
        verbs.push_back(PathVerb::MoveTo);
        points.push_back({ cx + rx, cy });
        const PointF segments[4][3] = {
            { { cx + rx, cy + ky }, { cx + kx, cy + ry }, { cx, cy + ry } },
            { { cx - kx, cy + ry }, { cx - rx, cy + ky }, { cx - rx, cy } },
            { { cx - rx, cy - ky }, { cx - kx, cy - ry }, { cx, cy - ry } },
            { { cx + kx, cy - ry }, { cx + rx, cy - ky }, { cx + rx, cy } },
        };
        for (const auto& segment : segments)
        {
            verbs.push_back(PathVerb::CubicTo);
            points.insert(points.end(), std::begin(segment), std::end(segment));
        }
        verbs.push_back(PathVerb::Close);
    }

    // Reads the clip into info (hasClip, clip path, transform); returns false if the clip kind is not supported.
    bool ReadClip(_In_ WUComp::ICompositionClip* clip, float width, float height, _Inout_ ImmediateRenderingHandOff::VisualInfo* info) noexcept
    {
        if (!ReadClipTransform(clip, &info->clipTransform))
        {
            return false;
        }
        float radii[8] = {};
        RectF rect;
        ComPtr<WUComp::IInsetClip> inset;
        ComPtr<WUComp::IRectangleClip> rectangle;
        ComPtr<WUComp::ICompositionGeometricClip> geometric;
        if (SUCCEEDED(clip->QueryInterface(IID_PPV_ARGS(&inset))))
        {
            float left = 0, top = 0, right = 0, bottom = 0;
            if (FAILED(inset->get_LeftInset(&left)) || FAILED(inset->get_TopInset(&top)) || FAILED(inset->get_RightInset(&right))
                || FAILED(inset->get_BottomInset(&bottom)))
            {
                return false;
            }
            rect = { left, top, width - right, height - bottom };
        }
        else if (SUCCEEDED(clip->QueryInterface(IID_PPV_ARGS(&rectangle))))
        {
            wfn::Vector2 corners[4]{};
            if (FAILED(rectangle->get_Left(&rect.left)) || FAILED(rectangle->get_Top(&rect.top)) || FAILED(rectangle->get_Right(&rect.right))
                || FAILED(rectangle->get_Bottom(&rect.bottom)) || FAILED(rectangle->get_TopLeftRadius(&corners[0]))
                || FAILED(rectangle->get_TopRightRadius(&corners[1])) || FAILED(rectangle->get_BottomRightRadius(&corners[2]))
                || FAILED(rectangle->get_BottomLeftRadius(&corners[3])))
            {
                return false;
            }
            for (int i = 0; i < 4; ++i)
            {
                radii[i * 2] = corners[i].X;
                radii[i * 2 + 1] = corners[i].Y;
            }
        }
        else if (SUCCEEDED(clip->QueryInterface(IID_PPV_ARGS(&geometric))))
        {
            ComPtr<WUComp::ICompositionViewBox> viewBox;
            ComPtr<WUComp::ICompositionGeometry> geometry;
            if (FAILED(geometric->get_ViewBox(&viewBox)) || viewBox != nullptr || FAILED(geometric->get_Geometry(&geometry))
                || geometry == nullptr)
            {
                return false;
            }
            float trimStart = 0, trimEnd = 1;
            IGNOREHR(geometry->get_TrimStart(&trimStart));
            IGNOREHR(geometry->get_TrimEnd(&trimEnd));
            if (trimStart != 0 || trimEnd != 1)
            {
                return false;
            }
            ComPtr<WUComp::ICompositionRectangleGeometry> rectangleGeometry;
            ComPtr<WUComp::ICompositionRoundedRectangleGeometry> roundedGeometry;
            ComPtr<WUComp::ICompositionEllipseGeometry> ellipseGeometry;
            wfn::Vector2 offset{}, size{}, radius{};
            if (SUCCEEDED(geometry.As(&rectangleGeometry)))
            {
                if (FAILED(rectangleGeometry->get_Offset(&offset)) || FAILED(rectangleGeometry->get_Size(&size)))
                {
                    return false;
                }
            }
            else if (SUCCEEDED(geometry.As(&roundedGeometry)))
            {
                if (FAILED(roundedGeometry->get_Offset(&offset)) || FAILED(roundedGeometry->get_Size(&size))
                    || FAILED(roundedGeometry->get_CornerRadius(&radius)))
                {
                    return false;
                }
                for (int i = 0; i < 4; ++i)
                {
                    radii[i * 2] = radius.X;
                    radii[i * 2 + 1] = radius.Y;
                }
            }
            else if (SUCCEEDED(geometry.As(&ellipseGeometry)))
            {
                wfn::Vector2 center{};
                if (FAILED(ellipseGeometry->get_Center(&center)) || FAILED(ellipseGeometry->get_Radius(&radius)))
                {
                    return false;
                }
                info->hasClip = true;
                info->clipRect = { center.X - radius.X, center.Y - radius.Y, center.X + radius.X, center.Y + radius.Y };
                AppendEllipse(center.X, center.Y, std::fabs(radius.X), std::fabs(radius.Y), info->clipVerbs, info->clipPoints);
                return true;
            }
            else
            {
                return false; // CompositionPathGeometry (its path cannot be read back), line geometry
            }
            rect = { offset.X, offset.Y, offset.X + size.X, offset.Y + size.Y };
        }
        else
        {
            return false;
        }

        info->hasClip = true;
        info->clipRect = rect;
        info->clipIsRect = std::all_of(std::begin(radii), std::end(radii), [](float r) { return r <= 0; });
        ImmediateRenderingHandOff::AppendRoundedRect(rect, radii, info->clipVerbs, info->clipPoints);
        return true;
    }
}

namespace ImmediateRenderingHandOff
{
    void AppendRoundedRect(const RectF& rect, const float radii[8], std::vector<PathVerb>& verbs, std::vector<PointF>& points)
    {
        const float width = std::max(0.0f, rect.right - rect.left);
        const float height = std::max(0.0f, rect.bottom - rect.top);
        // Corner radii are clamped like CSS/D2D: scale all of them down when adjacent radii overlap.
        float r[8];
        float shrink = 1;
        for (int i = 0; i < 8; ++i)
        {
            r[i] = std::max(0.0f, radii[i]);
        }
        auto limit = [&](float a, float b, float side) { if (a + b > side && a + b > 0) shrink = std::min(shrink, side / (a + b)); };
        limit(r[0], r[2], width);  // top
        limit(r[4], r[6], width);  // bottom
        limit(r[1], r[7], height); // left
        limit(r[3], r[5], height); // right
        for (float& v : r)
        {
            v *= shrink;
        }
        const float l = rect.left, t = rect.top, rt = rect.left + width, b = rect.top + height;
        verbs.push_back(PathVerb::MoveTo);
        points.push_back({ l + r[0], t });
        auto line = [&](float x, float y) { verbs.push_back(PathVerb::LineTo); points.push_back({ x, y }); };
        auto corner = [&](PointF c1, PointF c2, PointF end) { verbs.push_back(PathVerb::CubicTo); points.push_back(c1); points.push_back(c2); points.push_back(end); };
        line(rt - r[2], t);
        if (r[2] > 0 && r[3] > 0) corner({ rt - r[2] * (1 - c_kappa), t }, { rt, t + r[3] * (1 - c_kappa) }, { rt, t + r[3] }); else line(rt, t);
        line(rt, b - r[5]);
        if (r[4] > 0 && r[5] > 0) corner({ rt, b - r[5] * (1 - c_kappa) }, { rt - r[4] * (1 - c_kappa), b }, { rt - r[4], b }); else line(rt, b);
        line(l + r[6], b);
        if (r[6] > 0 && r[7] > 0) corner({ l + r[6] * (1 - c_kappa), b }, { l, b - r[7] * (1 - c_kappa) }, { l, b - r[7] }); else line(l, b);
        line(l, t + r[1]);
        if (r[0] > 0 && r[1] > 0) corner({ l, t + r[1] * (1 - c_kappa) }, { l + r[0] * (1 - c_kappa), t }, { l + r[0], t }); else line(l, t);
        verbs.push_back(PathVerb::Close);
    }

    bool Read(_In_ CUIElement* element, float width, float height, bool checkAnimations, _Out_ VisualInfo* info) noexcept
    {
        *info = {};
        ComPtr<WUComp::IVisual> visual;
        element->GetStoredHandOffVisual(visual.ReleaseAndGetAddressOf());
        if (visual == nullptr)
        {
            return false;
        }

        auto& state = info->state;
        wfn::Vector3 v{};
        wfn::Vector2 v2{};
        IGNOREHR(visual->get_Opacity(&state.opacity));
        IGNOREHR(visual->get_RotationAngle(&state.rotationAngle));
        if (SUCCEEDED(visual->get_Offset(&v))) { state.offset[0] = v.X; state.offset[1] = v.Y; state.offset[2] = v.Z; }
        if (SUCCEEDED(visual->get_Scale(&v))) { state.scale[0] = v.X; state.scale[1] = v.Y; state.scale[2] = v.Z; }
        if (SUCCEEDED(visual->get_RotationAxis(&v))) { state.rotationAxis[0] = v.X; state.rotationAxis[1] = v.Y; state.rotationAxis[2] = v.Z; }
        if (SUCCEEDED(visual->get_CenterPoint(&v))) { state.centerPoint[0] = v.X; state.centerPoint[1] = v.Y; state.centerPoint[2] = v.Z; }
        if (SUCCEEDED(visual->get_AnchorPoint(&v2))) { state.anchorPoint[0] = v2.X; state.anchorPoint[1] = v2.Y; }
        if (SUCCEEDED(visual->get_Size(&v2))) { state.size[0] = v2.X; state.size[1] = v2.Y; }
        wfn::Quaternion q{ 0, 0, 0, 1 };
        if (SUCCEEDED(visual->get_Orientation(&q))) { state.orientation[0] = q.X; state.orientation[1] = q.Y; state.orientation[2] = q.Z; state.orientation[3] = q.W; }
        wfn::Matrix4x4 m{};
        if (SUCCEEDED(visual->get_TransformMatrix(&m)))
        {
            const float values[16] = { m.M11, m.M12, m.M13, m.M14, m.M21, m.M22, m.M23, m.M24, m.M31, m.M32, m.M33, m.M34, m.M41, m.M42, m.M43, m.M44 };
            std::copy(std::begin(values), std::end(values), std::begin(state.transformMatrix));
        }
        boolean visible = true;
        if (SUCCEEDED(visual->get_IsVisible(&visible)))
        {
            info->isVisible = !!visible;
        }

        // Translation set by the app through the visual's property set (SetIsTranslationEnabled). XAML's own
        // UIElement.Translation (painted by the walk) is excluded.
        ComPtr<WUComp::ICompositionObject> object;
        ComPtr<WUComp::ICompositionPropertySet> properties;
        if (SUCCEEDED(visual.As(&object)) && SUCCEEDED(object->get_Properties(&properties)) && properties != nullptr)
        {
            WUComp::CompositionGetValueStatus status{};
            wfn::Vector3 translation{};
            if (SUCCEEDED(properties->TryGetVector3(wrl_wrappers::HStringReference(L"Translation").Get(), &translation, &status))
                && status == WUComp::CompositionGetValueStatus_Succeeded)
            {
                const wfn::Vector3 xamlTranslation = element->GetTranslation();
                state.translation[0] = translation.X - xamlTranslation.X;
                state.translation[1] = translation.Y - xamlTranslation.Y;
                state.translation[2] = translation.Z - xamlTranslation.Z;
            }
            if (checkAnimations)
            {
                static const wchar_t* const c_setProperties[] = { L"Translation" };
                state.animating = IsAnimating(properties.Get(), c_setProperties, ARRAYSIZE(c_setProperties), &info->animatedProperty);
            }
        }
        if (checkAnimations && !state.animating)
        {
            static const wchar_t* const c_visualProperties[] = {
                L"Offset", L"Offset.X", L"Offset.Y", L"Scale", L"Scale.X", L"Scale.Y", L"RotationAngle", L"RotationAngleInDegrees",
                L"Orientation", L"TransformMatrix", L"Opacity", L"CenterPoint", L"CenterPoint.X", L"CenterPoint.Y",
                L"AnchorPoint", L"Size", L"RotationAxis", L"Translation", L"Translation.X", L"Translation.Y",
            };
            state.animating = IsAnimating(visual.Get(), c_visualProperties, ARRAYSIZE(c_visualProperties), &info->animatedProperty);
        }

        ComPtr<WUComp::ICompositionClip> clip;
        if (SUCCEEDED(visual->get_Clip(&clip)) && clip != nullptr)
        {
            state.unsupportedClip = !ReadClip(clip.Get(), width, height, info);
            if (checkAnimations && !state.animating)
            {
                static const wchar_t* const c_clipProperties[] = {
                    L"LeftInset", L"TopInset", L"RightInset", L"BottomInset", L"Left", L"Top", L"Right", L"Bottom",
                    L"TopLeftRadius", L"TopRightRadius", L"BottomRightRadius", L"BottomLeftRadius",
                    L"Offset", L"Scale", L"RotationAngle", L"CenterPoint", L"TransformMatrix",
                };
                state.animating = IsAnimating(clip.Get(), c_clipProperties, ARRAYSIZE(c_clipProperties), &info->animatedProperty);
            }
            if (state.unsupportedClip)
            {
                info->hasClip = false;
                info->clipVerbs.clear();
                info->clipPoints.clear();
            }
        }
        return true;
    }
}
