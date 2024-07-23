// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TitleBar.h"
#include "TitleBarTemplateSettings.h"
#include "TitleBarAutomationPeer.h"
#include "ResourceAccessor.h"
#include <winrt/Microsoft.UI.Windowing.h>

TitleBar::TitleBar()
{
    SetValue(s_TemplateSettingsProperty, winrt::make<::TitleBarTemplateSettings>());

    SetDefaultStyleKey(this);

    SizeChanged({ this, &TitleBar::OnSizeChanged });
    LayoutUpdated({ this, &TitleBar::OnLayoutUpdated });

    if (const winrt::IFrameworkElement frameworkElement = *this)
    {
        m_actualThemeChangedRevoker = frameworkElement.ActualThemeChanged(winrt::auto_revoke,
                [this](auto&&, auto&&) { UpdateTheme(); });
    }
}

TitleBar::~TitleBar()
{
    if (m_inputActivationChangedToken.value)
    {
        m_inputActivationListener.InputActivationChanged(m_inputActivationChangedToken);
        m_inputActivationChangedToken.value = 0;
    }
}

winrt::AutomationPeer TitleBar::OnCreateAutomationPeer()
{
    return winrt::make<TitleBarAutomationPeer>(*this);
}

void TitleBar::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_leftPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(s_leftPaddingColumnName, controlProtected));
    m_rightPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(s_rightPaddingColumnName, controlProtected));

    if (const auto xamlRoot = XamlRoot())
    {
        if (const auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            const auto appWindowId = contentIslandEnvironment.AppWindowId();

            if (appWindowId.Value != 0)
            {
                m_inputActivationListener = winrt::Microsoft::UI::Input::InputActivationListener::GetForWindowId(appWindowId);
                m_inputActivationChangedToken = m_inputActivationListener.InputActivationChanged({ this, &TitleBar::OnInputActivationChanged });
            }
        }
    }

    UpdateHeight();
    UpdatePadding();
    UpdateIcon();
    UpdateBackButton();
    UpdatePaneToggleButton();
    UpdateTheme();
    UpdateTitle();
    UpdateSubtitle();
    UpdateHeader();
    UpdateContent();
    UpdateFooter();
    UpdateInteractableElementsList();
}

void TitleBar::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsBackButtonVisibleProperty)
    {
        UpdateBackButton();
    }
    else if (property == s_IsBackEnabledProperty)
    {
        UpdateInteractableElementsList();
    }
    else if (property == s_IsPaneToggleButtonVisibleProperty)
    {
        UpdatePaneToggleButton();
    }
    if (property == s_IconSourceProperty)
    {
        UpdateIcon();
    }
    else if (property == s_TitleProperty)
    {
        UpdateTitle();
    }
    else if (property == s_SubtitleProperty)
    {
        UpdateSubtitle();
    }
    if (property == s_HeaderProperty)
    {
        UpdateHeader();
    }
    else if (property == s_ContentProperty)
    {
        UpdateContent();
    }
    else if (property == s_FooterProperty)
    {
        UpdateFooter();
    }
}

void TitleBar::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    if (Content() != nullptr)
    {
        const auto contentArea = m_contentArea.get();
        const auto contentAreaGrid = m_contentAreaGrid.get();

        if (contentArea && contentAreaGrid)
        {
            if (!m_compactModeThresholdWidth && contentArea.DesiredSize().Width >= contentAreaGrid.ActualWidth())
            {
                m_compactModeThresholdWidth = args.NewSize().Width;
                winrt::VisualStateManager::GoToState(*this, s_compactVisualStateName, false);
            }
            else if (args.NewSize().Width >= m_compactModeThresholdWidth)
            {
                m_compactModeThresholdWidth = 0.0;
                winrt::VisualStateManager::GoToState(*this, s_expandedVisualStateName, false);
                UpdateTitle();
                UpdateSubtitle();
            }
        }
    }

    UpdateDragRegion();
}

void TitleBar::OnLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    UpdateDragRegion();
}

