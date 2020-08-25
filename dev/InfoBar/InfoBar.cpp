// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBar.h"
#include "InfoBarClosingEventArgs.h"
#include "InfoBarClosedEventArgs.h"
#include "InfoBarTemplateSettings.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

static constexpr wstring_view c_closeButtonName{ L"CloseButton"sv };

InfoBar::InfoBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBar);

    SetValue(s_TemplateSettingsProperty, winrt::make<::InfoBarTemplateSettings>());

    SetDefaultStyleKey(this);
}

void InfoBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    //const auto spinDownName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxDownSpinButtonName);

    if (const auto closeButton = GetTemplateChildT<winrt::Button>(c_closeButtonName, controlProtected))
    {
        m_closeButtonClickRevoker = closeButton.Click(winrt::auto_revoke, { this, &InfoBar::OnCloseButtonClick });

        // Do localization for the close button
        /* ### if (winrt::AutomationProperties::GetName(closeButton).empty())
        {
            winrt::AutomationProperties::SetName(closeButton, spinDownName);
        }*/
    }

    UpdateVisibility();
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

    winrt::Deferral instance
    { [strongThis = get_strong(), args] ()
        {
            strongThis->CheckThread();
            if (!args->Cancel())
            {
                strongThis->UpdateVisibility();
                strongThis->RaiseClosedEvent();
            }
            else
            {
                // The developer has changed the Cancel property to true, indicating that they wish to Cancel the
                // closing of this tip, so we need to revert the IsOpen property to true.
                strongThis->IsOpen(true);
            }
        }
    };

    args->SetDeferral(instance);

    args->IncrementDeferralCount();
    m_closingEventSource(*this, *args);
    args->DecrementDeferralCount();
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

void InfoBar::OnIsUserDismissablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateCloseButton();
}

void InfoBar::UpdateVisibility()
{
    OutputDebugString(IsOpen() ? L"Going to InfoBarVisible\n" : L"Going to InfoBarCollapsed\n");
    winrt::VisualStateManager::GoToState(*this, IsOpen() ? L"InfoBarVisible" : L"InfoBarCollapsed", false);
}

void InfoBar::UpdateSeverity()
{
    auto severityState = L"Default";

    switch (Severity())
    {
        case winrt::InfoBarSeverity::Critical: severityState = L"Critical"; break;
        case winrt::InfoBarSeverity::Warning:  severityState = L"Warning";  break;
        case winrt::InfoBarSeverity::Success:  severityState = L"Success";  break;
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
    winrt::VisualStateManager::GoToState(*this, IsUserDismissable() ? L"CloseButtonVisible" : L"CloseButtonCollapsed", false);
}
