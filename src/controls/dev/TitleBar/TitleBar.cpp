// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TitleBar.h"
#include "TitleBarTemplateSettings.h"
#include "TitleBarAutomationPeer.h"
#include "ResourceAccessor.h"
#include "TypeLogging.h"
#include "RuntimeProfiler.h"

bool TitleBarTrace::s_IsDebugOutputEnabled{ false };
bool TitleBarTrace::s_IsVerboseDebugOutputEnabled{ false };

TitleBar::TitleBar()
{
    TITLEBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    __RP_Marker_ClassById(RuntimeProfiler::ProfId_TitleBar);

    SetValue(s_TemplateSettingsProperty, winrt::make<::TitleBarTemplateSettings>());

    SetDefaultStyleKey(this);

    m_sizeChangedRevoker = SizeChanged(winrt::auto_revoke, { this, &TitleBar::OnSizeChanged });
    m_flowDirectionChangedRevoker = RegisterPropertyChanged(*this, winrt::FrameworkElement::FlowDirectionProperty(), { this, &TitleBar::OnFlowDirectionChanged });
}

TitleBar::~TitleBar()
{
    TITLEBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_sizeChangedRevoker.revoke();
    m_flowDirectionChangedRevoker.revoke();

    if (m_inputActivationChangedToken.value)
    {
        m_inputActivationListener.InputActivationChanged(m_inputActivationChangedToken);
        m_inputActivationChangedToken.value = 0;
    }

    const auto appWindow = TryGetAppWindow();
    if (appWindow)
    {
        // Safely restore default if the window title still matches what we last applied
        ResetTitle(Title());
    }
}

winrt::AutomationPeer TitleBar::OnCreateAutomationPeer()
{
    return winrt::make<TitleBarAutomationPeer>(*this);
}

void TitleBar::OnApplyTemplate()
{
    TITLEBAR_TRACE_INFO(*this, TRACE_MSG_METH, METH_NAME, this);

    __super::OnApplyTemplate();

    winrt::IControlProtected controlProtected{ *this };

    m_leftPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(s_leftPaddingColumnName, controlProtected));
    m_rightPaddingColumn.set(GetTemplateChildT<winrt::ColumnDefinition>(s_rightPaddingColumnName, controlProtected));

    auto appWindowId = GetAppWindowId();

    if (appWindowId.Value != 0)
    {
        m_inputActivationListener = winrt::Microsoft::UI::Input::InputActivationListener::GetForWindowId(appWindowId);
        m_inputActivationChangedToken = m_inputActivationListener.InputActivationChanged({ this, &TitleBar::OnInputActivationChanged });
    }

    UpdateHeight();
    UpdatePadding();
    UpdateIcon();
    UpdateBackButton();
    UpdatePaneToggleButton();
    UpdateTitle();
    UpdateSubtitle();
    UpdateLeftHeader();
    UpdateContent();
    UpdateRightHeader();
    UpdateInteractableElementsList();
    UpdateDragRegion();
    UpdateIconRegion();
}

void TitleBar::HandleTitleChange(const winrt::hstring& oldTitle, const winrt::hstring& newTitle)
{
    // If transitioning from non-empty to empty, prefer ResetTitle to avoid overwriting external titles.
    if (!oldTitle.empty() && newTitle.empty())
    {
        ResetTitle(oldTitle);
        GoToState(s_titleTextCollapsedVisualStateName, false);
    }
    else
    {
        UpdateTitle();
    }
}

void TitleBar::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsBackButtonVisibleProperty)
    {
        UpdateBackButton();
    }
    else if (property == s_IsBackButtonEnabledProperty)
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
        const auto oldTitle = winrt::unbox_value_or<winrt::hstring>(args.OldValue(), L"");
        const auto newTitle = Title();

        HandleTitleChange(oldTitle, newTitle);
    }
    else if (property == s_SubtitleProperty)
    {
        UpdateSubtitle();
    }
    if (property == s_LeftHeaderProperty)
    {
        UpdateLeftHeader();
    }
    else if (property == s_ContentProperty)
    {
        UpdateContent();
    }
    else if (property == s_RightHeaderProperty)
    {
        UpdateRightHeader();
    }

    UpdateDragRegion();
    UpdateIconRegion();
}

