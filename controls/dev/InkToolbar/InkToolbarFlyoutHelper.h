// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

// Applies the UWP InkToolbarFlyoutStyle to a tool/menu button's L3 flyout: tight (zero) presenter
// padding so the flyout matches WinUI 2, plus the acrylic background/border/corner. Built in code
// because the keyed style in InkToolbar's generic.xaml is not reachable from the button's own (empty)
// resources or from app-level lookup, so a plain resource lookup silently no-ops.
inline void ApplyInkToolbarFlyoutStyle(winrt::Flyout const& flyout)
{
    auto resources = winrt::Application::Current().Resources();

    winrt::Style style{ winrt::xaml_typename<winrt::Microsoft::UI::Xaml::Controls::FlyoutPresenter>() };
    auto setters = style.Setters();
    setters.Append(winrt::Setter{ winrt::Control::PaddingProperty(), winrt::box_value(winrt::Thickness{ 0, 0, 0, 0 }) });
    setters.Append(winrt::Setter{ winrt::FrameworkElement::MinWidthProperty(), winrt::box_value(0.0) });
    setters.Append(winrt::Setter{ winrt::FrameworkElement::MinHeightProperty(), winrt::box_value(0.0) });
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

    flyout.FlyoutPresenterStyle(style);
}
