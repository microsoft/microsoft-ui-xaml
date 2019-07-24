// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SharedHelpers.h"
#include "Vector.h"
#include "DisplayRegionHelper.h"
#include "LifetimeHandler.h"

DisplayRegionHelper::DisplayRegionHelper() 
{
};

DisplayRegionHelper::~DisplayRegionHelper()
{
}

/* static */
DisplayRegionHelperInfo DisplayRegionHelper::GetRegionInfo()
{
    auto instance = LifetimeHandler::GetDisplayRegionHelperInstance();

    DisplayRegionHelperInfo info;
    info.RegionCount = 1;
    info.Mode = winrt::TwoPaneViewMode::SinglePane;

    OutputDebugString(L"  GetRegionInfo()\n");

    if (instance->m_simulateDisplayRegions)
    {
        // Create fake rectangles for test app
        if (instance->m_simulateMode == winrt::TwoPaneViewMode::Wide)
        {
            info.RegionCount = 2;
            info.Regions[0] = m_simulateWide0;
            info.Regions[1] = m_simulateWide1;
            info.Mode = winrt::TwoPaneViewMode::Wide;
        }
        else if (instance->m_simulateMode == winrt::TwoPaneViewMode::Tall)
        {
            info.RegionCount = 2;
            info.Regions[0] = m_simulateTall0;
            info.Regions[1] = m_simulateTall1;
            info.Mode = winrt::TwoPaneViewMode::Tall;
        }
        else
        {
            info.RegionCount = 1;
            info.Regions[0] = m_simulateWide0;
        }
    }
    else if (SharedHelpers::IsApplicationViewGetDisplayRegionsAvailable())
    {
        WCHAR strOut[1024];
        OutputDebugString(L"    DisplayRegions available\n");

        // ApplicationView::GetForCurrentView throws on failure; in that case we just won't do anything.
        winrt::ApplicationView view{ nullptr };
        try
        {
            view = winrt::ApplicationView::GetForCurrentView();
        } catch(...) {}

        // Verify that the window is Tiled
        if (view)
        {
            OutputDebugString(L"    Got ApplicationView\n");

            auto regions = view.GetDisplayRegions();
            info.RegionCount = std::min(regions.Size(), c_maxRegions);

            StringCchPrintf(strOut, ARRAYSIZE(strOut), L"    Regions found: %d\n", (int)(regions.Size()));
            OutputDebugString(strOut);

            // More than one region
            if (info.RegionCount == 2)
            {
                OutputDebugString(L"    2 regions found\n");

                winrt::Rect windowRect = WindowRect();

                if (windowRect.Width > windowRect.Height)
                {
                    OutputDebugString(L"    Double portrait\n");

                    info.Mode = winrt::TwoPaneViewMode::Wide;
                    float width = windowRect.Width / 2;
                    info.Regions[0] = { 0, 0, width, windowRect.Height };
                    info.Regions[1] = { width, 0, width, windowRect.Height };
                }
                else
                {
                    OutputDebugString(L"    Double landscape\n");

                    info.Mode = winrt::TwoPaneViewMode::Tall;
                    float height = windowRect.Height / 2;
                    info.Regions[0] = { 0, 0, windowRect.Width, height };
                    info.Regions[1] = { 0, height, windowRect.Width, height };
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

        if (auto fe = safe_cast<winrt::FrameworkElement>(winrt::Window::Current().Content()))
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

