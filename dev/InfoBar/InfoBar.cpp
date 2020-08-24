// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBar.h"
#include "InfoBarTemplateSettings.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

InfoBar::InfoBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBar);

    SetValue(s_TemplateSettingsProperty, winrt::make<::InfoBarTemplateSettings>());

    SetDefaultStyleKey(this);
}

void InfoBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement

    UpdateSeverity();
    UpdateIcon();
    UpdateIconVisibility();
    UpdateCloseButton();
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
