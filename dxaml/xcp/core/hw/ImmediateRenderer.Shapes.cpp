// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// F1 shapes and strokes, F2 brushes for the immediate render walk: XAML brushes and Shape stroke properties mapped to
// painter BrushDesc / StrokeStyleDesc. See docs/design-notes/immediate-rendering-backends-proposal.md,
// "Full XAML coverage: shapes".

#include "precomp.h"
#include "ImmediateRenderer.Shapes.h"
#include "ImmediateRenderer.Materials.h"
#include <ImmediateRenderingPolicy.h>
#include <FxCallbacks.h>
#include <MetadataAPI.h>
#include <CustomDependencyProperty.h>
#include <XamlCompositionBrush.h>
#include "LinearGradientBrush.h"
#include "SolidColorBrush.h"
#include "Transform.h"
#include <BrushTypeUtils.h>
#include <algorithm>
#include <cmath>

using namespace RenderingBackend;
namespace irs = ImmediateRenderingShapes;

namespace
{
    DECLARE_CONST_XSTRING_PTR_STORAGE(c_radialGradientBrushStorage, L"Microsoft.UI.Xaml.Media.RadialGradientBrush");

    ColorF FromArgb(uint32_t argb) noexcept
    {
        return { ((argb >> 16) & 0xff) / 255.0f, ((argb >> 8) & 0xff) / 255.0f, (argb & 0xff) / 255.0f, ((argb >> 24) & 0xff) / 255.0f };
    }

    void SortStops(std::vector<GradientStop>& stops)
    {
        std::stable_sort(stops.begin(), stops.end(), [](const GradientStop& a, const GradientStop& b) { return a.offset < b.offset; });
    }

    // Degenerate gradients fill with the last stop color (what Composition shows).
    irs::PaintKind Solid(const ColorF& color, float opacity, _Out_ irs::Paint* paint) noexcept
    {
        paint->brush = {};
        paint->stops.clear();
        paint->brush.kind = BrushKind::Solid;
        paint->brush.color = color;
        paint->brush.color.a *= opacity;
        return paint->brush.color.a > 0 ? irs::PaintKind::Paint : irs::PaintKind::None;
    }

    bool TryGetDouble(const CValue& value, _Out_ double* result) noexcept
    {
        *result = 0;
        switch (value.GetType())
        {
            case valueDouble: *result = value.AsDouble(); return true;
            case valueFloat: *result = value.AsFloat(); return true;
            case valueSigned: *result = value.AsSigned(); return true;
            default: break;
        }
        if (value.GetType() == valueIInspectable && value.AsIInspectable() != nullptr)
        {
            Microsoft::WRL::ComPtr<wf::IPropertyValue> property;
            return SUCCEEDED(value.AsIInspectable()->QueryInterface(IID_PPV_ARGS(&property))) && SUCCEEDED(property->GetDouble(result));
        }
        return false;
    }

    bool TryGetInt(const CValue& value, _Out_ int32_t* result) noexcept
    {
        *result = 0;
        switch (value.GetType())
        {
            case valueEnum: *result = static_cast<int32_t>(value.AsEnum()); return true;
            case valueEnum8: *result = static_cast<int32_t>(value.AsEnum8()); return true;
            case valueSigned: *result = value.AsSigned(); return true;
            default: break;
        }
        if (value.GetType() == valueIInspectable && value.AsIInspectable() != nullptr)
        {
            Microsoft::WRL::ComPtr<wf::IPropertyValue> property;
            if (FAILED(value.AsIInspectable()->QueryInterface(IID_PPV_ARGS(&property))))
            {
                return false;
            }
            UINT32 u = 0;
            if (SUCCEEDED(property->GetInt32(result)))
            {
                return true;
            }
            if (SUCCEEDED(property->GetUInt32(&u)))
            {
                *result = static_cast<int32_t>(u);
                return true;
            }
        }
        return false;
    }

