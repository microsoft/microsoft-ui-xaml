// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DisplayRegionHelper.h"

#include "TwoPaneView.g.h"
#include "TwoPaneView.properties.h"

static constexpr double c_defaultMinWideModeWidth{ 641.0 };
static constexpr double c_defaultMinTallModeHeight{ 641.0 };

static constexpr winrt::GridLength c_pane1LengthDefault{ 1, winrt::GridUnitType::Auto };
static constexpr winrt::GridLength c_pane2LengthDefault{ 1, winrt::GridUnitType::Star };

enum class ViewMode
{
    Pane1Only,
    Pane2Only,
    LeftRight,
    RightLeft,
    TopBottom,
    BottomTop,
    None
};

class TwoPaneView :
    public ReferenceTracker<TwoPaneView, winrt::implementation::TwoPaneViewT>,
    public TwoPaneViewProperties
{
public:
    TwoPaneView();

    // IFrameworkElement
    void OnApplyTemplate();


    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

private:
    void SetScrollViewerProperties(std::wstring_view const& scrollViewerName, winrt::Control::Loaded_revoker& revoker);
    void OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);

    void OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);
    void OnWindowSizeChanged(const winrt::IInspectable& sender, const winrt::WindowSizeChangedEventArgs& args);

    void UpdateRowsColumns(ViewMode newMode, DisplayRegionHelperInfo info, winrt::Rect rcControl);
    void UpdateMode();

    winrt::Rect GetControlRect();
    bool IsInMultipleRegions(DisplayRegionHelperInfo info, winrt::Rect rcControl);

    ViewMode m_currentMode { ViewMode::None } ;

    bool m_loaded { false };

    winrt::Control::Loaded_revoker m_pane1LoadedRevoker{};
    winrt::Control::Loaded_revoker m_pane2LoadedRevoker{};

    tracker_ref<winrt::ColumnDefinition> m_columnLeft{ this };
    tracker_ref<winrt::ColumnDefinition> m_columnMiddle{ this };
    tracker_ref<winrt::ColumnDefinition> m_columnRight{ this };
    tracker_ref<winrt::RowDefinition> m_rowTop{ this };
    tracker_ref<winrt::RowDefinition> m_rowMiddle{ this };
    tracker_ref<winrt::RowDefinition> m_rowBottom{ this };
    winrt::IWindow::SizeChanged_revoker m_windowSizeChangedRevoker{};

};
