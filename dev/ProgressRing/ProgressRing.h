// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressRing.g.h"
#include "ProgressRing.properties.h"

class ProgressRing :
    public ReferenceTracker<ProgressRing, winrt::implementation::ProgressRingT>,
    public ProgressRingProperties
{

public:
    ProgressRing();

    // IFrameworkElement
    void OnApplyTemplate();

    void OnStrokeThicknessPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnStrokeThicknessPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void RenderSegment();
    void RenderAll();

    static winrt::Size ComputeEllipseSize(const double thickness, const double width, const double height);

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::PathFigure> m_outlineFigure{ this };
    tracker_ref<winrt::ArcSegment> m_outlineArc{ this };
    tracker_ref<winrt::PathFigure> m_barFigure{ this };
    tracker_ref<winrt::ArcSegment> m_barArc{ this };

    static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot" };
    static constexpr wstring_view s_OutlineFigureName{ L"OutlineFigurePart" };
    static constexpr wstring_view s_OutlineArcName{ L"OutlineArcPart" };
    static constexpr wstring_view s_BarFigureName{ L"BarFigurePart" };
    static constexpr wstring_view s_BarArcName{ L"BarArcPart" };
};