    bool TryGetPoint(const CValue& value, _Out_ PointF* result) noexcept
    {
        *result = {};
        if (value.GetType() == valuePoint && value.As<valuePoint>() != nullptr)
        {
            const XPOINTF* p = value.As<valuePoint>();
            *result = { p->x, p->y };
            return true;
        }
        if (value.GetType() == valueIInspectable && value.AsIInspectable() != nullptr)
        {
            Microsoft::WRL::ComPtr<wf::IPropertyValue> property;
            wf::Point point{};
            if (SUCCEEDED(value.AsIInspectable()->QueryInterface(IID_PPV_ARGS(&property))) && SUCCEEDED(property->GetPoint(&point)))
            {
                *result = { point.X, point.Y };
                return true;
            }
        }
        return false;
    }

    bool IsRadialGradientBrush(_In_ CBrush* brush) noexcept
    {
        if (do_pointer_cast<CXamlCompositionBrush>(brush) == nullptr)
        {
            return false;
        }
        const CClassInfo* info = brush->GetClassInformation();
        if (info != nullptr)
        {
            const xstring_ptr name = info->GetFullName();
            if (!name.IsNullOrEmpty() && wcsstr(name.GetBuffer(), L"RadialGradientBrush") != nullptr)
            {
                return true;
            }
        }
        return brush->HasManagedPeer()
            && SUCCEEDED(FxCallbacks::FrameworkCallbacks_CheckPeerType(brush, XSTRING_PTR_FROM_STORAGE(c_radialGradientBrushStorage), FALSE));
    }

