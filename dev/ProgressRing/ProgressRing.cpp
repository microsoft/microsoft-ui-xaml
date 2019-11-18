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

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));
    m_outlineFigure.set(GetTemplateChildT<winrt::PathFigure>(s_OutlineFigureName, controlProtected));
    m_outlineArc.set(GetTemplateChildT<winrt::ArcSegment>(s_OutlineArcName, controlProtected));
    m_barFigure.set(GetTemplateChildT<winrt::PathFigure>(s_BarFigureName, controlProtected));
    m_barArc.set(GetTemplateChildT<winrt::ArcSegment>(s_BarArcName, controlProtected));

    RenderAll();
}

void  ProgressRing::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}

void ProgressRing::OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&)
{
    RenderAll();
}

void ProgressRing::OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    RenderSegment();
}

void ProgressRing::OnStrokeThicknessPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    RenderAll();
}

winrt::Size ProgressRing::ComputeEllipseSize(const double thickness, const double actualWidth, const double actualHeight)
{
    const double safeThickness = std::max(thickness, static_cast<double>(0.0));
    const double width = std::max((actualWidth - safeThickness) / 2.0, 0.0);
    const double height = std::max((actualHeight - safeThickness) / 2.0, 0.0);

    return {static_cast<float>(width), static_cast<float>(height)};
}


void ProgressRing::RenderSegment()
{
    if (auto&& barArc = m_barArc.get())
    {
        const double thickness = StrokeThickness();
        const double range = Maximum() - Minimum();
        const double delta = Value() - Minimum();

        double normalizedRange = (range == 0.0) ? 0.0 : (delta / range);
        normalizedRange = std::min(std::max(0.0, normalizedRange), 0.9999);

        const double angle = 2 * M_PI * normalizedRange;
        const auto size = ComputeEllipseSize(thickness, Width(), Height());
        const double translationFactor = std::max(thickness / 2.0, 0.0);

        const double x = (std::sin(angle) * size.Width) + size.Width + translationFactor;
        const double y = (((std::cos(angle) * size.Height) - size.Height) * -1) + translationFactor;

        barArc.IsLargeArc(angle >= M_PI);
        barArc.Point(winrt::Point(static_cast<float>(x), static_cast<float>(y)));
    }

}

void ProgressRing::RenderAll()
{
    if (auto&& outlineFigure = m_outlineFigure.get())
    {
        if (auto&& outlineArc = m_outlineArc.get())
        {
            if (auto&& barFigure = m_barFigure.get())
            {
                if (auto&& barArc = m_barArc.get())
                {
                    const double thickness = StrokeThickness();
                    const auto size = ComputeEllipseSize(thickness, Width(), Height());

                    const float segmentWidth = size.Width;
                    const float translationFactor = static_cast<float>(std::max(thickness / 2.0, 0.0));

                    outlineFigure.StartPoint(winrt::Point(segmentWidth + translationFactor, translationFactor));
                    barFigure.StartPoint(winrt::Point(segmentWidth + translationFactor, translationFactor));
                    outlineArc.Size(winrt::Size(segmentWidth, size.Height));
                    barArc.Size(winrt::Size(segmentWidth, size.Height));
                    outlineArc.Point(winrt::Point(segmentWidth + translationFactor - 0.05f, translationFactor));

                    RenderSegment();
                }
            }
        }
    }
}
