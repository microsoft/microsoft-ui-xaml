// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressRingLoading.h"

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
    void OnIsIndeterminatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnForegroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnBackgroundPropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);

private:
    void OnRangeBasePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&);
    void OnSizeChanged(const winrt::IInspectable&, const winrt::IInspectable&);
    void ApplyLottieAnimation();
    void UpdateStates();
    void UpdateSegment();
    void UpdateRing();

    static winrt::Size ComputeCircleSize(double thickness, double width);

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::PathFigure> m_outlineFigure{ this };
    tracker_ref<winrt::ArcSegment> m_outlineArc{ this };
    tracker_ref<winrt::PathFigure> m_ringFigure{ this };
    tracker_ref<winrt::ArcSegment> m_ringArc{ this };
    tracker_ref<winrt::AnimatedVisualPlayer> m_player{ this };

    static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot" };
    static constexpr wstring_view s_OutlineFigureName{ L"OutlineFigurePart" };
    static constexpr wstring_view s_OutlineArcName{ L"OutlineArcPart" };
    static constexpr wstring_view s_BarFigureName{ L"RingFigurePart" };
    static constexpr wstring_view s_BarArcName{ L"RingArcPart" };
    static constexpr wstring_view s_LottiePlayerName{ L"LottiePlayer" };
    static constexpr wstring_view s_DeterminateStateName{ L"Determinate" };
    static constexpr wstring_view s_IndeterminateStateName{ L"Indeterminate" };
    
};