void TitleBar::GoToState(std::wstring_view const& stateName, bool useTransitions)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, stateName.data(), useTransitions);

    winrt::VisualStateManager::GoToState(*this, stateName, useTransitions);
}

void TitleBar::OnSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (Content() != nullptr)
    {
        const auto contentArea = m_contentArea.get();
        const auto contentAreaGrid = m_contentAreaGrid.get();

        if (contentArea && contentAreaGrid)
        {
            if (!m_compactModeThresholdWidth && contentArea.DesiredSize().Width >= contentAreaGrid.ActualWidth())
            {
                m_compactModeThresholdWidth = args.NewSize().Width;
                m_isCompact = true;
                GoToState(s_compactVisualStateName, false);
            }
            else if (m_isCompact && args.NewSize().Width >= m_compactModeThresholdWidth)
            {
                m_compactModeThresholdWidth = 0.0;
                m_isCompact = false;
                GoToState(s_expandedVisualStateName, false);
                UpdateTitle();
                UpdateSubtitle();
            }
        }
    }

    UpdateDragRegion();
    UpdateIconRegion();
}

void TitleBar::OnFlowDirectionChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdatePadding();
}

void TitleBar::OnInputActivationChanged(const winrt::InputActivationListener& sender, const winrt::InputActivationListenerActivationChangedEventArgs& args)
{
    bool isDeactivated = sender.State() == winrt::InputActivationState::Deactivated;

    TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this, L"isDeactivated:", isDeactivated);

    if (IsBackButtonVisible() && IsBackButtonEnabled())
    {
       GoToState(isDeactivated ? s_backButtonDeactivatedVisualStateName : s_backButtonVisibleVisualStateName, false);
    }

    if (IsPaneToggleButtonVisible())
    {
        GoToState(isDeactivated ? s_paneToggleButtonDeactivatedVisualStateName : s_paneToggleButtonVisibleVisualStateName, false);
    }

    if (IconSource() != nullptr)
    {
       GoToState(isDeactivated ? s_iconDeactivatedVisualStateName : s_iconVisibleVisualStateName, false);
    }

    if (!Title().empty())
    {
        if (!m_isCompact)
        {
            GoToState(isDeactivated ? s_titleTextDeactivatedVisualStateName : s_titleTextVisibleVisualStateName, false);
        }
    }

    if (!Subtitle().empty())
    {
        if (!m_isCompact)
        {
            GoToState(isDeactivated ? s_subtitleTextDeactivatedVisualStateName : s_subtitleTextVisibleVisualStateName, false);
        }
    }

    if (LeftHeader() != nullptr)
    {
        GoToState(isDeactivated ? s_leftHeaderDeactivatedVisualStateName : s_leftHeaderVisibleVisualStateName, false);
    }

    if (Content() != nullptr)
    {
        GoToState(isDeactivated ? s_contentDeactivatedVisualStateName : s_contentVisibleVisualStateName, false);
    }

    if (RightHeader() != nullptr)
    {
        GoToState(isDeactivated ? s_rightHeaderDeactivatedVisualStateName : s_rightHeaderVisibleVisualStateName, false);
    }

    UpdateIconRegion();
}

void TitleBar::OnWindowRectChanged(const winrt::InputNonClientPointerSource& sender, const winrt::WindowRectChangedEventArgs& args)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateIconRegion();
}

void TitleBar::OnBackButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_backRequestedEventSource(*this, nullptr);
}

void TitleBar::OnPaneToggleButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_paneToggleRequestedEventSource(*this, nullptr);
}

