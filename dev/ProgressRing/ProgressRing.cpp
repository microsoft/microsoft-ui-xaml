// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "math.h"

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);

    RegisterPropertyChangedCallback(winrt::RangeBase::ValueProperty(), { this, &ProgressRing::OnRangeBasePropertyChanged });

    SizeChanged({ this, &ProgressRing::OnSizeChanged });
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    m_outlineFigure.set(GetTemplateChildT<winrt::PathFigure>(s_OutlineFigureName, controlProtected));
    m_outlineArc.set(GetTemplateChildT<winrt::ArcSegment>(s_OutlineArcName, controlProtected));
    m_barFigure.set(GetTemplateChildT<winrt::PathFigure>(s_BarFigureName, controlProtected));
    m_barArc.set(GetTemplateChildT<winrt::ArcSegment>(s_BarArcName, controlProtected));

    UpdateRing();
}

void ProgressRing::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    UpdateRing();
}

void ProgressRing::OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    UpdateSegment();
}

void ProgressRing::OnStrokeThicknessPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRing();
}

winrt::Size ProgressRing::ComputeEllipseSize(double thickness, double actualWidth, double actualHeight)
{
    const double safeThickness = std::max(thickness, static_cast<double>(0.0));
    const double width = std::max((actualWidth - safeThickness) / 2.0, 0.0);
    const double height = std::max((actualHeight - safeThickness) / 2.0, 0.0);

    return {static_cast<float>(width), static_cast<float>(height)};
}


void ProgressRing::UpdateSegment()
{
    if (auto&& barArc = m_barArc.get())
    {
        const double thickness = StrokeThickness();
        const double minimum = Minimum();
        const double range = Maximum() - minimum;
        const double delta = Value() - minimum;

        double normalizedRange = (range == 0.0) ? 0.0 : (delta / range);

        // normalizedRange offsets calculation to display a full ring when value = 100%
        // std::nextafter is set as a float as winrt::Point takes floats 
        normalizedRange = std::min(normalizedRange, static_cast<double>(std::nextafterf(1.0, 0.0)));

        const double angle = 2 * M_PI * normalizedRange;
        const auto size = ComputeEllipseSize(thickness, ActualWidth(), ActualHeight());
        const double translationFactor = std::max(thickness / 2.0, 0.0);

        const double x = (std::sin(angle) * size.Width) + size.Width + translationFactor;
        const double y = (((std::cos(angle) * size.Height) - size.Height) * -1) + translationFactor;

        barArc.IsLargeArc(angle >= M_PI);
        barArc.Point(winrt::Point(static_cast<float>(x), static_cast<float>(y)));
    }
}

void ProgressRing::UpdateRing()
{
    const double thickness = StrokeThickness();
    const auto size = ComputeEllipseSize(thickness, ActualWidth(), ActualHeight());

    const float segmentWidth = size.Width;
    const float translationFactor = static_cast<float>(std::max(thickness / 2.0, 0.0));

    if (auto&& outlineFigure = m_outlineFigure.get())
    {
        outlineFigure.StartPoint(winrt::Point(segmentWidth + translationFactor, translationFactor));
    }

    if (auto&& barFigure = m_barFigure.get())
    {
        barFigure.StartPoint(winrt::Point(segmentWidth + translationFactor, translationFactor));
    }

    if (auto&& outlineArc = m_outlineArc.get())
    {
        outlineArc.Size(winrt::Size(segmentWidth, size.Height));
        outlineArc.Point(winrt::Point(segmentWidth + translationFactor - 0.05f, translationFactor));
    }

    if (auto&& barArc = m_barArc.get())
    {  
        barArc.Size(winrt::Size(segmentWidth, size.Height));
    }

    UpdateSegment();
}
