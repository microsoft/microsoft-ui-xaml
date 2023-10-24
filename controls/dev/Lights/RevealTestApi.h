// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "RevealTestApi.g.h"

class RevealTestApi :
    public ReferenceTracker<RevealTestApi, winrt::implementation::RevealTestApiT, winrt::composable>
{
public:
    winrt::ApplicationTheme TargetTheme();
    void TargetTheme(winrt::ApplicationTheme const& value);
    winrt::SpotLight BackgroundLight();
    winrt::XamlLight BorderLight();
    winrt::XamlLight BorderWideLight();

    double BackgroundLightMinSize();
    void BackgroundLightMinSize(double value);
    double BackgroundLightMaxSize();
    void BackgroundLightMaxSize(double value);

    winrt::SpotLight GetSpotLight(winrt::XamlLight const& value);
    winrt::ExpressionAnimation GetHoverLightOffsetExpression(winrt::RevealHoverLight const& value);
    winrt::RevealBorderLight GetAsRevealBorderLight(winrt::XamlLight const& value);
    winrt::RevealHoverLight GetAsRevealHoverLight(winrt::XamlLight const& value);

    bool BorderLight_ShouldBeOn(winrt::RevealBorderLight const& value);
    bool HoverLight_ShouldBeOn(winrt::RevealHoverLight const& value);
    bool HoverLight_IsPressed(winrt::RevealHoverLight const& value);
    bool HoverLight_IsPointerOver(winrt::RevealHoverLight const& value);
#ifdef BUILD_WINDOWS
    winrt::SharedLight GetSharedLight(winrt::RevealBorderLight const& value);
    bool BorderLight_FallbackToLocalLight(winrt::RevealBorderLight const& value);
#endif

    RevealTestApi();

    static winrt::SpotLight GetBackgroundSpotlightProxy(winrt::ApplicationTheme theme);

private:
#if DBG
    static winrt::ICoreWindow::PointerMoved_revoker s_pointerMovedRevoker;
    static winrt::SpotLight s_backgroundSpotlightProxy[2];
#endif

    winrt::XamlLight GetWindowLightAt(uint32_t i);

    winrt::ApplicationTheme m_targetTheme{};
};