void TitleBar::UpdateIcon()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    auto const templateSettings = winrt::get_self<::TitleBarTemplateSettings>(TemplateSettings());
    if (auto const source = IconSource())
    {
        if (!m_iconViewbox.get())
        {
            m_iconViewbox.set(GetTemplateChildT<winrt::FrameworkElement>(s_iconViewboxPartName, *this));
        }

        // 55625016
        // AppWindowTitleBar's InputNonClientPointerSource currently resets all non-passthrough region rects on every interaction.
        // As such, we added extra event listeners to manually update the nonClientPointerSource::Icon rect as a workaround.
        if (auto const iconViewbox = m_iconViewbox.get())
        {
            m_iconLayoutUpdatedRevoker = iconViewbox.LayoutUpdated(winrt::auto_revoke, { this, &TitleBar::OnIconLayoutUpdated });
        }
        else
        {
            m_iconLayoutUpdatedRevoker.revoke();
        }

        if (const auto& nonClientPointerSource = GetInputNonClientPointerSource())
        {
            m_windowRectChangedToken = nonClientPointerSource.WindowRectChanged({ this, &TitleBar::OnWindowRectChanged });
        }
        else if (m_inputNonClientPointerSource)
        {
            m_inputNonClientPointerSource.WindowRectChanged(m_windowRectChangedToken);
            m_windowRectChangedToken.value = 0;
        }

        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
        GoToState(s_iconVisibleVisualStateName, false);
    }
    else
    {
        m_iconLayoutUpdatedRevoker.revoke();

        if (m_inputNonClientPointerSource)
        {
            m_inputNonClientPointerSource.WindowRectChanged(m_windowRectChangedToken);
            m_windowRectChangedToken.value = 0;
        }

        templateSettings->IconElement(nullptr);
        GoToState(s_iconCollapsedVisualStateName, false);
    }

    UpdateDragRegion();
    UpdateIconRegion();
}

void TitleBar::OnIconLayoutUpdated(const winrt::IInspectable& sender, const winrt::IInspectable& args)
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    UpdateIconRegion();
}

void TitleBar::UpdateBackButton()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (IsBackButtonVisible())
    {
        if (!m_backButton.get())
        {
            LoadBackButton();
        }

        GoToState(s_backButtonVisibleVisualStateName, false);
    }
    else
    {
        GoToState(s_backButtonCollapsedVisualStateName, false);
    }
    
    UpdateInteractableElementsList();
    UpdateLeftHeaderSpacing();
}

void TitleBar::UpdatePaneToggleButton()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (IsPaneToggleButtonVisible())
    {
        if (!m_paneToggleButton.get())
        {
            LoadPaneToggleButton();
        }

        GoToState(s_paneToggleButtonVisibleVisualStateName, false);
    }
    else
    {
        GoToState(s_paneToggleButtonCollapsedVisualStateName, false);
    }

    UpdateInteractableElementsList();
    UpdateLeftHeaderSpacing();
}

void TitleBar::UpdateHeight()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    GoToState((Content() == nullptr && LeftHeader() == nullptr && RightHeader() == nullptr) ?
        s_compactHeightVisualStateName : s_expandedHeightVisualStateName,
        false);
}

void TitleBar::UpdatePadding()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    const auto appWindow = TryGetAppWindow();
    if (appWindow)
    {
        // TODO 50724421: Bind to appTitleBar Left and Right inset changed event.
        if (const auto appTitleBar = appWindow.TitleBar())
        {
            if (const auto leftColumn = m_leftPaddingColumn.get())
            {
                const auto leftColumnInset =
                    FlowDirection() == winrt::FlowDirection::LeftToRight ?
                    appTitleBar.LeftInset() :
                    appTitleBar.RightInset();

                TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                    L"LeftColumn width:",
                    leftColumnInset);
                
                leftColumn.Width(winrt::GridLengthHelper::FromPixels(leftColumnInset));
            }

            if (const auto rightColumn = m_rightPaddingColumn.get())
            {
                const auto rightColumnInset =
                    FlowDirection() == winrt::FlowDirection::LeftToRight ?
                    appTitleBar.RightInset() :
                    appTitleBar.LeftInset();

                TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
                    L"RightColumn width:",
                    rightColumnInset);
              
                rightColumn.Width(winrt::GridLengthHelper::FromPixels(rightColumnInset));
            }
        }
    }
}

