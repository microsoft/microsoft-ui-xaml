// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealTestApi.h"
#include "RevealHoverLight.h"
#include "RevealBorderLight.h"

#include "RevealTestApi.properties.cpp"

RevealTestApi::RevealTestApi()
{
#ifndef BUILD_WINDOWS
    MUXControlsFactory::EnsureInitialized(); // Just in case we are called earlier than app initialization.
#endif
}

winrt::XamlLight RevealTestApi::GetWindowLightAt(uint32_t i)
{
    if (auto root = winrt::Window::Current().Content())
    {
        // Walk up to find the lights.
        auto current = root;
        auto parent = current;
        do
        {
            current = parent;
            parent = winrt::VisualTreeHelper::GetParent(current).as<winrt::UIElement>();
        } while (parent != nullptr);

        winrt::XamlLight light = nullptr;
        if (auto lights = current.Lights())
        {
            light = lights.GetAt(i);
        }

        return light;
    }

    return nullptr;
}


winrt::SpotLight RevealTestApi::BackgroundLight()
{
    return GetBackgroundSpotlightProxy(m_targetTheme);
}

winrt::XamlLight RevealTestApi::BorderLight()
{
    return GetWindowLightAt((m_targetTheme == winrt::ApplicationTheme::Light) ? 0 : 2);
}

winrt::XamlLight RevealTestApi::BorderWideLight()
{
    return GetWindowLightAt((m_targetTheme == winrt::ApplicationTheme::Light) ? 1 : 3);
}

winrt::ApplicationTheme RevealTestApi::TargetTheme()
{
    return m_targetTheme;
}

void RevealTestApi::TargetTheme(winrt::ApplicationTheme const& value)
{
    m_targetTheme = value;
}

double RevealTestApi::BackgroundLightMinSize()
{
#if DBG
    return RevealHoverLight::s_lightMinSize;
#else
    return 0;
#endif
}

void RevealTestApi::BackgroundLightMinSize(double value)
{
#if DBG
    RevealHoverLight::s_lightMinSize = (float)value;
#endif
}

double RevealTestApi::BackgroundLightMaxSize()
{
#if DBG
    return RevealHoverLight::s_lightMaxSize;
#else
    return 0;
#endif
}

void RevealTestApi::BackgroundLightMaxSize(double value)
{
#if DBG
    RevealHoverLight::s_lightMaxSize = (float)value;
#endif
}

winrt::SpotLight RevealTestApi::GetSpotLight(winrt::XamlLight const& value)
{
    return value.as<winrt::IXamlLightProtected>().CompositionLight().as<winrt::SpotLight>();
}

winrt::ExpressionAnimation RevealTestApi::GetHoverLightOffsetExpression(winrt::RevealHoverLight const& value)
{
    return winrt::get_self<RevealHoverLight>(value)->m_offsetAnimation;
}

winrt::RevealBorderLight RevealTestApi::GetAsRevealBorderLight(winrt::XamlLight const& value)
{
    return value.as<winrt::RevealBorderLight>();
}

winrt::RevealHoverLight RevealTestApi::GetAsRevealHoverLight(winrt::XamlLight const& value)
{
    return value.as<winrt::RevealHoverLight>();
}

bool RevealTestApi::BorderLight_ShouldBeOn(winrt::RevealBorderLight const& value)
{
    return winrt::get_self<RevealBorderLight>(value)->GetShouldLightBeOn();
}

#ifdef BUILD_WINDOWS
winrt::SharedLight RevealTestApi::GetSharedLight(winrt::RevealBorderLight const& value)
{
    return winrt::get_self<RevealBorderLight>(value)->GetSharedLight();
}

bool RevealTestApi::BorderLight_FallbackToLocalLight(winrt::RevealBorderLight const& value)
{
    return winrt::get_self<RevealBorderLight>(value)->GetFallbackToLocalLight();
}
#endif

bool RevealTestApi::HoverLight_ShouldBeOn(winrt::RevealHoverLight const& value)
{
    return winrt::get_self<RevealHoverLight>(value)->m_shouldLightBeOn;
}

bool RevealTestApi::HoverLight_IsPressed(winrt::RevealHoverLight const& value)
{
    return winrt::get_self<RevealHoverLight>(value)->m_isPressed;
}

bool RevealTestApi::HoverLight_IsPointerOver(winrt::RevealHoverLight const& value)
{
    return winrt::get_self<RevealHoverLight>(value)->m_isPointerOver;
}

#if DBG
winrt::ICoreWindow::PointerMoved_revoker RevealTestApi::s_pointerMovedRevoker{};
winrt::SpotLight RevealTestApi::s_backgroundSpotlightProxy[2] = { nullptr, nullptr };
#endif

winrt::SpotLight RevealTestApi::GetBackgroundSpotlightProxy(winrt::ApplicationTheme theme)
{
#if DBG
    int index = std::clamp((int)theme, 0, 1);
    if (!s_backgroundSpotlightProxy[index])
    {
        auto light = s_backgroundSpotlightProxy[index] = winrt::Window::Current().Compositor().CreateSpotLight();

        light.InnerConeAngleInDegrees(RevealHoverLight::s_innerConeAngleInDegrees);
        light.OuterConeAngleInDegrees(RevealHoverLight::s_outerConeAngleInDegrees);
        light.ConstantAttenuation(RevealHoverLight::s_constantAttenuation);
        light.InnerConeColor(RevealHoverLight::s_revealHoverSpotlightStates[RevealHoverSpotlightState_AnimToOff].InnerConeColor.Value);
        light.OuterConeColor(RevealHoverLight::s_revealHoverSpotlightStates[RevealHoverSpotlightState_AnimToOff].OuterConeColor.Value);

        s_pointerMovedRevoker = winrt::Window::Current().CoreWindow().PointerMoved(winrt::auto_revoke,
            [=](const winrt::CoreWindow&, const winrt::PointerEventArgs&)
            {
                RevealHoverLight::s_revealHoverSpotlightStates[RevealHoverSpotlightState_AnimToOff].InnerConeColor.Value = light.InnerConeColor();
                RevealHoverLight::s_revealHoverSpotlightStates[RevealHoverSpotlightState_AnimToOff].OuterConeColor.Value = light.OuterConeColor();
            });
    }

    return s_backgroundSpotlightProxy[index];
#else
    return nullptr;
#endif
}
