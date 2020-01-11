// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "Vector.h"
#include "SharedHelpers.h"
#include "RadialGradientBrush.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

RadialGradientBrush::RadialGradientBrush()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadialGradientBrush);

    m_gradientStops = winrt::make<Vector<winrt::GradientStop>>().as<winrt::IObservableVector<winrt::GradientStop>>();
}

winrt::IObservableVector<winrt::GradientStop> RadialGradientBrush::GradientStops()
{
    return m_gradientStops;
}

void RadialGradientBrush::OnConnected()
{
    // XCBB will use fallback rendering in design v1 mode, so do not create a CompositionBrush.
    if (SharedHelpers::IsInDesignMode()) { return; }

    m_isConnected = true;

    EnsureCompositionBrush();

    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        // If CompositionRadialGradientBrush will be used then listen for changes to gradient stops so the composition brush can be updated.
        m_gradientStopsChangedToken = m_gradientStops.VectorChanged({
            [this](winrt::IObservableVector<winrt::GradientStop> const& sender, winrt::IVectorChangedEventArgs const& args)
            {
                if (m_brush)
                {
                    UpdateCompositionGradientStops();
                }
            }
            });
    }
    else
    {
        // If CompositionRadialGradientBrush won't be used then listen for changes to the fallback color so the fallback brush can be updated.
        m_fallbackColorChangedToken.value = RegisterPropertyChangedCallback(winrt::XamlCompositionBrushBase::FallbackColorProperty(), { this, &RadialGradientBrush::OnFallbackColorChanged });
    }
}

void RadialGradientBrush::OnDisconnected()
{
    m_isConnected = false;

    if (m_brush)
    {
        m_brush.Close();
        m_brush = nullptr;
        CompositionBrush(nullptr);
    }

    if (m_gradientStopsChangedToken)
    {
        m_gradientStops.VectorChanged(m_gradientStopsChangedToken);
    }

    if (m_fallbackColorChangedToken)
    {
        UnregisterPropertyChangedCallback(winrt::XamlCompositionBrushBase::FallbackColorProperty(), m_fallbackColorChangedToken.value);
        m_fallbackColorChangedToken.value = 0;
    }
}

void RadialGradientBrush::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        auto property = args.Property();

        if (property == s_EllipseCenterProperty)
        {
            UpdateCompositionGradientEllipseCenter();
        }
        else if (property == s_EllipseRadiusProperty)
        {
            UpdateCompositionGradientEllipseRadius();
        }
        else if (property == s_GradientOriginOffsetProperty)
        {
            UpdateCompositionGradientOriginOffset();
        }
        else if (property == s_MappingModeProperty)
        {
            UpdateCompositionGradientMappingMode();
        }
        else if (property == s_InterpolationSpaceProperty)
        {
            UpdateCompositionInterpolationSpace();
        }
    }
}

void RadialGradientBrush::OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateFallbackBrush();
}

void RadialGradientBrush::EnsureCompositionBrush()
{
    if (m_isConnected && !m_brush)
    {
        auto compositor = winrt::Window::Current().Compositor();

        if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
        {
            // If CompositionRadialGradientBrush is available then use it to render a gradient.
            m_brush = compositor.CreateRadialGradientBrush();

            UpdateCompositionGradientEllipseCenter();
            UpdateCompositionGradientEllipseRadius();
            UpdateCompositionGradientOriginOffset();
            UpdateCompositionGradientStops();
            UpdateCompositionGradientMappingMode();
            UpdateCompositionInterpolationSpace();
        }
        else
        {
            // If CompositionRadialGradientBrush isn't available then render using the FallbackColor.
            m_brush = compositor.CreateColorBrush();
            UpdateFallbackBrush();
        }

        CompositionBrush(m_brush);
    }
}

void RadialGradientBrush::UpdateCompositionGradientEllipseCenter()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        compositionGradientBrush.EllipseCenter(winrt::float2(EllipseCenter().X, EllipseCenter().Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientEllipseRadius()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        compositionGradientBrush.EllipseRadius(winrt::float2(EllipseRadius().X, EllipseRadius().Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientMappingMode()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        switch (MappingMode())
        {
        case winrt::BrushMappingMode::Absolute:
            compositionGradientBrush.MappingMode(winrt::Windows::UI::Composition::CompositionMappingMode::Absolute);
            break;

        case winrt::BrushMappingMode::RelativeToBoundingBox:
            compositionGradientBrush.MappingMode(winrt::Windows::UI::Composition::CompositionMappingMode::Relative);
            break;
        }
    }
}

void RadialGradientBrush::UpdateCompositionGradientOriginOffset()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        compositionGradientBrush.GradientOriginOffset(winrt::float2(GradientOriginOffset().X, GradientOriginOffset().Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientStops()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        auto compositor = winrt::Window::Current().Compositor();

        compositionGradientBrush.ColorStops().Clear();

        for (uint32_t i = 0; i < m_gradientStops.Size(); i++)
        {
            auto gradientStop = m_gradientStops.GetAt(i);

            auto compositionStop = compositor.CreateColorGradientStop();
            compositionStop.Color(gradientStop.Color());
            compositionStop.Offset(static_cast<float>(gradientStop.Offset()));

            compositionGradientBrush.ColorStops().Append(compositionStop);
        }
    }
}

void RadialGradientBrush::UpdateCompositionInterpolationSpace()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>();

    if (compositionGradientBrush)
    {
        compositionGradientBrush.InterpolationSpace(InterpolationSpace());
    }
}

void RadialGradientBrush::UpdateFallbackBrush()
{
    auto compositionColorBrush = m_brush.try_as<winrt::CompositionColorBrush>();

    if (compositionColorBrush)
    {
        compositionColorBrush.Color(FallbackColor());
    }
}
