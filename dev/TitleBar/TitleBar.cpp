// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TitleBar.h"
#include "TitleBarTemplateSettings.h"
#include "TitleBarAutomationPeer.h"
#include "ResourceAccessor.h"

TitleBar::TitleBar()
{
    SetValue(s_TemplateSettingsProperty, winrt::make<::TitleBarTemplateSettings>());

    SetDefaultStyleKey(this);

    SizeChanged({ this, &TitleBar::OnSizeChanged });

    if (const auto currentView = winrt::CoreApplication::GetCurrentView())
    {
        if (const auto coreTitleBar = currentView.TitleBar())
        {
            m_titleBarMetricsChangedRevoker = coreTitleBar.LayoutMetricsChanged(winrt::auto_revoke, { this, &TitleBar::OnTitleBarMetricsChanged });
            m_titleBarIsVisibleChangedRevoker = coreTitleBar.IsVisibleChanged(winrt::auto_revoke, { this, &TitleBar::OnTitleBarIsVisibleChanged });
        }
    }

    if (const auto window = winrt::Window::Current())
    {
        window.Activated({ this, &TitleBar::OnWindowActivated });
    }

    if (winrt::IFrameworkElement6 frameworkElement6 = *this)
    {
        m_actualThemeChangedRevoker = frameworkElement6.ActualThemeChanged(winrt::auto_revoke,
                [this](auto&&, auto&&) { UpdateTheme(); });
    }
}

winrt::AutomationPeer TitleBar::OnCreateAutomationPeer()
{
    return winrt::make<TitleBarAutomationPeer>(*this);
}

void TitleBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    if (const auto currentView = winrt::CoreApplication::GetCurrentView())
    {
        if (const auto coreTitleBar = currentView.TitleBar())
        {
            coreTitleBar.ExtendViewIntoTitleBar(true);
        }
    }

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(L"LayoutRoot", controlProtected));
    m_leftPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"LeftPaddingColumn", controlProtected));
    m_rightPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"RightPaddingColumn", controlProtected));

    m_titleTextBlock.set(GetTemplateChildT<winrt::TextBlock>(L"TitleText", controlProtected));
    m_customArea.set(GetTemplateChildT<winrt::FrameworkElement>(L"CustomContentPresenter", controlProtected));

    if (const auto window = winrt::Window::Current())
    {
        if (const auto dragRegion = GetTemplateChildT<winrt::Grid>(L"DragRegion", controlProtected))
        {
            window.SetTitleBar(dragRegion);
        }
        else
        {
            window.SetTitleBar(nullptr);
        }
    }

    if (const auto backButton = GetTemplateChildT<winrt::Button>(L"BackButton", controlProtected))
    {
        m_backButtonClickRevoker = backButton.Click(winrt::auto_revoke, { this, &TitleBar::OnBackButtonClick });

        // Do localization for the back button
        if (winrt::AutomationProperties::GetName(backButton).empty())
        {
            const auto backButtonName = ResourceAccessor::GetLocalizedStringResourceFromWinUI(SR_NavigationBackButtonName);
            winrt::AutomationProperties::SetName(backButton, backButtonName);
        }

        // Setup the tooltip for the back button
        const auto tooltip = winrt::ToolTip();
        const auto backButtonTooltipText = ResourceAccessor::GetLocalizedStringResourceFromWinUI(SR_NavigationBackButtonToolTip);
        tooltip.Content(box_value(backButtonTooltipText));
        winrt::ToolTipService::SetToolTip(backButton, tooltip);
    }

    UpdateVisibility();
    UpdateHeight();
    UpdatePadding();
    UpdateIcon();
    UpdateBackButton();
    UpdateTheme();
    UpdateTitle();
}

void TitleBar::OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_backRequestedEventSource(*this, nullptr);
}

void TitleBar::OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateIcon();
}

void TitleBar::OnIsBackButtonVisiblePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateBackButton();
}

void TitleBar::OnCustomContentPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateHeight();
}

void TitleBar::OnTitlePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateTitle();
}

void TitleBar::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    const auto titleTextBlock = m_titleTextBlock.get();
    const auto customArea = m_customArea.get();
    if (titleTextBlock && customArea)
    {
        auto const templateSettings = winrt::get_self<::TitleBarTemplateSettings>(TemplateSettings());

        if (m_isTitleSquished)
        {
            // If the title column has * sizing but it's not trimmed anymore, then give the extra space back to the custom area.
            if (!titleTextBlock.IsTextTrimmed())
            {
                templateSettings->TitleColumnGridLength({ 1, winrt::GridUnitType::Auto });
                templateSettings->CustomColumnGridLength({ 1, winrt::GridUnitType::Star });

                m_isTitleSquished = false;
            }
        }
        else
        {
            // If the custom area is at its minimum width, switch the title column to be * sized so it squishes instead.
            if (!m_isTitleSquished && customArea.DesiredSize().Width >= customArea.ActualWidth())
            {
                templateSettings->TitleColumnGridLength({ 1, winrt::GridUnitType::Star });
                templateSettings->CustomColumnGridLength({ 1, winrt::GridUnitType::Auto });

                m_isTitleSquished = true;
            }
        }
    }
}

