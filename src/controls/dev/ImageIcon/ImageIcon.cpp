// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ImageIcon.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "FrameworkUdk/Containment.h"

// Bug 62784965: [2.0 servicing] [Opt-in] Optimize away extra Grid container in CFontIcon and CBitmapIcon
#define WINAPPSDK_CHANGEID_62784965 62784965

ImageIcon::ImageIcon()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ImageIcon);
}

void ImageIcon::OnApplyTemplate()
{
    winrt::IInspectable diagnostics{};
    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_62784965>())
    {
        auto const firstChild = winrt::VisualTreeHelper::GetChild(*this, 0);

        // The first child may be a Grid (old behavior) or directly an Image (new behavior).
        if (auto const grid = firstChild.try_as<winrt::Grid>())
        {
            auto const image = winrt::VisualTreeHelper::GetChild(grid, 0).as<winrt::Image>();
            image.Source(Source());
            m_rootImage.set(image);
        }
        else if (auto const image = firstChild.try_as<winrt::Image>())
        {
            image.Source(Source());
            m_rootImage.set(image);
        }
        else
        {
            m_rootImage.set(nullptr);
        }
    }
    else
    {
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
}

void  ImageIcon::OnSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto const image = m_rootImage.get())
    {
        image.Source(Source());
    }
}
