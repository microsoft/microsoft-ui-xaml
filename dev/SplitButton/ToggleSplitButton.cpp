// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ToggleSplitButton.h"
#include "SplitButton.h"
#include "SplitButtonEventArgs.h"
#include "ToggleSplitButtonAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

ToggleSplitButton::ToggleSplitButton()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SplitButton);

    SetDefaultStyleKey(this);
}

ToggleSplitButton::~ToggleSplitButton()
{
}

void ToggleSplitButton::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_IsCheckedProperty)
    {
        OnIsCheckedChanged();
    }
}

winrt::AutomationPeer ToggleSplitButton::OnCreateAutomationPeer()
{
    return winrt::make<ToggleSplitButtonAutomationPeer>(*this);
}

void ToggleSplitButton::OnIsCheckedChanged()
{
    if (m_hasLoaded)
    {
        auto eventArgs = winrt::make_self<ToggleSplitButtonIsCheckedChangedEventArgs>();
        m_isCheckedChangedEventSource(*this, *eventArgs);
    }

    UpdateVisualStates();
}

void ToggleSplitButton::OnClickPrimary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args)
{
    Toggle();

    __super::OnClickPrimary(sender, args);
}

bool ToggleSplitButton::InternalIsChecked()
{
    return IsChecked();
}

void ToggleSplitButton::Toggle()
{
    IsChecked(!IsChecked());
}
