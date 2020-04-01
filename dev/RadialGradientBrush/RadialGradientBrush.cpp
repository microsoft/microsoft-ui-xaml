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
        // If CompositionRadialGradientBrush will be used then listen for changes to gradient stops so the CompositionRadialGradientBrush can be updated.
        m_gradientStopsVectorChangedRevoker = m_gradientStops.VectorChanged(winrt::auto_revoke, { this, &RadialGradientBrush::OnGradientStopsVectorChanged });
    }
    else
    {
        // If CompositionRadialGradientBrush won't be used then listen for changes to the fallback color so the fallback brush can be updated.
        m_fallbackColorChangedRevoker = RegisterPropertyChanged(*this, winrt::XamlCompositionBrushBase::FallbackColorProperty(), { this, &RadialGradientBrush::OnFallbackColorChanged });
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

    if (m_gradientStopsVectorChangedRevoker)
    {
        m_gradientStopsVectorChangedRevoker.revoke();
    }

    if (m_fallbackColorChangedRevoker)
    {
        m_fallbackColorChangedRevoker.revoke();
    }
}

void RadialGradientBrush::OnEllipseCenterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionGradientEllipseCenter();
    }
}

void RadialGradientBrush::OnEllipseRadiusPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionGradientEllipseRadius();
    }
}

void RadialGradientBrush::OnGradientOriginPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionGradientOrigin();
    }
}

void RadialGradientBrush::OnMappingModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionGradientMappingMode();
    }
}

void RadialGradientBrush::OnInterpolationSpacePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionInterpolationSpace();
    }
}

void RadialGradientBrush::OnSpreadMethodPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionExtendMode();
    }
}

void RadialGradientBrush::OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateFallbackBrush();
}

void RadialGradientBrush::OnGradientStopsVectorChanged(winrt::Collections::IObservableVector<winrt::GradientStop> const& sender, winrt::Collections::IVectorChangedEventArgs const& e)
{
    if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
    {
        UpdateCompositionGradientStops();
    }
}

void RadialGradientBrush::EnsureCompositionBrush()
{
    if (m_isConnected && !m_brush)
    {
        const auto compositor = winrt::Window::Current().Compositor();

        if (SharedHelpers::IsCompositionRadialGradientBrushAvailable())
        {
            // If CompositionRadialGradientBrush is available then use it to render a gradient.
            m_brush = compositor.CreateRadialGradientBrush();

            UpdateCompositionGradientEllipseCenter();
            UpdateCompositionGradientEllipseRadius();
            UpdateCompositionGradientOrigin();
            UpdateCompositionGradientStops();
            UpdateCompositionGradientMappingMode();
            UpdateCompositionInterpolationSpace();
            UpdateCompositionExtendMode();
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

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto ellipseCenter = EllipseCenter();
        compositionGradientBrush.EllipseCenter(winrt::float2(ellipseCenter.X, ellipseCenter.Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientEllipseRadius()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto ellipseRadius = EllipseRadius();
        compositionGradientBrush.EllipseRadius(winrt::float2(ellipseRadius.X, ellipseRadius.Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientMappingMode()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        switch (MappingMode())
        {
        case winrt::BrushMappingMode::Absolute:
            compositionGradientBrush.MappingMode(winrt::Windows::UI::Composition::CompositionMappingMode::Absolute);
            break;
        case winrt::BrushMappingMode::RelativeToBoundingBox:
            [[fallthrough]];
        default: // Use Relative as the default if the mode isn't recognized.
            compositionGradientBrush.MappingMode(winrt::Windows::UI::Composition::CompositionMappingMode::Relative);
            break;
        }
    }
}

void RadialGradientBrush::UpdateCompositionGradientOrigin()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto gradientOrigin = GradientOrigin();
        // This sets the gradient offset center to the top left corner
        // Top Left is (-0.5,-0.5), center is (0,0)
        compositionGradientBrush.GradientOriginOffset(winrt::float2(gradientOrigin.X - 0.5f, gradientOrigin.Y - 0.5f));
    }
}

void RadialGradientBrush::UpdateCompositionGradientStops()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto compositor = winrt::Window::Current().Compositor();

        compositionGradientBrush.ColorStops().Clear();

        for (const auto& gradientStop : m_gradientStops)
        {
            const auto compositionStop = compositor.CreateColorGradientStop();
            compositionStop.Color(gradientStop.Color());
            compositionStop.Offset(static_cast<float>(gradientStop.Offset()));

            compositionGradientBrush.ColorStops().Append(compositionStop);
        }
    }
}

void RadialGradientBrush::UpdateCompositionInterpolationSpace()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        compositionGradientBrush.InterpolationSpace(InterpolationSpace());
    }
}

void RadialGradientBrush::UpdateCompositionExtendMode()
{
    MUX_ASSERT(SharedHelpers::IsCompositionRadialGradientBrushAvailable());

    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        switch (SpreadMethod())
        {
        case winrt::GradientSpreadMethod::Repeat:
            compositionGradientBrush.ExtendMode(winrt::Windows::UI::Composition::CompositionGradientExtendMode::Wrap);
            break;
        case winrt::GradientSpreadMethod::Reflect:
            compositionGradientBrush.ExtendMode(winrt::Windows::UI::Composition::CompositionGradientExtendMode::Mirror);
            break;
        case winrt::GradientSpreadMethod::Pad:
            [[fallthrough]];
        default: // Use Pad as the default if the mode isn't recognized.
            compositionGradientBrush.ExtendMode(winrt::Windows::UI::Composition::CompositionGradientExtendMode::Clamp);
            break;
        }
    }
}

void RadialGradientBrush::UpdateFallbackBrush()
{
    if (const auto compositionColorBrush = m_brush.try_as<winrt::CompositionColorBrush>())
    {
        compositionColorBrush.Color(FallbackColor());
    }
}
