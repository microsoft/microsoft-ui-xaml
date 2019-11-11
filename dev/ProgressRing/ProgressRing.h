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
    ~ProgressRing() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void RenderAll();

    winrt::Grid::Loaded_revoker m_layoutRootLoadedRevoker{};
    winrt::Path::Loaded_revoker m_outlineFigureRevoker{};
    winrt::Path::Loaded_revoker m_outlineArcRevoker{};

    tracker_ref<winrt::Grid> m_layoutRoot{ this };
    tracker_ref<winrt::Windows::UI::Xaml::Media::PathFigure> m_outlineFigure{ this };
    tracker_ref<winrt::Windows::UI::Xaml::Media::ArcSegment> m_outlineArc{ this };

    static constexpr wstring_view s_LayoutRootName{ L"LayoutRoot" };
    static constexpr wstring_view s_OutlineFigureName{ L"OutlineFigurePart" };
    static constexpr wstring_view s_OutlineArcName{ L"OutlineArcPart" };
};
