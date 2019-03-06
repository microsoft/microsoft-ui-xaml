// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RadialGradientBrush.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

RadialGradientBrush::RadialGradientBrush() : m_brushCreated(false)
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadialGradientBrush);

    UpdateRadialGradientBrush();
}

void RadialGradientBrush::SetPropertyToDefaultValues()
{
    setEllipseCenter(0.5f, 0.5f);
    setEllipseRadius(1.0f, 1.0f);
    setGradientOriginOffset(0.2f, 0.2f);
    AddColorGradientStop(winrt::Color{ 255, 255, 128, 0 }, 0.25f);//Orange
    AddColorGradientStop(winrt::Color{ 255, 90, 200, 90 }, 0.5f);//Green
    AddColorGradientStop(winrt::Color{ 255, 255, 128, 0 }, 0.75f);//Orange
    AddColorGradientStop(winrt::Color{ 255, 90, 200, 90 }, 1.0f);//Green
}

void RadialGradientBrush::CreateRadialGradientBrush()
{
    //Initialize the Comp brush instance
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    winrt::CompositionRadialGradientBrush radialGradientBrush = compositor.CreateRadialGradientBrush();

    m_brush = radialGradientBrush;

    SetPropertyToDefaultValues();

    m_brushCreated = true;
}

void RadialGradientBrush::UpdateRadialGradientBrush()
{
    if (!m_brushCreated)
    {
        CreateRadialGradientBrush();
    }
    CompositionBrush(m_brush);
}

void RadialGradientBrush::setEllipseCenter(float x, float y)
{
    m_ellipseCenter.x = x;
    m_ellipseCenter.y = y;

    m_brush.EllipseCenter(m_ellipseCenter);
}

void RadialGradientBrush::setEllipseRadius(float x, float y)
{
    m_ellipseRadius.x = x;
    m_ellipseRadius.y = y;

    m_brush.EllipseRadius(m_ellipseRadius);
}

void RadialGradientBrush::setGradientOriginOffset(float x, float y)
{
    m_gradientOriginOffset.x = x;
    m_gradientOriginOffset.y = y;

    m_brush.GradientOrigin(m_gradientOriginOffset);
}

void RadialGradientBrush::AddColorGradientStop(winrt::Color color, const float offset)
{
    winrt::Compositor compositor = winrt::Window::Current().Compositor();

    winrt::CompositionColorGradientStop colorGradientStop = compositor.CreateColorGradientStop();
    colorGradientStop.Color(color);
    colorGradientStop.Offset(offset);

    winrt::CompositionColorGradientStopCollection colorGradientStopCollection = m_brush.ColorStops();
    colorGradientStopCollection.Append(colorGradientStop);

    m_stops.push_back(colorGradientStop);
}

void  RadialGradientBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}
