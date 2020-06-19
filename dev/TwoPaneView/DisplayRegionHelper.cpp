// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SharedHelpers.h"
#include "Vector.h"
#include "DisplayRegionHelper.h"
#include "LifetimeHandler.h"

// TODO: Remove once ApplicationViewMode::Spanning is available in the SDK
const int c_ApplicationViewModeSpanning = 2;

/* static */
DisplayRegionHelperInfo DisplayRegionHelper::GetRegionInfo()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();

    DisplayRegionHelperInfo info;
    info.Mode = winrt::TwoPaneViewMode::SinglePane;

    if (instance->m_simulateDisplayRegions)
    {
        // Create fake rectangles for test app
        if (instance->m_simulateMode == winrt::TwoPaneViewMode::Wide)
        {
            info.Regions[0] = m_simulateWide0;
            info.Regions[1] = m_simulateWide1;
            info.Mode = winrt::TwoPaneViewMode::Wide;
        }
        else if (instance->m_simulateMode == winrt::TwoPaneViewMode::Tall)
        {
            info.Regions[0] = m_simulateTall0;
            info.Regions[1] = m_simulateTall1;
            info.Mode = winrt::TwoPaneViewMode::Tall;
        }
        else
        {
            info.Regions[0] = m_simulateWide0;
        }
    }
    else
    {
        // ApplicationView::GetForCurrentView throws on failure; in that case we just won't do anything.
        winrt::ApplicationView view{ nullptr };
        try
        {
            view = winrt::ApplicationView::GetForCurrentView();
        } catch(...) {}

        if (view && view.ViewMode() == (winrt::Windows::UI::ViewManagement::ApplicationViewMode)c_ApplicationViewModeSpanning)
        {
            if (const auto appView = view.try_as<winrt::IApplicationViewSpanningRects>())
            {
                winrt::IVectorView<winrt::Rect> rects = appView.GetSpanningRects();

                if (rects.Size() == 2)
                {
                    info.Regions[0] = rects.GetAt(0);
                    info.Regions[1] = rects.GetAt(1);

                    // Determine orientation. If neither of these are true, default to doing nothing.
                    if (info.Regions[0].X < info.Regions[1].X && info.Regions[0].Y == info.Regions[1].Y)
                    {
                        // Double portrait
                        info.Mode = winrt::TwoPaneViewMode::Wide;
                    }
                    else if (info.Regions[0].X == info.Regions[1].X && info.Regions[0].Y < info.Regions[1].Y)
                    {
                        // Double landscape
                        info.Mode = winrt::TwoPaneViewMode::Tall;
                    }
                }
            }
        }
    }

    return info;
}

/* static */
winrt::UIElement DisplayRegionHelper::WindowElement()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();

    if (instance->m_simulateDisplayRegions)
    {
        // Instead of returning the actual window, find the SimulatedWindow element
        winrt::UIElement window = nullptr;

        if (auto fe = winrt::Window::Current().Content().as<winrt::FrameworkElement>())
        {
            window = SharedHelpers::FindInVisualTreeByName(fe, L"SimulatedWindow");
        }

        return window;
    }
    else
    {
        return winrt::Window::Current().Content();
    }
}

/* static */
winrt::Rect DisplayRegionHelper::WindowRect()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();

    if (instance->m_simulateDisplayRegions)
    {
        // Return the bounds of the simulated window
        winrt::FrameworkElement window = DisplayRegionHelper::WindowElement().as<winrt::FrameworkElement>();
        winrt::Rect rc = {
            0, 0,
            (float)window.ActualWidth(),
            (float)window.ActualHeight() };
        return rc;
    }
    else
    {
        return winrt::Window::Current().Bounds();
    }
}

/* static */
void DisplayRegionHelper::SimulateDisplayRegions(bool value)
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();
    instance->m_simulateDisplayRegions = value;
}

/* static */
bool DisplayRegionHelper::SimulateDisplayRegions()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();
    return instance->m_simulateDisplayRegions;
}

/* static */
void DisplayRegionHelper::SimulateMode(winrt::TwoPaneViewMode value)
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();
    instance->m_simulateMode = value;
}

/* static */
winrt::TwoPaneViewMode DisplayRegionHelper::SimulateMode()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();
    return instance->m_simulateMode;
}

