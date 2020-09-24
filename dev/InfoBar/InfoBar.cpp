// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBar.h"
#include "InfoBarClosingEventArgs.h"
#include "InfoBarClosedEventArgs.h"
#include "InfoBarTemplateSettings.h"
#include "InfoBarAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "../ResourceHelper/Utils.h"

static constexpr wstring_view c_closeButtonName{ L"CloseButton"sv };

InfoBar::InfoBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBar);

    SetValue(s_TemplateSettingsProperty, winrt::make<::InfoBarTemplateSettings>());

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer InfoBar::OnCreateAutomationPeer()
{
    return winrt::make<InfoBarAutomationPeer>(*this);
}

void InfoBar::OnApplyTemplate()
{
    m_applyTemplateCalled = true;

    winrt::IControlProtected controlProtected{ *this };

    if (const auto closeButton = GetTemplateChildT<winrt::Button>(c_closeButtonName, controlProtected))
    {
        m_closeButtonClickRevoker = closeButton.Click(winrt::auto_revoke, { this, &InfoBar::OnCloseButtonClick });

        // Do localization for the close button
        if (winrt::AutomationProperties::GetName(closeButton).empty())
        {
            const auto closeButtonName = ResourceAccessor::GetLocalizedStringResource(SR_InfoBarCloseButtonName);
            winrt::AutomationProperties::SetName(closeButton, closeButtonName);
        }
    }

    UpdateVisibility(m_notifyOpen, true);
    m_notifyOpen = false;

    UpdateSeverity();
    UpdateIcon();
    UpdateIconVisibility();
    UpdateCloseButton();
}

void InfoBar::OnCloseButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    m_closeButtonClickEventSource(*this, nullptr);
    m_lastCloseReason = winrt::InfoBarCloseReason::CloseButton;
    IsOpen(false);
}

void InfoBar::RaiseClosingEvent()
{
    auto const args = winrt::make_self<InfoBarClosingEventArgs>();
    args->Reason(m_lastCloseReason);

    m_closingEventSource(*this, *args);

    if (!args->Cancel())
    {
        UpdateVisibility();
        RaiseClosedEvent();
    }
    else
    {
        // The developer has changed the Cancel property to true,
        // so we need to revert the IsOpen property to true.
        IsOpen(true);
    }
}

void InfoBar::RaiseClosedEvent()
{
    auto const args = winrt::make_self<InfoBarClosedEventArgs>();
    args->Reason(m_lastCloseReason);
    m_closedEventSource(*this, *args);
}

void InfoBar::OnIsOpenPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (IsOpen())
    {
        //Reset the close reason to the default value of programmatic.
        m_lastCloseReason = winrt::InfoBarCloseReason::Programmatic;

        UpdateVisibility();
    }
    else
    {
        RaiseClosingEvent();
    }
}

void InfoBar::OnSeverityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSeverity();
}

void InfoBar::OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateIcon();
    UpdateIconVisibility();
}

void InfoBar::OnIsIconVisiblePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateIconVisibility();
}

void InfoBar::OnIsClosablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCloseButton();
}

void InfoBar::UpdateVisibility(bool notify, bool force)
{
    auto const peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).try_as<winrt::InfoBarAutomationPeer>();
    if (!m_applyTemplateCalled)
    {
        // ApplyTemplate() hasn't been called yet but IsOpen has already been set.
        // Since this method will be called again shortly from ApplyTemplate, we'll just wait so that we don't send two notifications.
        m_notifyOpen = true;
    }
    else
    {
        // Don't do any work if nothing has changed (unless we are forcing a update)
        if (force || IsOpen() != m_isVisible)
        {
            if (IsOpen())
            {
                if (notify && peer)
                {
                    auto const notificationString = StringUtil::FormatString(
                        ResourceAccessor::GetLocalizedStringResource(SR_InfoBarOpenedNotification),
                        Title().data(),
                        Message().data());

                    winrt::get_self<InfoBarAutomationPeer>(peer)->RaiseOpenedEvent(Severity(), notificationString);
                }

                winrt::VisualStateManager::GoToState(*this, L"InfoBarVisible", false);
                winrt::AutomationProperties::SetAccessibilityView(*this, winrt::AccessibilityView::Control);
                m_isVisible = true;
            }
            else
            {
                if (notify && peer)
                {
                    auto const notificationString = ResourceAccessor::GetLocalizedStringResource(SR_InfoBarClosedNotification);

                    winrt::get_self<InfoBarAutomationPeer>(peer)->RaiseClosedEvent(Severity(), notificationString);
                }

                winrt::VisualStateManager::GoToState(*this, L"InfoBarCollapsed", false);
                winrt::AutomationProperties::SetAccessibilityView(*this, winrt::AccessibilityView::Raw);
                m_isVisible = false;
            }
        }
    }
}

void InfoBar::UpdateSeverity()
{
    auto severityState = L"Informational";

    switch (Severity())
    {
        case winrt::InfoBarSeverity::Success:  severityState = L"Success";  break;
        case winrt::InfoBarSeverity::Warning:  severityState = L"Warning";  break;
        case winrt::InfoBarSeverity::Critical: severityState = L"Critical"; break;
    };

    winrt::VisualStateManager::GoToState(*this, severityState, false);
}

void InfoBar::UpdateIcon()
{
    auto const templateSettings = winrt::get_self<::InfoBarTemplateSettings>(TemplateSettings());
    if (auto const source = IconSource())
    {
        templateSettings->IconElement(SharedHelpers::MakeIconElementFrom(source));
    }
    else
    {
        templateSettings->IconElement(nullptr);
    }
}

void InfoBar::UpdateIconVisibility()
{
    winrt::VisualStateManager::GoToState(*this, IsIconVisible() ? (IconSource() ? L"UserIconVisible" : L"StandardIconVisible") : L"NoIconVisible", false);
}

void InfoBar::UpdateCloseButton()
{
    winrt::VisualStateManager::GoToState(*this, IsClosable() ? L"CloseButtonVisible" : L"CloseButtonCollapsed", false);
}
