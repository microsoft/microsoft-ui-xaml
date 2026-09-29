// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include <winrt/Windows.UI.Xaml.Interop.h> // Declares winrt::xaml_typename (no longer in the PCH after #12054).

// Tightens a tool/menu button's L3 flyout to match WinUI 2 (zero presenter padding). The background,
// border and corner are inherited from the framework's default FlyoutPresenter style so they stay
// theme-aware (following RequestedTheme and theme switches). If that style is somehow not reachable,
// falls back to explicitly re-supplying the acrylic surface so the flyout is never left bare.
inline void ApplyInkToolbarFlyoutStyle(winrt::Flyout const& flyout)
{
    auto resources = winrt::Application::Current().Resources();

    winrt::Style style{ winrt::xaml_typename<winrt::Microsoft::UI::Xaml::Controls::FlyoutPresenter>() };
    auto setters = style.Setters();
    setters.Append(winrt::Setter{ winrt::Control::PaddingProperty(), winrt::box_value(winrt::Thickness{ 0, 0, 0, 0 }) });
    setters.Append(winrt::Setter{ winrt::FrameworkElement::MinWidthProperty(), winrt::box_value(0.0) });
    setters.Append(winrt::Setter{ winrt::FrameworkElement::MinHeightProperty(), winrt::box_value(0.0) });

    if (auto defaultStyle = resources.TryLookup(winrt::box_value(L"DefaultFlyoutPresenterStyle")).try_as<winrt::Style>())
    {
        style.BasedOn(defaultStyle);
    }
    else
    {
        setters.Append(winrt::Setter{ winrt::Control::BorderThicknessProperty(), winrt::box_value(winrt::Thickness{ 1, 1, 1, 1 }) });
        if (auto background = resources.TryLookup(winrt::box_value(L"AcrylicBackgroundFillColorDefaultBrush")))
        {
            setters.Append(winrt::Setter{ winrt::Control::BackgroundProperty(), background });
        }
        if (auto borderBrush = resources.TryLookup(winrt::box_value(L"SurfaceStrokeColorDefaultBrush")))
        {
            setters.Append(winrt::Setter{ winrt::Control::BorderBrushProperty(), borderBrush });
        }
        if (auto cornerRadius = resources.TryLookup(winrt::box_value(L"ControlCornerRadius")))
        {
            setters.Append(winrt::Setter{ winrt::Control::CornerRadiusProperty(), cornerRadius });
        }
    }

    flyout.FlyoutPresenterStyle(style);
}
