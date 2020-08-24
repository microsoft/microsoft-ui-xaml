// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "InfoBar.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

InfoBar::InfoBar()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_InfoBar);

    SetDefaultStyleKey(this);
}

void InfoBar::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };

    // TODO: Implement

    UpdateSeverity();
    UpdateCloseButton();
}

void InfoBar::OnSeverityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSeverity();
}


void InfoBar::OnShowCloseButtonPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
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

void InfoBar::UpdateCloseButton()
{
    winrt::VisualStateManager::GoToState(*this, ShowCloseButton() ? L"CloseButtonVisible" : L"CloseButtonCollapsed", false);
}
