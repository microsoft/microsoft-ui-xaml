// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadialGradientBrush.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

RadialGradientBrush::RadialGradientBrush()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadialGradientBrush);

    SetDefaultStyleKey(this);

    //Initialize the Comp brush instance
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    winrt::CompositionRadialGradientBrush radialGradientBrush = compositor.CreateRadialGradientBrush();

    m_brush = radialGradientBrush;
}

void RadialGradientBrush::setEllipseCenter(const float x, const float y)
{
    m_ellipseCenter.X = x;
    m_ellipseCenter.Y = y;

    m_brush.EllipseCenter = m_ellipseCenter;
}

void RadialGradientBrush::setEllipseRadius(const float x, const float y)
{
    m_ellipseRadius.X = x;
    m_ellipseRadius.Y = y;

    m_brush.EllipseRadius = m_ellipseRadius;
}

void RadialGradientBrush::setGradientOriginOffset(const float x, const float y)
{
    m_gradientOriginOffset.X = x;
    m_gradientOriginOffset.Y = y;

    m_brush.GradientOriginOffset = m_gradientOriginOffset;
}

void RadialGradientBrush::AddColorGradientStop(Windows::UI::Color color, const float offset)
{
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    winrt::CompositionColorGradientStop colorGradientStop = compositor.CreateColorGradientStop();
    colorGradientStop.Color = color;
    colorGradientStop.Offset = offset;

    winrt::CompositionColorGradientStopCollection colorGradientStopCollection = m_brush.ColorStops;
    colorGradientStopCollection.Add(colorGradientStop);

    m_stops.push_back(colorGradientStop);
}

void RadialGradientBrush::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement
}

void  RadialGradientBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
