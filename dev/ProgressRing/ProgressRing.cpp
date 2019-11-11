// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ProgressRing.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ProgressRing::ProgressRing()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ProgressRing);

    SetDefaultStyleKey(this);
}

void ProgressRing::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(s_LayoutRootName, controlProtected));
    m_outlineFigure.set(GetTemplateChildT<winrt::Windows::UI::Xaml::Media::PathFigure>(s_OutlineFigureName, controlProtected));
    m_outlineArc.set(GetTemplateChildT<winrt::Windows::UI::Xaml::Media::ArcSegment>(s_OutlineArcName, controlProtected));

    RenderAll();
}

void  ProgressRing::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // TODO: Implement
}

void ProgressRing::RenderAll()
{
    if (auto&& progressRing = m_layoutRoot.get())
    {
        auto&& outlineFigure = m_outlineFigure.get();
        auto&& outlineArc = m_outlineArc.get();

        outlineFigure.StartPoint(winrt::Windows::Foundation::Point(0,100));
        outlineArc.Size(winrt::Windows::Foundation::Size(300, 50));
        outlineArc.Point(winrt::Windows::Foundation::Point(200, 100));
    }
}
