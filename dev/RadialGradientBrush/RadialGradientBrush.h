// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "RadialGradientBrush.g.h"
#include "RadialGradientBrush.properties.h"
#include "MaterialHelper.h"

class RadialGradientBrush :
#if BUILD_WINDOWS
    public ReferenceTracker<RadialGradientBrush, winrt::implementation::RadialGradientBrushT, winrt::IXamlCompositionBrushBaseOverridesPrivate>,
#else
    public ReferenceTracker<RadialGradientBrush, winrt::implementation::RadialGradientBrushT>,
#endif
    public RadialGradientBrushProperties
{
    friend MaterialHelper;

public:
    RadialGradientBrush();
    ~RadialGradientBrush() {}

    void SetPropertyToDefaultValues();
    void CreateRadialGradientBrush();
    void UpdateRadialGradientBrush();
    void setEllipseCenter(float x, float y);
    void setEllipseRadius(float x, float y);
    void setGradientOriginOffset(float x, float y);
    void AddColorGradientStop(winrt::Color color, const float offset);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    typedef winrt::Windows::Foundation::Numerics::float2 Vector2;

    winrt::CompositionRadialGradientBrush m_brush{ NULL };

    std::vector<winrt::CompositionColorGradientStop> m_stops;

    Vector2 m_ellipseCenter;
    Vector2 m_ellipseRadius;
    Vector2 m_gradientOriginOffset;

    bool m_brushCreated;
};
