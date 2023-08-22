// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

const uint32_t c_maxRegions = 2;

struct DisplayRegionHelperInfo
{
    winrt::TwoPaneViewMode Mode{ winrt::TwoPaneViewMode::SinglePane };
    std::array<winrt::Rect, c_maxRegions> Regions{};
};

class DisplayRegionHelper:
    public winrt::implements<DisplayRegionHelper, winrt::IInspectable>
{
public:
    static DisplayRegionHelperInfo GetRegionInfo();

    static winrt::Rect WindowRect();
    static winrt::UIElement WindowElement();

    static void SimulateDisplayRegions(bool value);
    static bool SimulateDisplayRegions();

    static void SimulateMode(winrt::TwoPaneViewMode value);
    static winrt::TwoPaneViewMode SimulateMode();

private:

    void OnDisplayModeChanged(const winrt::IInspectable& sender, const winrt::IInspectable& args);

    bool m_simulateDisplayRegions{ false };
    winrt::TwoPaneViewMode m_simulateMode{ winrt::TwoPaneViewMode::SinglePane };
    
    static constexpr winrt::Rect m_simulateWide0{ 0, 0, 300, 400 };
    static constexpr winrt::Rect m_simulateWide1{ 312, 0, 300, 400 };
    static constexpr winrt::Rect m_simulateTall0{ 0, 0, 400, 300 };
    static constexpr winrt::Rect m_simulateTall1{ 0, 312, 400, 300 };
};
