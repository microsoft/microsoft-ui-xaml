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

    if (const auto appView = winrt::ApplicationView::GetForCurrentView())
    {
        if (const auto titleBar = appView.TitleBar())
        {
            const auto transparentColor = box_value<winrt::Windows::UI::Color>(winrt::Windows::UI::Colors::Transparent()).as<winrt::IReference<winrt::Color>>();

            titleBar.ButtonBackgroundColor(transparentColor);
            titleBar.ButtonInactiveBackgroundColor(transparentColor);
        }
    }

    m_layoutRoot.set(GetTemplateChildT<winrt::Grid>(L"LayoutRoot", controlProtected));
    m_leftPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"LeftPaddingColumn", controlProtected));
    m_rightPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(L"RightPaddingColumn", controlProtected));

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
    UpdatePadding();
    UpdateIcon();
    UpdateBackButton();
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