void TitleBar::UpdateTitle()
{
    const winrt::hstring titleText = Title();

    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, titleText.c_str());

    const auto appWindow = TryGetAppWindow();

    // Capture default title once.
    if (appWindow && !m_hasDefaultAppWindowTitle)
    {
        m_defaultAppWindowTitle = appWindow.Title();
        m_hasDefaultAppWindowTitle = true;
    }

    if (titleText.empty())
    {
        // Do not set appWindow.Title here. Reset is handled by ResetTitle via OnPropertyChanged.
        GoToState(s_titleTextCollapsedVisualStateName, false);
        return;
    }

    // Only set the window title if it actually needs to change.
    if (appWindow)
    {
        const auto currentTitle = appWindow.Title();
        if (currentTitle != titleText)
        {
            appWindow.Title(titleText);
        }
    }

    GoToState(s_titleTextVisibleVisualStateName, false);
}

void TitleBar::ResetTitle(winrt::hstring const& lastAppliedTitle)
{
    TITLEBAR_TRACE_INFO(nullptr, TRACE_MSG_METH, METH_NAME, nullptr);

    if (!m_hasDefaultAppWindowTitle)
    {
        return;
    }

    const auto appWindow = TryGetAppWindow();
    if (!appWindow)
    {
        return;
    }

    // Restore only if the current title matches what we previously applied
    const auto currentTitle = appWindow.Title();
    if (lastAppliedTitle == currentTitle && currentTitle != m_defaultAppWindowTitle)
    {
        appWindow.Title(m_defaultAppWindowTitle);
        m_hasDefaultAppWindowTitle = false;
    }
}

void TitleBar::UpdateSubtitle()
{
    const winrt::hstring subTitleText = Subtitle();

    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR, METH_NAME, this, subTitleText.c_str());

    if (subTitleText.empty())
    {
        GoToState(s_subtitleTextCollapsedVisualStateName, false);
    }
    else
    {
        GoToState(s_subtitleTextVisibleVisualStateName, false);
    }
}

void TitleBar::UpdateLeftHeader()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (LeftHeader() == nullptr)
    {
        GoToState(s_leftHeaderCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_leftHeaderArea.get())
        {
            m_leftHeaderArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_leftHeaderPresenterPartName, *this));
        }
        GoToState(s_leftHeaderVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

void TitleBar::UpdateContent()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (Content() == nullptr)
    {
        GoToState(s_contentCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_contentArea.get())
        {
            winrt::IControlProtected controlProtected{ *this };

            m_contentAreaGrid.set(GetTemplateChildT<winrt::Grid>(s_contentPresenterGridPartName, controlProtected));
            m_contentArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_contentPresenterPartName, controlProtected));
        }

        GoToState(s_contentVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

void TitleBar::UpdateRightHeader()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    if (RightHeader() == nullptr)
    {
        GoToState(s_rightHeaderCollapsedVisualStateName, false);
    }
    else
    {
        if (!m_rightHeaderArea.get())
        {
            m_rightHeaderArea.set(GetTemplateChildT<winrt::FrameworkElement>(s_rightHeaderPresenterPartName, *this));
        }
        GoToState(s_rightHeaderVisibleVisualStateName, false);
    }

    UpdateHeight();
    UpdateInteractableElementsList();
}