void TitleBar::OnInputActivationChanged(const winrt::InputActivationListener& sender, const winrt::InputActivationListenerActivationChangedEventArgs& args)
{
    bool isDeactivated = sender.State() == winrt::InputActivationState::Deactivated;

    if (IsBackButtonVisible() && IsBackEnabled())
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_backButtonDeactivatedVisualStateName : s_backButtonVisibleVisualStateName, false);
    }

    if (IsPaneToggleButtonVisible())
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_paneToggleButtonDeactivatedVisualStateName : s_paneToggleButtonVisibleVisualStateName, false);
    }

    if (IconSource() != nullptr)
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_iconDeactivatedVisualStateName : s_iconVisibleVisualStateName, false);
    }

    if (!Title().empty())
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_titleTextDeactivatedVisualStateName : s_titleTextVisibleVisualStateName, false);
    }

    if (!Subtitle().empty())
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_subtitleTextDeactivatedVisualStateName : s_subtitleTextVisibleVisualStateName, false);
    }

    if (Header() != nullptr)
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_headerDeactivatedVisualStateName : s_headerVisibleVisualStateName, false);
    }

    if (Content() != nullptr)
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_contentDeactivatedVisualStateName : s_contentVisibleVisualStateName, false);
    }

    if (Footer() != nullptr)
    {
        winrt::VisualStateManager::GoToState(*this,
            isDeactivated ? s_footerDeactivatedVisualStateName : s_footerVisibleVisualStateName, false);
    }
}

void TitleBar::OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_backRequestedEventSource(*this, nullptr);
}

void TitleBar::OnPaneToggleButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_paneToggleRequestedEventSource(*this, nullptr);
}

void TitleBar::UpdateIcon()
{
    auto const templateSettings = winrt::get_self<::TitleBarTemplateSettings>(TemplateSettings());
    if (auto const source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        winrt::VisualStateManager::GoToState(*this, s_iconVisibleVisualStateName, false);
    }
    else
    {
        templateSettings->IconElement(nullptr);
        winrt::VisualStateManager::GoToState(*this, s_iconCollapsedVisualStateName, false);
    }
}

void TitleBar::UpdateBackButton()
{
    if (IsBackButtonVisible())
    {
        if (!m_backButton.get())
        {
            LoadBackButton();
        }

        winrt::VisualStateManager::GoToState(*this, s_backButtonVisibleVisualStateName, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_backButtonCollapsedVisualStateName, false);
    }
    
    UpdateInteractableElementsList();
}

void TitleBar::UpdatePaneToggleButton()
{
    if (IsPaneToggleButtonVisible())
    {
        if (!m_paneToggleButton.get())
        {
            LoadPaneToggleButton();
        }

        winrt::VisualStateManager::GoToState(*this, s_paneToggleButtonVisibleVisualStateName, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_paneToggleButtonCollapsedVisualStateName, false);
    }

    UpdateInteractableElementsList();
}

void TitleBar::UpdateHeight()
{
    winrt::VisualStateManager::GoToState(*this,
        (Content() == nullptr && Header() == nullptr && Footer() == nullptr) ?
        s_compactHeightVisualStateName : s_expandedHeightVisualStateName,
        false);
}

void TitleBar::UpdatePadding()
{
    if (const auto xamlRoot = XamlRoot())
    {
        if (const auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            const auto appWindowId = contentIslandEnvironment.AppWindowId();
            const winrt::Microsoft::UI::Windowing::AppWindow appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(appWindowId);

            if (const auto appTitleBar = appWindow.TitleBar())
            {
                // TODO 50724421: Bind to appTitleBar Left and Right inset changed event.
                if (const auto leftColumn = m_leftPaddingColumn.get())
                {
                    leftColumn.Width(winrt::GridLengthHelper::FromPixels(appTitleBar.LeftInset()));
                }

                if (const auto rightColumn = m_rightPaddingColumn.get())
                {
                    rightColumn.Width(winrt::GridLengthHelper::FromPixels(appTitleBar.RightInset()));
                }
            }
        }
    }
}