void TitleBar::OnWindowActivated(const winrt::IInspectable& sender, const winrt::WindowActivatedEventArgs& args)
{
    winrt::VisualStateManager::GoToState(*this,
        (args.WindowActivationState() == winrt::CoreWindowActivationState::Deactivated) ? L"Deactivated" : L"Activated",
        false);
}

void TitleBar::OnTitleBarMetricsChanged(const winrt::IInspectable& /*sender*/, const winrt::IInspectable& /*args*/)
{
    UpdatePadding();
}

void TitleBar::OnTitleBarIsVisibleChanged(const winrt::CoreApplicationViewTitleBar& sender, const winrt::IInspectable& /*args*/)
{
    UpdateVisibility();
}

void TitleBar::UpdateIcon()
{
    auto const templateSettings = winrt::get_self<::TitleBarTemplateSettings>(TemplateSettings());
    if (auto const source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, L"IconVisible", false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, L"IconCollapsed", false);
    }
}

void TitleBar::UpdateBackButton()
{
    winrt::VisualStateManager::GoToState(*this,
        IsBackButtonVisible() ? L"BackButtonVisible" : L"BackButtonCollapsed",
        false);
}

void TitleBar::UpdateVisibility()
{
    if (const auto currentView = winrt::CoreApplication::GetCurrentView())
    {
        if (const auto coreTitleBar = currentView.TitleBar())
        {
            winrt::VisualStateManager::GoToState(*this,
                coreTitleBar.IsVisible() ? L"TitleBarVisible" : L"TitleBarCollapsed",
                false);
        }
    }
}

void TitleBar::UpdateHeight()
{
    winrt::VisualStateManager::GoToState(*this,
        (CustomContent() == nullptr) ? L"CompactHeight" : L"ExpandedHeight",
        false);
}

void TitleBar::UpdatePadding()
{
    if (const auto currentView = winrt::CoreApplication::GetCurrentView())
    {
        if (const auto coreTitleBar = currentView.TitleBar())
        {
            if (const auto leftColumn = m_leftPaddingColumn.get())
            {
                leftColumn.Width(winrt::GridLengthHelper::FromPixels(coreTitleBar.SystemOverlayLeftInset()));
            }

            if (const auto rightColumn = m_rightPaddingColumn.get())
            {
                rightColumn.Width(winrt::GridLengthHelper::FromPixels(coreTitleBar.SystemOverlayRightInset()));
            }
        }
    }
}

void TitleBar::UpdateTheme()
{
    if (const auto appView = winrt::ApplicationView::GetForCurrentView())
    {
        if (const auto titleBar = appView.TitleBar())
        {
            // rest colors
            const auto buttonForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonForegroundColor")).as<winrt::Color>();
            titleBar.ButtonForegroundColor(buttonForegroundColor);

            const auto buttonBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonBackgroundColor")).as<winrt::Color>();
            titleBar.ButtonBackgroundColor(buttonBackgroundColor);
            titleBar.ButtonInactiveBackgroundColor(buttonBackgroundColor);

            // hover colors
            const auto buttonHoverForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonHoverForegroundColor")).as<winrt::Color>();
            titleBar.ButtonHoverForegroundColor(buttonHoverForegroundColor);

            const auto buttonHoverBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonHoverBackgroundColor")).as<winrt::Color>();
            titleBar.ButtonHoverBackgroundColor(buttonHoverBackgroundColor);

            // pressed colors
            const auto buttonPressedForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonPressedForegroundColor")).as<winrt::Color>();
            titleBar.ButtonPressedForegroundColor(buttonPressedForegroundColor);

            const auto buttonPressedBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonPressedBackgroundColor")).as<winrt::Color>();
            titleBar.ButtonPressedBackgroundColor(buttonPressedBackgroundColor);

            // inactive foreground
            const auto buttonInactiveForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(L"TitleBarButtonInactiveForegroundColor")).as<winrt::Color>();
            titleBar.ButtonInactiveForegroundColor(buttonInactiveForegroundColor);
        }
    }
}

void TitleBar::UpdateTitle()
{
    const winrt::hstring titleText = Title();
    if (titleText.empty())
    {
        winrt::VisualStateManager::GoToState(*this, L"TitleTextCollapsed", false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"TitleTextVisible", false);
    }
}