    // MUXC RadialGradientBrush.GradientStops is a plain (non-dependency) property of the managed peer: read it through
    // the type's IXamlMember.
    bool ReadRadialStops(_In_ CBrush* brush, _Inout_ std::vector<GradientStop>& stops) noexcept
    {
        const CClassInfo* type = nullptr;
        const CDependencyProperty* property = nullptr;
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_gradientStops, L"GradientStops");
        if (FAILED(DirectUI::MetadataAPI::GetClassInfoByFullName(XSTRING_PTR_FROM_STORAGE(c_radialGradientBrushStorage), &type)) || type == nullptr
            || FAILED(DirectUI::MetadataAPI::TryGetPropertyByName(type, c_gradientStops, &property)) || property == nullptr)
        {
            return false;
        }
        const CCustomProperty* custom = property->AsOrNull<CCustomProperty>();
        if (custom == nullptr || custom->GetXamlPropertyNoRef() == nullptr || brush->GetDXamlPeer() == nullptr)
        {
            return false;
        }
        Microsoft::WRL::ComPtr<IInspectable> instance;
        Microsoft::WRL::ComPtr<IInspectable> result;
        Microsoft::WRL::ComPtr<wfc::IVector<xaml_media::GradientStop*>> vector;
        UINT32 size = 0;
        if (FAILED(reinterpret_cast<IUnknown*>(brush->GetDXamlPeer())->QueryInterface(IID_PPV_ARGS(&instance)))
            || FAILED(custom->GetXamlPropertyNoRef()->GetValue(instance.Get(), &result)) || result == nullptr
            || FAILED(result.As(&vector)) || FAILED(vector->get_Size(&size)))
        {
            return false;
        }
        for (UINT32 i = 0; i < size; ++i)
        {
            Microsoft::WRL::ComPtr<xaml_media::IGradientStop> stop;
            wu::Color color{};
            DOUBLE offset = 0;
            if (SUCCEEDED(vector->GetAt(i, &stop)) && stop != nullptr && SUCCEEDED(stop->get_Color(&color)) && SUCCEEDED(stop->get_Offset(&offset)))
            {
                stops.push_back({ static_cast<float>(offset), { color.R / 255.0f, color.G / 255.0f, color.B / 255.0f, color.A / 255.0f } });
            }
        }
        return true;
    }

    irs::PaintKind ResolveRadial(_In_ CBrush* brush, const RectF& bounds, float opacity, _Out_ irs::Paint* paint, _Outptr_result_maybenull_ const wchar_t** detail)
    {
        namespace irm = ImmediateRenderingMaterials;
        const xstring_ptr typeName = XSTRING_PTR_FROM_STORAGE(c_radialGradientBrushStorage);
        auto get = [&](const wchar_t* name, size_t length, CValue* value)
        {
            xstring_ptr property;
            return SUCCEEDED(xstring_ptr::CloneBuffer(name, static_cast<uint32_t>(length), &property))
                && irm::TryGetCustomValue(brush, typeName, property, value);
        };
        CValue value;
        PointF center{ 0.5f, 0.5f };
        PointF origin{ 0.5f, 0.5f };
        double radiusX = 0.5, radiusY = 0.5;
        int32_t mapping = 1;        // BrushMappingMode::RelativeToBoundingBox
        int32_t space = 0;          // CompositionColorSpace::Auto
        int32_t spread = 0;         // GradientSpreadMethod::Pad
        if (get(L"Center", 6, &value)) { TryGetPoint(value, &center); }
        if (get(L"GradientOrigin", 14, &value)) { TryGetPoint(value, &origin); }
        if (get(L"RadiusX", 7, &value)) { TryGetDouble(value, &radiusX); }
        if (get(L"RadiusY", 7, &value)) { TryGetDouble(value, &radiusY); }
        if (get(L"MappingMode", 11, &value)) { TryGetInt(value, &mapping); }
        if (get(L"InterpolationSpace", 18, &value)) { TryGetInt(value, &space); }
        if (get(L"SpreadMethod", 12, &value)) { TryGetInt(value, &spread); }

        paint->brush = {};
        paint->stops.clear();
        paint->brush.opacity = brush->m_eOpacity * opacity;
        if (!ReadRadialStops(brush, paint->stops))
        {
            // Stops unreadable: keep the brush's fallback color rendering.
            *detail = L"RadialGradientBrush stops:fallback";
            auto composition = static_cast<CXamlCompositionBrush*>(brush);
            return Solid(FromArgb(composition->m_fallbackColor), paint->brush.opacity, paint);
        }
        if (paint->stops.empty() || !(paint->brush.opacity > 0))
        {
            return irs::PaintKind::None;
        }
        SortStops(paint->stops);
        if (paint->stops.size() == 1 || !(radiusX > 0) || !(radiusY > 0))
        {
            return Solid(paint->stops.back().color, paint->brush.opacity, paint);
        }
        paint->brush.kind = BrushKind::Radial;
        paint->brush.center = center;
        paint->brush.origin = origin;
        paint->brush.radiusX = static_cast<float>(radiusX);
        paint->brush.radiusY = static_cast<float>(radiusY);
        paint->brush.spread = spread == 1 ? GradientSpread::Reflect : spread == 2 ? GradientSpread::Repeat : GradientSpread::Pad;
        // CompositionColorSpace: Auto 0, Hsl 1, Rgb 2, HslLinear 3, RgbLinear 4.
        paint->brush.interpolation = (space == 3 || space == 4) ? GradientInterpolation::Linear : GradientInterpolation::SRgb;
        if (space == 1 || space == 3)
        {
            *detail = L"RadialGradientBrush HSL interpolation:approximated";
        }
        if (mapping != 0)
        {
            // Relative: brush space is the unit square of the bounds (Composition maps it to the visual's size).
            paint->brush.transform = { bounds.right - bounds.left, 0, 0, bounds.bottom - bounds.top, bounds.left, bounds.top };
        }
        // XamlCompositionBrushBase rejects Transform/RelativeTransform, so there is nothing else to apply.
        return irs::PaintKind::Paint;
    }

    irs::PaintKind ResolveLinear(_In_ CLinearGradientBrush* linear, const RectF& bounds, float opacity, _Out_ irs::Paint* paint)
    {
        paint->brush = {};
        paint->stops.clear();
        paint->brush.opacity = linear->m_eOpacity * opacity;
        if (linear->m_pStops != nullptr)
        {
            for (CDependencyObject* item : *linear->m_pStops)
            {
                if (auto stop = do_pointer_cast<CGradientStop>(item))
                {
                    paint->stops.push_back({ stop->m_stop.rPosition, FromArgb(stop->m_rgb) });
                }
            }
        }
        if (paint->stops.empty() || !(paint->brush.opacity > 0))
        {
            return irs::PaintKind::None;
        }
        SortStops(paint->stops);

        // Like XAML (CLinearGradientBrush::D2DEnsureDeviceIndependentResources / GetWUCBrush): the gradient is
        // perpendicular to start-end in DIP space, and the combined brush transform (which may skew) maps it.
        const float width = bounds.right - bounds.left, height = bounds.bottom - bounds.top;
        PointF start{ linear->m_ptStart.x, linear->m_ptStart.y };
        PointF end{ linear->m_ptEnd.x, linear->m_ptEnd.y };
        if (linear->m_nMapping == XcpRelative)
        {
            start = { start.x * width + bounds.left, start.y * height + bounds.top };
            end = { end.x * width + bounds.left, end.y * height + bounds.top };
        }
        CMILMatrix relative(true), absolute(true), combined(true);
        const auto relativeTransform = linear->GetRelativeTransform();
        const auto transform = linear->GetTransform();
        if (relativeTransform != nullptr)
        {
            relativeTransform->GetTransform(&relative);
        }
        if (transform != nullptr)
        {
            transform->GetTransform(&absolute);
        }
        const XRECTF box{ bounds.left, bounds.top, width, height };
        CBrushTypeUtils::GetBrushTransform(relativeTransform != nullptr ? &relative : nullptr, transform != nullptr ? &absolute : nullptr, &box, &combined);
        const float dx = end.x - start.x, dy = end.y - start.y;
        const float det = combined.GetM11() * combined.GetM22() - combined.GetM12() * combined.GetM21();
        if (paint->stops.size() == 1 || !(dx * dx + dy * dy > 1e-12f) || !(std::abs(det) > 1e-12f) || !std::isfinite(det))
        {
            return Solid(paint->stops.back().color, paint->brush.opacity, paint);
        }
        paint->brush.kind = BrushKind::Linear;
        paint->brush.start = start;
        paint->brush.end = end;
        paint->brush.transform = { combined.GetM11(), combined.GetM12(), combined.GetM21(), combined.GetM22(), combined.GetDx(), combined.GetDy() };
        paint->brush.spread = linear->m_nSpread == XcpGradientWrapModeFlip ? GradientSpread::Reflect
            : linear->m_nSpread == XcpGradientWrapModeTile ? GradientSpread::Repeat : GradientSpread::Pad;
        paint->brush.interpolation = linear->m_nInterpolate == XcpColorInterpolationModeScRgbLinearInterpolation
            ? GradientInterpolation::Linear : GradientInterpolation::SRgb;
        return irs::PaintKind::Paint;
    }

    RenderingBackend::LineCap ToCap(XcpPenCap cap) noexcept
    {
        switch (cap)
        {
            case XcpPenCapSquare: return RenderingBackend::LineCap::Square;
            case XcpPenCapRound: return RenderingBackend::LineCap::Round;
            case XcpPenCapTriangle: return RenderingBackend::LineCap::Triangle;
            default: return RenderingBackend::LineCap::Flat;
        }
    }

    RenderingBackend::LineJoin ToJoin(XcpLineJoin join) noexcept
    {
        switch (join)
        {
            case XcpLineJoinBevel: return RenderingBackend::LineJoin::Bevel;
            case XcpLineJoinRound: return RenderingBackend::LineJoin::Round;
            default: return RenderingBackend::LineJoin::Miter;
        }
    }

    struct Hasher
    {
        uint64_t hash = 14695981039346656037ull;

        void Add(const void* data, size_t size) noexcept
        {
            auto bytes = static_cast<const uint8_t*>(data);
            for (size_t i = 0; i < size; ++i)
            {
                hash ^= bytes[i];
                hash *= 1099511628211ull;
            }
        }

        template <typename T> void AddValue(const T& value) noexcept { Add(&value, sizeof(value)); }
    };
}