void TitleBar::UpdateTheme()
{
    if (const auto xamlRoot = XamlRoot())
    {
        if (const auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            const auto appWindowId = contentIslandEnvironment.AppWindowId();
            const winrt::Microsoft::UI::Windowing::AppWindow appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(appWindowId);

            // AppWindow TitleBar's caption buttons does not update colors with theme change.
            // We need to set them here.
            if (const auto appTitleBar = appWindow.TitleBar())
            {
                // Rest colors.
                const auto buttonForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonForegroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonForegroundColor(buttonForegroundColor);

                const auto buttonBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonBackgroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonBackgroundColor(buttonBackgroundColor);
                appTitleBar.ButtonInactiveBackgroundColor(buttonBackgroundColor);

                // Hover colors.
                const auto buttonHoverForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonHoverForegroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonHoverForegroundColor(buttonHoverForegroundColor);

                const auto buttonHoverBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonHoverBackgroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonHoverBackgroundColor(buttonHoverBackgroundColor);

                // Pressed colors.
                const auto buttonPressedForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonPressedForegroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonPressedForegroundColor(buttonPressedForegroundColor);

                const auto buttonPressedBackgroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonPressedBackgroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonPressedBackgroundColor(buttonPressedBackgroundColor);

                // Inactive foreground.
                const auto buttonInactiveForegroundColor = ResourceAccessor::ResourceLookup(*this, box_value(s_titleBarButtonInactiveForegroundColorName)).as<winrt::Color>();
                appTitleBar.ButtonInactiveForegroundColor(buttonInactiveForegroundColor);
            }
        }
    }
}

void TitleBar::UpdateTitle()
{
    const winrt::hstring titleText = Title();
    if (titleText.empty())
    {
        winrt::VisualStateManager::GoToState(*this, s_titleTextCollapsedVisualStateName, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_titleTextVisibleVisualStateName, false);
    }
}

void TitleBar::UpdateSubtitle()
{
    const winrt::hstring titleText = Title();
    if (titleText.empty())
    {
        winrt::VisualStateManager::GoToState(*this, s_subtitleTextCollapsedVisualStateName, false);
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, s_subtitleTextVisibleVisualStateName, false);
    }
}

