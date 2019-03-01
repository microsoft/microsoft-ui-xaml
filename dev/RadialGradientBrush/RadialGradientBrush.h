// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "RadialGradientBrush.g.h"
#include "RadialGradientBrush.properties.h"
#include "MaterialHelper.h"

class RadialGradientBrush :
    public ReferenceTracker<RadialGradientBrush, winrt::implementation::RadialGradientBrushT>,
    public RadialGradientBrushProperties
{
    friend MaterialHelper;

public:
    RadialGradientBrush();
    ~RadialGradientBrush() {}

    void setEllipseCenter(const float x, const float y);
    void setEllipseRadius(const float x, const float y);
    void setGradientOriginOffset(const float x, const float y);
    void AddColorGradientStop(winrt::Color color, const float offset);

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    winrt::CompositionRadialGradientBrush m_brush{ nullptr };

    std::vector<winrt::CompositionColorGradientStop> m_stops;

    Windows::Foundation::Numerics::Vector2 m_ellipseCenter;
    Windows::Foundation::Numerics::Vector2 m_ellipseRadius;
    Windows::Foundation::Numerics::Vector2 m_gradientOriginOffset;
};