namespace ImmediateRenderingShapes
{
    BrushDesc Paint::Bind(float opacity) const noexcept
    {
        BrushDesc result = brush;
        result.stops = stops.empty() ? nullptr : stops.data();
        result.stopCount = static_cast<uint32_t>(stops.size());
        result.opacity *= opacity;
        return result;
    }

    const StrokeStyleDesc* Paint::BindStyle(_Out_ StrokeStyleDesc* storage) const noexcept
    {
        *storage = style;
        storage->dashes = dashes.empty() ? nullptr : dashes.data();
        storage->dashCount = static_cast<uint32_t>(dashes.size());
        return hasStyle ? storage : nullptr;
    }

    uint64_t Paint::Hash() const noexcept
    {
        Hasher h;
        h.AddValue(brush.kind);
        h.AddValue(brush.color);
        h.AddValue(brush.start);
        h.AddValue(brush.end);
        h.AddValue(brush.center);
        h.AddValue(brush.origin);
        h.AddValue(brush.radiusX);
        h.AddValue(brush.radiusY);
        h.AddValue(brush.spread);
        h.AddValue(brush.interpolation);
        h.AddValue(brush.transform);
        h.AddValue(brush.opacity);
        h.Add(stops.data(), stops.size() * sizeof(GradientStop));
        h.AddValue(hasStyle);
        if (hasStyle)
        {
            h.AddValue(style.startCap);
            h.AddValue(style.endCap);
            h.AddValue(style.dashCap);
            h.AddValue(style.lineJoin);
            h.AddValue(style.miterLimit);
            h.AddValue(style.dashOffset);
            h.Add(dashes.data(), dashes.size() * sizeof(float));
        }
        return h.hash;
    }

