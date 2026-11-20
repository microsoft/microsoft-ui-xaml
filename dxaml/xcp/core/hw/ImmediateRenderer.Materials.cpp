// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// S13 materials for the immediate render walk: LinearGradientBrush, in-app AcrylicBrush and ThemeShadow mapped to
// painter primitives. See docs/design-notes/immediate-rendering-backends-proposal.md, "Full-backend decisions (S11-S23)".

#include "precomp.h"
#include "ImmediateRenderer.Materials.h"
#include <FxCallbacks.h>
#include <MetadataAPI.h>
#include <XamlCompositionBrush.h>
#include <FrameworkTheming.h>
#include "LinearGradientBrush.h"
#include "ThemeShadow.h"
#include "Transform.h"
#include "rectangle.h"
#include <algorithm>

using namespace RenderingBackend;

namespace
{
    ColorF FromArgb(uint32_t argb, float opacity) noexcept
    {
        ColorF c;
        c.a = ((argb >> 24) & 0xff) / 255.0f * opacity;
        c.r = ((argb >> 16) & 0xff) / 255.0f;
        c.g = ((argb >> 8) & 0xff) / 255.0f;
        c.b = (argb & 0xff) / 255.0f;
        return c;
    }

    Matrix3x2F TransformOf(const xref_ptr<CTransform>& transform) noexcept
    {
        if (transform == nullptr)
        {
            return {};
        }
        CMILMatrix m(true);
        transform->GetTransform(&m);
        return { m.GetM11(), m.GetM12(), m.GetM21(), m.GetM22(), m.GetDx(), m.GetDy() };
    }

    bool IsHighContrast(_In_ CDependencyObject* object) noexcept
    {
        CCoreServices* core = object->GetContext();
        return core != nullptr && core->GetFrameworkTheming() != nullptr && core->GetFrameworkTheming()->HasHighContrastTheme();
    }

    bool TryGetDouble(const CValue& value, _Out_ double* result) noexcept
    {
        *result = 0;
        if (value.GetType() == valueDouble)
        {
            *result = value.AsDouble();
            return true;
        }
        if (value.GetType() == valueFloat)
        {
            *result = value.AsFloat();
            return true;
        }
        if (value.GetType() == valueIInspectable && value.AsIInspectable() != nullptr)
        {
            Microsoft::WRL::ComPtr<wf::IReference<double>> reference;
            return SUCCEEDED(value.AsIInspectable()->QueryInterface(IID_PPV_ARGS(&reference))) && SUCCEEDED(reference->get_Value(result));
        }
        return false;
    }

    bool TryGetColor(const CValue& value, _Out_ uint32_t* argb) noexcept
    {
        *argb = 0;
        if (value.GetType() == valueColor)
        {
            *argb = value.AsColor();
            return true;
        }
        if (value.GetType() == valueIInspectable && value.AsIInspectable() != nullptr)
        {
            Microsoft::WRL::ComPtr<wf::IReference<wu::Color>> reference;
            wu::Color color{};
            if (SUCCEEDED(value.AsIInspectable()->QueryInterface(IID_PPV_ARGS(&reference))) && SUCCEEDED(reference->get_Value(&color)))
            {
                *argb = (static_cast<uint32_t>(color.A) << 24) | (static_cast<uint32_t>(color.R) << 16) | (static_cast<uint32_t>(color.G) << 8) | color.B;
                return true;
            }
        }
        return false;
    }

    DECLARE_CONST_XSTRING_PTR_STORAGE(c_acrylicBrushStorage, L"Microsoft.UI.Xaml.Media.AcrylicBrush");

    bool IsAcrylicBrush(_In_ CBrush* brush) noexcept
    {
        const CClassInfo* info = brush->GetClassInformation();
        if (info != nullptr)
        {
            const xstring_ptr name = info->GetFullName();
            if (!name.IsNullOrEmpty() && wcsstr(name.GetBuffer(), L"AcrylicBrush") != nullptr)
            {
                return true;
            }
        }
        return brush->HasManagedPeer()
            && SUCCEEDED(FxCallbacks::FrameworkCallbacks_CheckPeerType(brush, XSTRING_PTR_FROM_STORAGE(c_acrylicBrushStorage), FALSE));
    }
}