winrt::Windows::Graphics::RectInt32 const TitleBar::GetBounds(const winrt::FrameworkElement& element)
{
    const auto transformBounds = element.TransformToVisual(nullptr);
    const auto width = element.ActualWidth();
    const auto height = element.ActualHeight();
    const auto bounds = transformBounds.TransformBounds(winrt::Rect{
        0.0f,
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height) });

    const auto scale = XamlRoot().RasterizationScale();
    const auto returnRect = winrt::Windows::Graphics::RectInt32{
        static_cast<int32_t>(bounds.X * scale),
        static_cast<int32_t>(bounds.Y * scale),
        static_cast<int32_t>(bounds.Width * scale),
        static_cast<int32_t>(bounds.Height * scale),
    };

    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH_STR_STR,
        METH_NAME, this,
        element.Name().c_str(),
        TypeLogging::RectInt32ToString(returnRect).c_str());

    return returnRect;
}

// Once TitleBar control is set as the Window titlebar in developer codebehind, the entire region's input is marked as non-client
// and becomes non interactable. We need to punch out a hole for each interactable region in TitleBar. 
void TitleBar::UpdateDragRegion()
{
    if (const auto& nonClientPointerSource = GetInputNonClientPointerSource())
    {
        if (!m_interactableElementsList.empty())
        {
            std::vector<winrt::Windows::Graphics::RectInt32> passthroughRects;

            // Get rects for each interactable element in TitleBar.
            for (const auto frameworkElement : m_interactableElementsList)
            {
                const auto transparentRect = GetBounds(frameworkElement);

                if (transparentRect.X >= 0 || transparentRect.Y >= 0)
                {
                    passthroughRects.push_back(transparentRect);
                }
            }

            TITLEBAR_TRACE_VERBOSE_DBG(*this, L"%s[0x%p](SetRegionRects - Size: %d)\n", METH_NAME, this, passthroughRects.size());

            // Set list of rects as passthrough regions for the non-client area.
            nonClientPointerSource.SetRegionRects(winrt::NonClientRegionKind::Passthrough, passthroughRects);
        }
        else
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Clear Passthrough RegionRects");

            // There is no interactable areas. Clear previous passthrough rects.
            nonClientPointerSource.ClearRegionRects(winrt::NonClientRegionKind::Passthrough);
        }
    }
}

void TitleBar::UpdateIconRegion()
{
    if (const auto& nonClientPointerSource = GetInputNonClientPointerSource())
    {
        if (IconSource())
        {
            if (const auto iconViewbox = m_iconViewbox.get())
            {               
                std::vector<winrt::Windows::Graphics::RectInt32> iconRects;

                const auto iconRect = GetBounds(iconViewbox);

                if (iconRect.X >= 0 || iconRect.Y >= 0)
                {
                    iconRects.push_back(iconRect);
                }

                TITLEBAR_TRACE_VERBOSE_DBG(*this, L"%s[0x%p](Set Icon RegionRects)\n", METH_NAME, this);

                nonClientPointerSource.SetRegionRects(winrt::NonClientRegionKind::Icon, iconRects);
            }
        }
        else
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Clear Icon RegionRects");

            nonClientPointerSource.ClearRegionRects(winrt::NonClientRegionKind::Icon);
        }
    }
}