    bool IsGradientBrush(_In_opt_ CBrush* brush) noexcept
    {
        return brush != nullptr && (do_pointer_cast<CLinearGradientBrush>(brush) != nullptr || IsRadialGradientBrush(brush));
    }

    PaintKind ResolvePaint(_In_opt_ CBrush* brush, const RectF& bounds, float opacity, _Out_ Paint* paint, _Outptr_result_maybenull_ const wchar_t** detail)
    {
        *detail = nullptr;
        if (brush == nullptr)
        {
            return PaintKind::NotHandled;
        }
        if (auto solid = do_pointer_cast<CSolidColorBrush>(brush))
        {
            return Solid(FromArgb(solid->m_rgb), solid->m_eOpacity * opacity, paint);
        }
        if (auto linear = do_pointer_cast<CLinearGradientBrush>(brush))
        {
            return ResolveLinear(linear, bounds, opacity, paint);
        }
        if (IsRadialGradientBrush(brush))
        {
            return ResolveRadial(brush, bounds, opacity, paint, detail);
        }
        return PaintKind::NotHandled;
    }

    void ResolveStrokeStyle(_In_ CShape* shape, float thickness, _Inout_ Paint* paint)
    {
        paint->style = {};
        paint->dashes.clear();
        paint->style.startCap = ToCap(shape->GetStrokeStartLineCap());
        paint->style.endCap = ToCap(shape->GetStrokeEndLineCap());
        paint->style.dashCap = ToCap(shape->GetStrokeDashCap());
        paint->style.lineJoin = ToJoin(shape->GetStrokeLineJoin());
        paint->style.miterLimit = shape->GetStrokeMiterLimit();
        if (shape->HasStrokeDashArray())
        {
            if (const xref_ptr<CDoubleCollection> array = shape->GetStrokeDashArray())
            {
                bool any = false;
                for (float dash : array->GetCollection())
                {
                    const float length = std::max(0.0f, dash) * thickness;
                    any = any || length > 0;
                    paint->dashes.push_back(length);
                }
                if (!any)
                {
                    // An all-zero pattern is invalid for D2D; treat it as solid.
                    paint->dashes.clear();
                }
            }
        }
        if (!paint->dashes.empty())
        {
            // Like the dash array, the offset is in units of the stroke thickness.
            paint->style.dashOffset = shape->GetStrokeDashOffset() * thickness;
        }
        const StrokeStyleDesc defaults;
        paint->hasStyle = !paint->dashes.empty() || paint->style.startCap != defaults.startCap || paint->style.endCap != defaults.endCap
            || paint->style.lineJoin != defaults.lineJoin || paint->style.miterLimit != defaults.miterLimit;
    }

    float StrokeReach(const Paint& paint, float thickness) noexcept
    {
        const float half = thickness / 2;
        const StrokeStyleDesc& style = paint.style;
        float reach = half;
        if (!paint.hasStyle || style.lineJoin == RenderingBackend::LineJoin::Miter)
        {
            reach = std::max(reach, half * std::max(1.0f, paint.hasStyle ? style.miterLimit : StrokeStyleDesc{}.miterLimit));
        }
        for (RenderingBackend::LineCap cap : { style.startCap, style.endCap, style.dashCap })
        {
            if (cap == RenderingBackend::LineCap::Square)
            {
                reach = std::max(reach, half * 1.4143f);
            }
        }
        return reach;
    }
}
