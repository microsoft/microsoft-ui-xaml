// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TwoPaneView.h"
#include "DisplayRegionHelperTestApi.h"
#include "RuntimeProfiler.h"

static constexpr auto c_pane1ScrollViewerName = L"PART_Pane1ScrollViewer";
static constexpr auto c_pane2ScrollViewerName = L"PART_Pane2ScrollViewer";

static constexpr auto c_columnLeftName   = L"PART_ColumnLeft"sv;
static constexpr auto c_columnMiddleName = L"PART_ColumnMiddle"sv;
static constexpr auto c_columnRightName  = L"PART_ColumnRight"sv;
static constexpr auto c_rowTopName       = L"PART_RowTop"sv;
static constexpr auto c_rowMiddleName    = L"PART_RowMiddle"sv;
static constexpr auto c_rowBottomName    = L"PART_RowBottom"sv;

using namespace std;

TwoPaneView::TwoPaneView()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TwoPaneView);

    SetDefaultStyleKey(this);

    SizeChanged({ this, &TwoPaneView::OnSizeChanged });
    m_windowSizeChangedRevoker = winrt::Window::Current().SizeChanged(winrt::auto_revoke, { this, &TwoPaneView::OnWindowSizeChanged });

    EnsureProperties();
}

void TwoPaneView::OnApplyTemplate()
{
    m_loaded = true;

    winrt::IControlProtected controlProtected = *this;

    SetScrollViewerProperties(c_pane1ScrollViewerName, m_pane1LoadedRevoker);
    SetScrollViewerProperties(c_pane2ScrollViewerName, m_pane2LoadedRevoker);

    if (auto column = GetTemplateChildT<winrt::ColumnDefinition>(c_columnLeftName, controlProtected))
    {
        m_columnLeft.set(column);
    }
    if (auto column = GetTemplateChildT<winrt::ColumnDefinition>(c_columnMiddleName, controlProtected))
    {
        m_columnMiddle.set(column);
    }
    if (auto column = GetTemplateChildT<winrt::ColumnDefinition>(c_columnRightName, controlProtected))
    {
        m_columnRight.set(column);
    }
    if (auto row = GetTemplateChildT<winrt::RowDefinition>(c_rowTopName, controlProtected))
    {
        m_rowTop.set(row);
    }
    if (auto row = GetTemplateChildT<winrt::RowDefinition>(c_rowMiddleName, controlProtected))
    {
        m_rowMiddle.set(row);
    }
    if (auto row = GetTemplateChildT<winrt::RowDefinition>(c_rowBottomName, controlProtected))
    {
        m_rowBottom.set(row);
    }
}

void TwoPaneView::SetScrollViewerProperties(std::wstring_view const& scrollViewerName, winrt::Control::Loaded_revoker& revoker)
{
    if (SharedHelpers::IsRS5OrHigher())
    {
        if (auto scrollViewer = GetTemplateChildT<winrt::FxScrollViewer>(scrollViewerName, *this))
        {
            if (SharedHelpers::IsScrollContentPresenterSizesContentToTemplatedParentAvailable())
            {
                revoker = scrollViewer.Loaded(winrt::auto_revoke, { this, &TwoPaneView::OnScrollViewerLoaded });
            }

            if (SharedHelpers::IsScrollViewerReduceViewportForCoreInputViewOcclusionsAvailable())
            {
                scrollViewer.ReduceViewportForCoreInputViewOcclusions(true);
            }
        }
    }
}

void TwoPaneView::OnScrollViewerLoaded(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    if (auto scrollViewer = sender.as<winrt::FrameworkElement>())
    {
        auto scrollContentPresenterFE = SharedHelpers::FindInVisualTreeByName(scrollViewer, L"ScrollContentPresenter");
        if (scrollContentPresenterFE)
        {
            if (auto scrollContentPresenter = scrollContentPresenterFE.as<winrt::ScrollContentPresenter>())
            {
                scrollContentPresenter.SizesContentToTemplatedParent(true);
            }
        }
    }
}

void TwoPaneView::OnWindowSizeChanged(const winrt::IInspectable& sender, const winrt::WindowSizeChangedEventArgs& args)
{
    UpdateMode();
}

void TwoPaneView::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    UpdateMode();
}