void TitleBar::UpdateInteractableElementsList()
{
    m_interactableElementsList.clear();

    if (IsBackButtonVisible() && IsBackButtonEnabled())
    {
        if (const auto backButton = m_backButton.get())
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Append backButton to m_interactableElementsList");

            m_interactableElementsList.push_back(backButton);
        }
    }

    if (IsPaneToggleButtonVisible())
    {
        if (const auto paneToggleButton = m_paneToggleButton.get())
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Append paneToggleButton to m_interactableElementsList");

            m_interactableElementsList.push_back(paneToggleButton);
        }
    }

    if (LeftHeader() != nullptr)
    {
        if (const auto leftHeaderArea = m_leftHeaderArea.get())
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Append headerArea to m_interactableElementsList");

            m_interactableElementsList.push_back(leftHeaderArea);
        }
    }

    if (Content() != nullptr)
    {
        if (const auto contentArea = m_contentArea.get())
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Append contentArea to m_interactableElementsList");

            m_interactableElementsList.push_back(contentArea);
        }
    }

    if (RightHeader() != nullptr)
    {
        if (const auto rightHeaderArea = m_rightHeaderArea.get())
        {
            TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR, METH_NAME, this, L"Append footerArea to m_interactableElementsList");

            m_interactableElementsList.push_back(rightHeaderArea);
        }
    }

    TITLEBAR_TRACE_VERBOSE_DBG(*this, TRACE_MSG_METH_STR_INT, METH_NAME, this,
        L"m_interactableElementsList Size:",
        m_interactableElementsList.size());
}

void TitleBar::UpdateLeftHeaderSpacing()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    GoToState(
        IsBackButtonVisible() == IsPaneToggleButtonVisible() ?
        s_defaultSpacingVisualStateName : s_negativeInsetVisualStateName,
        false);
}

void TitleBar::LoadBackButton()
{
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

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
    TITLEBAR_TRACE_VERBOSE(*this, TRACE_MSG_METH, METH_NAME, this);

    m_paneToggleButton.set(GetTemplateChildT<winrt::Button>(s_paneToggleButtonPartName, *this));

    if (const auto paneToggleButton = m_paneToggleButton.get())
    {
        m_paneToggleButtonClickRevoker = paneToggleButton.Click(winrt::auto_revoke, { this, &TitleBar::OnPaneToggleButtonClick });

        // Do localization for paneToggleButton
        if (winrt::AutomationProperties::GetName(paneToggleButton).empty())
        {
            const auto paneToggleButtonName = ResourceAccessor::GetLocalizedStringResource(SR_NavigationButtonToggleName);
            winrt::AutomationProperties::SetName(paneToggleButton, paneToggleButtonName);
        }

        // Setup the tooltip for the paneToggleButton
        const auto tooltip = winrt::ToolTip();
        tooltip.Content(box_value(winrt::AutomationProperties::GetName(paneToggleButton)));
        winrt::ToolTipService::SetToolTip(paneToggleButton, tooltip);
    }
}

winrt::WindowId TitleBar::GetAppWindowId()
{
    winrt::WindowId appWindowId{};

    if (auto xamlRoot = XamlRoot())
    {
        if (auto contentIslandEnvironment = xamlRoot.ContentIslandEnvironment())
        {
            appWindowId = contentIslandEnvironment.AppWindowId();
        }
    }

    if (appWindowId.Value != m_lastAppWindowId.Value)
    {
        m_lastAppWindowId = appWindowId;

        m_inputNonClientPointerSource = nullptr;

        m_appWindow = nullptr;
    }

    return appWindowId;
}

winrt::InputNonClientPointerSource const& TitleBar::GetInputNonClientPointerSource()
{
    auto appWindowId = GetAppWindowId();

    if (!m_inputNonClientPointerSource && appWindowId.Value != 0)
    {
        m_inputNonClientPointerSource = winrt::InputNonClientPointerSource::GetForWindowId(appWindowId);
    }

    return m_inputNonClientPointerSource;
}

// Helper to retrieve and cache AppWindow for current WindowId
winrt::Microsoft::UI::Windowing::AppWindow TitleBar::TryGetAppWindow()
{
    const auto appWindowId = GetAppWindowId();
    if (appWindowId.Value == 0)
    {
        m_appWindow = nullptr;
        return nullptr;
    }

    // Refresh cache if WindowId changed or cache is empty
    if (!m_appWindow)
    {
        m_appWindow = winrt::Microsoft::UI::Windowing::AppWindow::GetFromWindowId(appWindowId);
    }

    return m_appWindow;
}
