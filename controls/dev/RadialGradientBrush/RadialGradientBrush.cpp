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

    // If CompositionRadialGradientBrush will be used then listen for changes to gradient stops so the CompositionRadialGradientBrush can be updated.
    m_gradientStopsVectorChangedRevoker = m_gradientStops.VectorChanged(winrt::auto_revoke, { this, &RadialGradientBrush::OnGradientStopsVectorChanged });
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

void RadialGradientBrush::OnCenterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionGradientCenter();
}

void RadialGradientBrush::OnRadiusXPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionGradientRadius();
}

void RadialGradientBrush::OnRadiusYPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionGradientRadius();
}

void RadialGradientBrush::OnGradientOriginPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionGradientOrigin();
}

void RadialGradientBrush::OnMappingModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionGradientMappingMode();
}

void RadialGradientBrush::OnInterpolationSpacePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionInterpolationSpace();
}

void RadialGradientBrush::OnSpreadMethodPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCompositionExtendMode();
}

void RadialGradientBrush::OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args)
{
    UpdateFallbackBrush();
}

void RadialGradientBrush::OnGradientStopsVectorChanged(winrt::Collections::IObservableVector<winrt::GradientStop> const& sender, winrt::Collections::IVectorChangedEventArgs const& e)
{
    UpdateCompositionGradientStops();
}

void RadialGradientBrush::EnsureCompositionBrush()
{
    if (m_isConnected && !m_brush)
    {
        const auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

        // If CompositionRadialGradientBrush is available then use it to render a gradient.
        m_brush = compositor.CreateRadialGradientBrush();

        UpdateCompositionGradientCenter();
        UpdateCompositionGradientRadius();
        UpdateCompositionGradientOrigin();
        UpdateCompositionGradientStops();
        UpdateCompositionGradientMappingMode();
        UpdateCompositionInterpolationSpace();
        UpdateCompositionExtendMode();

        CompositionBrush(m_brush);
    }
}

void RadialGradientBrush::UpdateCompositionGradientCenter()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto center = Center();
        compositionGradientBrush.EllipseCenter(winrt::float2(center.X, center.Y));

        // Changing the center affects the origin in the comp brush because we are translating from
        // Origin to offset.
        UpdateCompositionGradientOrigin();
    }
}

void RadialGradientBrush::UpdateCompositionGradientRadius()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        compositionGradientBrush.EllipseRadius(winrt::float2(static_cast<float>(RadiusX()), static_cast<float>(RadiusY())));
    }
}

void RadialGradientBrush::UpdateCompositionGradientMappingMode()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        switch (MappingMode())
        {
        case winrt::BrushMappingMode::Absolute:
            compositionGradientBrush.MappingMode(winrt::Microsoft::UI::Composition::CompositionMappingMode::Absolute);
            break;
        case winrt::BrushMappingMode::RelativeToBoundingBox:
            [[fallthrough]];
        default: // Use Relative as the default if the mode isn't recognized.
            compositionGradientBrush.MappingMode(winrt::Microsoft::UI::Composition::CompositionMappingMode::Relative);
            break;
        }
    }
}

void RadialGradientBrush::UpdateCompositionGradientOrigin()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto gradientOrigin = GradientOrigin();
        const auto center = Center();
        // Comp uses offset, WinUI will follow WPF and use bounds relative to the element in all mapping modes.
        // If ElementWidth=100 ElementHeight=100
        // Relative mode
        //     TopLeft of element = 0,0
        //     Center of element = .5,.5
        //     BottomRight of element = 1,1
        // Absolute mode
        //     TopLeft of element = 0,0
        //     Center of element = 50,50
        //     BottomRight of element = 100,100
        compositionGradientBrush.GradientOriginOffset(winrt::float2(gradientOrigin.X - center.X, gradientOrigin.Y - center.Y));
    }
}

void RadialGradientBrush::UpdateCompositionGradientStops()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        const auto compositor = winrt::CompositionTarget::GetCompositorForCurrentThread();

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
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        compositionGradientBrush.InterpolationSpace(InterpolationSpace());
    }
}

void RadialGradientBrush::UpdateCompositionExtendMode()
{
    if (const auto compositionGradientBrush = m_brush.try_as<winrt::CompositionRadialGradientBrush>())
    {
        switch (SpreadMethod())
        {
        case winrt::GradientSpreadMethod::Repeat:
            compositionGradientBrush.ExtendMode(winrt::Microsoft::UI::Composition::CompositionGradientExtendMode::Wrap);
            break;
        case winrt::GradientSpreadMethod::Reflect:
            compositionGradientBrush.ExtendMode(winrt::Microsoft::UI::Composition::CompositionGradientExtendMode::Mirror);
            break;
        case winrt::GradientSpreadMethod::Pad:
            [[fallthrough]];
        default: // Use Pad as the default if the mode isn't recognized.
            compositionGradientBrush.ExtendMode(winrt::Microsoft::UI::Composition::CompositionGradientExtendMode::Clamp);
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