void TwoPaneView::UpdateMode()
{
    // Don't bother running this logic until after we hit OnApplyTemplate.
    if (!m_loaded) return;

    const double controlWidth = ActualWidth();
    const double controlHeight = ActualHeight();

    ViewMode newMode = (PanePriority() == winrt::TwoPaneViewPriority::Pane1) ? ViewMode::Pane1Only : ViewMode::Pane2Only;

    // Calculate new mode
    const DisplayRegionHelperInfo info = DisplayRegionHelper::GetRegionInfo();
    const winrt::Rect rcControl = GetControlRect();
    const bool isInMultipleRegions = IsInMultipleRegions(info, rcControl);

    if (isInMultipleRegions)
    {
        if (info.Mode == winrt::TwoPaneViewMode::Wide)
        {
            // Regions are laid out horizontally
            if (WideModeConfiguration() != winrt::TwoPaneViewWideModeConfiguration::SinglePane)
            {
                newMode = (WideModeConfiguration() == winrt::TwoPaneViewWideModeConfiguration::LeftRight) ? ViewMode::LeftRight : ViewMode::RightLeft;
            }
        }
        else if (info.Mode == winrt::TwoPaneViewMode::Tall)
        {
            // Regions are laid out vertically
            if (TallModeConfiguration() != winrt::TwoPaneViewTallModeConfiguration::SinglePane)
            {
                newMode = (TallModeConfiguration() == winrt::TwoPaneViewTallModeConfiguration::TopBottom) ? ViewMode::TopBottom : ViewMode::BottomTop;
            }
        }
    }
    else
    {
        // One region
        if (controlWidth > MinWideModeWidth() && WideModeConfiguration() != winrt::TwoPaneViewWideModeConfiguration::SinglePane)
        {
            // Split horizontally
            newMode = (WideModeConfiguration() == winrt::TwoPaneViewWideModeConfiguration::LeftRight) ? ViewMode::LeftRight : ViewMode::RightLeft;
        }
        else if (controlHeight > MinTallModeHeight() && TallModeConfiguration() != winrt::TwoPaneViewTallModeConfiguration::SinglePane)
        {
            // Split vertically
            newMode = (TallModeConfiguration() == winrt::TwoPaneViewTallModeConfiguration::TopBottom) ? ViewMode::TopBottom : ViewMode::BottomTop;
        }
    }

    // Update row/column sizes (this may need to happen even if the mode doesn't change)
    UpdateRowsColumns(newMode, info, rcControl);

    // Update mode if necessary
    if (newMode != m_currentMode)
    {
        m_currentMode = newMode;

        winrt::TwoPaneViewMode newViewMode = winrt::TwoPaneViewMode::SinglePane;

        switch (m_currentMode)
        {
            case ViewMode::Pane1Only: winrt::VisualStateManager::GoToState(*this, L"ViewMode_OneOnly", true); break;
            case ViewMode::Pane2Only: winrt::VisualStateManager::GoToState(*this, L"ViewMode_TwoOnly", true); break;
            case ViewMode::LeftRight: winrt::VisualStateManager::GoToState(*this, L"ViewMode_LeftRight", true); newViewMode = winrt::TwoPaneViewMode::Wide; break;
            case ViewMode::RightLeft: winrt::VisualStateManager::GoToState(*this, L"ViewMode_RightLeft", true); newViewMode = winrt::TwoPaneViewMode::Wide; break;
            case ViewMode::TopBottom: winrt::VisualStateManager::GoToState(*this, L"ViewMode_TopBottom", true); newViewMode = winrt::TwoPaneViewMode::Tall; break;
            case ViewMode::BottomTop: winrt::VisualStateManager::GoToState(*this, L"ViewMode_BottomTop", true); newViewMode = winrt::TwoPaneViewMode::Tall; break;
        }

        if (newViewMode != Mode())
        {
            SetValue(s_ModeProperty, box_value(newViewMode));
            m_modeChangedEventSource(*this, *this);
        }
    }
}

