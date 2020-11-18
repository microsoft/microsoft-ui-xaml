// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ImageIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ImageIcon::ImageIcon()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ImageIcon);
}

void ImageIcon::OnApplyTemplate()
{
    Loaded({ this, &ImageIcon::OnLoaded });
}

void ImageIcon::OnLoaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    winrt::IInspectable diagnostics{};
    if (auto const grid = winrt::VisualTreeHelper::GetChild(*this, 0).as<winrt::Grid>())
    {
        auto const image = winrt::VisualTreeHelper::GetChild(grid, 0).as<winrt::Image>();
        image.Source(Source());
        m_rootImage.set(image);
    }
    else
    {
        m_rootImage.set(nullptr);
    }
}

void  ImageIcon::OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto const image = m_rootImage.get())
    {
        image.Source(Source());
    }
}