namespace ImmediateRenderingMaterials
{
    bool TryGetCustomValue(_In_ CDependencyObject* object, _In_ const xstring_ptr& typeName, _In_ const xstring_ptr& propertyName, _Out_ CValue* value) noexcept
    {
        const CClassInfo* type = nullptr;
        const CDependencyProperty* property = nullptr;
        return SUCCEEDED(DirectUI::MetadataAPI::GetClassInfoByFullName(typeName, &type)) && type != nullptr
            && SUCCEEDED(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(type, propertyName, &property)) && property != nullptr
            && SUCCEEDED(object->GetValue(property, value));
    }

    FillKind ResolveLinearGradient(_In_opt_ CBrush* brush, const RectF& bounds, float opacity, _Out_ GradientFill* gradient, _Out_ ColorF* solid)
    {
        *solid = {};
        gradient->stops.clear();
        gradient->detail = nullptr;
        auto linear = do_pointer_cast<CLinearGradientBrush>(brush);
        if (linear == nullptr)
        {
            return FillKind::NotHandled;
        }
        gradient->opacity = linear->m_eOpacity * opacity;
        if (linear->m_pStops != nullptr)
        {
            for (CDependencyObject* item : *linear->m_pStops)
            {
                if (auto stop = do_pointer_cast<CGradientStop>(item))
                {
                    gradient->stops.push_back({ stop->m_stop.rPosition, FromArgb(stop->m_rgb, 1.0f) });
                }
            }
        }
        if (gradient->stops.empty() || !(gradient->opacity > 0))
        {
            return FillKind::None;
        }
        std::stable_sort(gradient->stops.begin(), gradient->stops.end(), [](const GradientStop& a, const GradientStop& b) { return a.offset < b.offset; });

        ImmediateRenderingPolicy::LinearGradientInput input;
        input.start = { linear->m_ptStart.x, linear->m_ptStart.y };
        input.end = { linear->m_ptEnd.x, linear->m_ptEnd.y };
        input.relativeToBoundingBox = linear->m_nMapping == XcpRelative;
        input.bounds = bounds;
        input.relativeTransform = TransformOf(linear->GetRelativeTransform());
        input.transform = TransformOf(linear->GetTransform());
        if (gradient->stops.size() == 1 || !ImmediateRenderingPolicy::MapLinearGradient(input, &gradient->start, &gradient->end))
        {
            *solid = gradient->stops.back().color;
            solid->a *= gradient->opacity;
            return solid->a > 0 ? FillKind::Solid : FillKind::None;
        }
        if (linear->m_nSpread != XcpGradientWrapModeExtend)
        {
            // Painters only clamp; Reflect/Repeat outside [start, end] shows the edge stops.
            gradient->detail = L"LinearGradientBrush SpreadMethod:approximated";
        }
        if (linear->m_nInterpolate != XcpColorInterpolationModeSRgbLinearInterpolation)
        {
            gradient->detail = L"LinearGradientBrush ColorInterpolationMode:approximated";
        }
        return FillKind::Gradient;
    }