void TwoPaneView::UpdateRowsColumns(ViewMode newMode, DisplayRegionHelperInfo info, winrt::Rect rcControl)
{
    if (m_columnLeft && m_columnMiddle && m_columnRight && m_rowTop && m_rowMiddle && m_rowBottom)
    {
        // Reset split lengths
        m_columnMiddle.get().Width({ 0, winrt::GridUnitType::Pixel });
        m_rowMiddle.get().Height({ 0, winrt::GridUnitType::Pixel });

        // Set columns lengths
        if (newMode == ViewMode::LeftRight || newMode == ViewMode::RightLeft)
        {
            m_columnLeft.get().Width((newMode == ViewMode::LeftRight) ? Pane1Length() : Pane2Length());
            m_columnRight.get().Width((newMode == ViewMode::LeftRight) ? Pane2Length() : Pane1Length());
        }
        else
        {
            m_columnLeft.get().Width({ 1, winrt::GridUnitType::Star });
            m_columnRight.get().Width({ 0, winrt::GridUnitType::Pixel });
        }

        // Set row lengths
        if (newMode == ViewMode::TopBottom || newMode == ViewMode::BottomTop)
        {
            m_rowTop.get().Height((newMode == ViewMode::TopBottom) ? Pane1Length() : Pane2Length());
            m_rowBottom.get().Height((newMode == ViewMode::TopBottom) ? Pane2Length() : Pane1Length());
        }
        else
        {
            m_rowTop.get().Height({ 1, winrt::GridUnitType::Star });
            m_rowBottom.get().Height({ 0, winrt::GridUnitType::Pixel });
        }

        // Handle regions
        if (IsInMultipleRegions(info, rcControl) && newMode != ViewMode::Pane1Only && newMode != ViewMode::Pane2Only)
        {
            const winrt::Rect rc1 = info.Regions[0];
            const winrt::Rect rc2 = info.Regions[1];
            const winrt::Rect rcWindow = DisplayRegionHelper::WindowRect();

            if (info.Mode == winrt::TwoPaneViewMode::Wide)
            {
                m_columnMiddle.get().Width({ rc2.X - rc1.Width, winrt::GridUnitType::Pixel });

                m_columnLeft.get().Width({ rc1.Width - rcControl.X , winrt::GridUnitType::Pixel });
                m_columnRight.get().Width({ rc2.Width - ((rcWindow.Width - rcControl.Width) - rcControl.X) , winrt::GridUnitType::Pixel });
            }
            else
            {
                m_rowMiddle.get().Height({ rc2.Y - rc1.Height, winrt::GridUnitType::Pixel });

                m_rowTop.get().Height({ rc1.Height - rcControl.Y , winrt::GridUnitType::Pixel });
                m_rowBottom.get().Height({ rc2.Height - ((rcWindow.Height - rcControl.Height) - rcControl.Y) , winrt::GridUnitType::Pixel });
            }
        }
    }
}

winrt::Rect TwoPaneView::GetControlRect()
{
    // Find out where this control is in the window
    winrt::GeneralTransform transform = TransformToVisual(DisplayRegionHelper::WindowElement());
    return transform.TransformBounds({ 0, 0, (float)ActualWidth(), (float)ActualHeight() });
}

bool TwoPaneView::IsInMultipleRegions(DisplayRegionHelperInfo info, winrt::Rect rcControl)
{
    bool isInMultipleRegions = false;

    if (info.Mode != winrt::TwoPaneViewMode::SinglePane)
    {
        const winrt::Rect rc1 = info.Regions[0];
        const winrt::Rect rc2 = info.Regions[1];

        if (info.Mode == winrt::TwoPaneViewMode::Wide)
        {
            // Check that the control is over the split
            if (rcControl.X < rc1.Width && rcControl.X + rcControl.Width > rc2.X)
            {
                isInMultipleRegions = true;
            }
        }
        else if (info.Mode == winrt::TwoPaneViewMode::Tall)
        {
            // Check that the control is over the split
            if (rcControl.Y < rc1.Height && rcControl.Y + rcControl.Height > rc2.Y)
            {
                isInMultipleRegions = true;
            }
        }
    }

    return isInMultipleRegions;
}

void TwoPaneView::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto property = args.Property();

    // Clamp property values -- early return if the values were clamped as we'll come back with the new value.
    if (property == s_MinWideModeWidthProperty || property == s_MinTallModeHeightProperty)
    {
        auto value = winrt::unbox_value<double>(args.NewValue());
        auto clampedValue = std::max(0.0, value);
        if (clampedValue != value)
        {
            SetValue(property, winrt::box_value(clampedValue));
            return;
        }
    }

    if (property == s_PanePriorityProperty
        || property == s_Pane1LengthProperty
        || property == s_Pane2LengthProperty
        || property == s_WideModeConfigurationProperty
        || property == s_TallModeConfigurationProperty
        || property == s_MinWideModeWidthProperty
        || property == s_MinTallModeHeightProperty)
    {
        UpdateMode();
    }
}