void TitleBar::UpdateHeader()
{
    if (Header() == nullptr)
    {
        winrt::VisualStateManager::GoToState(*this, s_headerCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_headerArea.get())
        {
            m_headerArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_headerContentPresenterPartName, *this));
        }
        winrt::VisualStateManager::GoToState(*this, s_headerVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

void TitleBar::UpdateContent()
{
    if (Content() == nullptr)
    {
        winrt::VisualStateManager::GoToState(*this, s_contentCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_contentArea.get())
        {
            winrt::IControlProtected controlProtected{ *this };

            m_contentAreaGrid.set(GetTemplateChildT<winrt::Grid>(s_contentPresenterGridPartName, controlProtected));
            m_contentArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_contentPresenterPartName, controlProtected));
        }

        winrt::VisualStateManager::GoToState(*this, s_contentVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

void TitleBar::UpdateFooter()
{
    if (Footer() == nullptr)
    {
        winrt::VisualStateManager::GoToState(*this, s_footerCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_footerArea.get())
        {
            m_footerArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_footerPresenterPartName, *this));
        }
        winrt::VisualStateManager::GoToState(*this, s_footerVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

// Once TitleBar control is set as the Window titlebar in developer codebehind, the entire region's input is marked as non-client
// and becomes non interactable. We need to punch out a hole for each interactable region in TitleBar. 
void TitleBar::UpdateDragRegion()
{
    if (const auto xamlRoot = XamlRoot())
    {
        if (const auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            auto appWindowId = contentIslandEnvironment.AppWindowId();
            winrt::InputNonClientPointerSource nonClientPointerSource = winrt::InputNonClientPointerSource::GetForWindowId(appWindowId);

            if (!m_interactableElementsList.empty())
            {
                std::vector<winrt::Windows::Graphics::RectInt32> passthroughRects;

                // Get rects for each interactable element in TitleBar.
                for (const auto frameworkElement : m_interactableElementsList)
                {
                    const auto transformBounds = frameworkElement.TransformToVisual(nullptr);
                    const auto width = frameworkElement.ActualWidth();
                    const auto height = frameworkElement.ActualHeight();
                    const auto bounds = transformBounds.TransformBounds(winrt::Rect{
                        0.0f,
                        0.0f,
                        static_cast<float>(width),
                        static_cast<float>(height) });

                    if (bounds.X < 0 || bounds.Y < 0)
                    {
                        continue;
                    }

                    const auto scale = xamlRoot.RasterizationScale();
                    const auto transparentRect = winrt::Windows::Graphics::RectInt32{
                        static_cast<int32_t>(bounds.X * scale),
                        static_cast<int32_t>(bounds.Y * scale),
                        static_cast<int32_t>(bounds.Width * scale),
                        static_cast<int32_t>(bounds.Height * scale),
                    };

                    passthroughRects.push_back(transparentRect);
                }

                // Set list of rects as passthrough regions for the non-client area.
                nonClientPointerSource.SetRegionRects(winrt::NonClientRegionKind::Passthrough, passthroughRects);
            }
            else
            {
                // There is no interactable areas. Clear previous passthrough rects.
                nonClientPointerSource.ClearRegionRects(winrt::NonClientRegionKind::Passthrough);
            }
        }
    }
}

void TitleBar::UpdateInteractableElementsList()
{
    m_interactableElementsList.clear();

    if (IsBackButtonVisible() && IsBackEnabled())
    {
        if (const auto backButton = m_backButton.get())
        {
            m_interactableElementsList.push_back(backButton);
        }
    }

    if (IsPaneToggleButtonVisible())
    {
        if (const auto paneToggleButton = m_paneToggleButton.get())
        {
            m_interactableElementsList.push_back(paneToggleButton);
        }
    }

    if (Header() != nullptr)
    {
        if (const auto headerArea = m_headerArea.get())
        {
            m_interactableElementsList.push_back(headerArea);
        }
    }

    if (Content() != nullptr)
    {
        if (const auto contentArea = m_contentArea.get())
        {
            m_interactableElementsList.push_back(contentArea);
        }
    }

    if (Footer() != nullptr)
    {
        if (const auto footerArea = m_footerArea.get())
        {
            m_interactableElementsList.push_back(footerArea);
        }
    }
}

void TitleBar::LoadBackButton()
{
    m_backButton.set(GetTemplateChildT<winrt::Button>(s_backButtonPartName, *this));

    if (const auto backButton = m_backButton.get())
    {
        m_backButtonClickRevoker = backButton.Click(winrt::auto_revoke, { this, &TitleBar::OnBackButtonClick });

        // Do localization for the back button
        if (winrt::AutomationProperties::GetName(backButton).empty())
        {
            const auto backButtonName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationBackButtonName);
            winrt::AutomationProperties::SetName(backButton, backButtonName);
        }

        // Setup the tooltip for the back button
        const auto tooltip = winrt::ToolTip();
        const auto backButtonTooltipText = ResourceAccessor::GetLocalizedStringResource(SR_NavigationBackButtonToolTip);
        tooltip.Content(box_value(backButtonTooltipText));
        winrt::ToolTipService::SetToolTip(backButton, tooltip);
    }
}

void TitleBar::LoadPaneToggleButton()
{
    m_paneToggleButton.set(GetTemplateChildT<winrt::Button>(s_paneToggleButtonPartName, *this));

    if (const auto paneToggleButton = m_paneToggleButton.get())
    {
        m_paneToggleButtonClickRevoker = paneToggleButton.Click(winrt::auto_revoke, { this, &TitleBar::OnPaneToggleButtonClick });

        // Do localization for paneToggleButton
        if (winrt::AutomationProperties::GetName(paneToggleButton).empty())
        {
            const auto paneToggleButtonName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationButtonOpenName);
            winrt::AutomationProperties::SetName(paneToggleButton, paneToggleButtonName);
        }

        // Setup the tooltip for the paneToggleButton
        const auto tooltip = winrt::ToolTip();
        tooltip.Content(box_value(winrt::AutomationProperties::GetName(paneToggleButton)));
        winrt::ToolTipService::SetToolTip(paneToggleButton, tooltip);
    }
}