    bool ResolveAcrylic(_In_opt_ CBrush* brush, float opacity, _Out_ AcrylicFill* acrylic)
    {
        *acrylic = {};
        auto composition = do_pointer_cast<CXamlCompositionBrush>(brush);
        if (composition == nullptr || !IsAcrylicBrush(brush))
        {
            return false;
        }
        const float brushOpacity = brush->m_eOpacity * opacity;
        acrylic->fallback = FromArgb(composition->m_fallbackColor, brushOpacity);
        if (IsHighContrast(brush))
        {
            return true;
        }

        const xstring_ptr c_acrylicBrush = XSTRING_PTR_FROM_STORAGE(c_acrylicBrushStorage);
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_alwaysUseFallback, L"AlwaysUseFallback");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_tintColor, L"TintColor");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_tintOpacity, L"TintOpacity");
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_tintLuminosityOpacity, L"TintLuminosityOpacity");
        CValue value;
        if (TryGetCustomValue(brush, c_acrylicBrush, c_alwaysUseFallback, &value) && value.GetType() == valueBool && value.AsBool())
        {
            return true;
        }
        uint32_t tintArgb = 0;
        double tintOpacity = 0;
        value.SetNull();
        if (!TryGetCustomValue(brush, c_acrylicBrush, c_tintColor, &value) || !TryGetColor(value, &tintArgb))
        {
            acrylic->detail = L"AcrylicBrush:fallback"; // unreadable: keep the fallback color
            return true;
        }
        value.SetNull();
        if (!TryGetCustomValue(brush, c_acrylicBrush, c_tintOpacity, &value) || !TryGetDouble(value, &tintOpacity))
        {
            acrylic->detail = L"AcrylicBrush:fallback";
            return true;
        }
        double luminosityOpacity = 0;
        value.SetNull();
        const bool hasLuminosity = TryGetCustomValue(brush, c_acrylicBrush, c_tintLuminosityOpacity, &value) && TryGetDouble(value, &luminosityOpacity);

        const ImmediateRenderingPolicy::AcrylicLayers layers = ImmediateRenderingPolicy::GetAcrylicLayers(
            FromArgb(tintArgb, 1.0f), static_cast<float>(tintOpacity), hasLuminosity, static_cast<float>(luminosityOpacity));
        acrylic->blur = true;
        acrylic->luminosity = layers.luminosity;
        acrylic->tint = layers.tint;
        acrylic->luminosity.a *= brushOpacity;
        acrylic->tint.a *= brushOpacity;
        return true;
    }

    bool ResolveThemeShadow(_In_ CUIElement* element, _Out_ ThemeShadowFill* shadow)
    {
        *shadow = {};
        if (!CThemeShadow::IsDropShadowMode() || IsHighContrast(element) || element->OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            return false;
        }
        CUIElement* caster = element->IsShadowCaster() ? element : nullptr;
        CUIElement* parent = element->GetUIElementAdjustedParentInternal();
        if (parent != nullptr && parent->OfTypeByIndex<KnownTypeIndex::Popup>() && parent->IsShadowCaster())
        {
            caster = parent;
        }
        if (caster == nullptr)
        {
            return false;
        }

        bool dark = false;
        if (element->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
        {
            CValue value;
            if (SUCCEEDED(element->GetValueByIndex(KnownPropertyIndex::FrameworkElement_ActualTheme, &value)))
            {
                dark = static_cast<DirectUI::ElementTheme>(value.AsEnum()) == DirectUI::ElementTheme::Dark;
            }
        }
        shadow->count = ImmediateRenderingPolicy::GetThemeShadowLayers(caster->GetTranslation().Z, dark, shadow->layers);
        if (shadow->count == 0)
        {
            return false;
        }

        // Corner radius: the first rounded element from the caster up (HWCompTreeNodeWinRT::UpdateDropShadowRecipeCornerRadius).
        for (CUIElement* e = element; e != nullptr; e = e->GetUIElementAdjustedParentInternal(false))
        {
            float radius = 0;
            if (e->OfTypeByIndex<KnownTypeIndex::Rectangle>())
            {
                auto rectangle = static_cast<CRectangle*>(e);
                if (rectangle->m_eRadiusX != 0 || rectangle->m_eRadiusY != 0)
                {
                    shadow->radiusX = rectangle->m_eRadiusX;
                    shadow->radiusY = rectangle->m_eRadiusY;
                    break;
                }
                continue;
            }
            XCORNERRADIUS corners{};
            if (e->OfTypeByIndex<KnownTypeIndex::Control>())
            {
                CValue value;
                if (SUCCEEDED(e->GetValueByIndex(KnownPropertyIndex::Control_CornerRadius, &value)) && value.AsCornerRadius() != nullptr)
                {
                    corners = *value.AsCornerRadius();
                }
            }
            else if (e->OfTypeByIndex<KnownTypeIndex::FrameworkElement>())
            {
                corners = static_cast<CFrameworkElement*>(e)->GetCornerRadius();
            }
            radius = std::max(std::max(corners.topLeft, corners.topRight), std::max(corners.bottomLeft, corners.bottomRight));
            if (radius != 0)
            {
                shadow->radiusX = shadow->radiusY = radius;
                break;
            }
        }
        return true;
    }
}
