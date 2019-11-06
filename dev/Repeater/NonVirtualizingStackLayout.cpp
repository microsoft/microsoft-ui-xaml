// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NonVirtualizingStackLayout.h"


//CppWinRTActivatableClassWithBasicFactory(NonVirtualizingStackLayout);

NonVirtualizingStackLayout::NonVirtualizingStackLayout()
{

}

winrt::Size NonVirtualizingStackLayout::MeasureOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& availableSize)
{
    float extentHeight = 0.0;
    float extentWidth = 0.0;

    // HORIZONTAL
    for (auto element : context.Children())
    {
        element.Measure(availableSize);
        extentWidth += element.DesiredSize().Width;
        extentHeight = std::max(extentHeight, element.DesiredSize().Height);
    }

    // VERTICAL
    //for (auto element : context.Children())
    //{
    //    element.Measure(availableSize);
    //    extentHeight += element.DesiredSize().Height;
    //    extentWidth = std::max(extentWidth, element.DesiredSize().Width);
    //}

    return winrt::Size(extentWidth, extentHeight);
}

winrt::Size NonVirtualizingStackLayout::ArrangeOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& finalSize)
{
    // HORIZONTAL
    float offset = 0.0;
    for (auto element : context.Children())
    {
        element.Arrange(winrt::Rect(offset, 0, element.DesiredSize().Width, element.DesiredSize().Height));
        offset += element.DesiredSize().Width;
    }

    // VERTICAL
    //float offset = 0.0;
    //for (auto element : context.Children())
    //{
    //    element.Arrange(winrt::Rect(0, offset, element.DesiredSize().Width, element.DesiredSize().Height));
    //    offset += element.DesiredSize().Height;
    //}

    return finalSize;
}

void NonVirtualizingStackLayout::OnOrientationPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{

}
