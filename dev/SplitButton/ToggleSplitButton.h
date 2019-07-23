// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "pch.h"

#include "SplitButton.h"
#include "ToggleSplitButton.g.h"
#include "ToggleSplitButton.properties.h"

class ToggleSplitButton :
    public winrt::implementation::ToggleSplitButtonT<ToggleSplitButton, SplitButton>,
    public ToggleSplitButtonProperties
{

public:
    ForwardRefToBaseReferenceTracker(SplitButton)

    ToggleSplitButton();
    ~ToggleSplitButton();

    // Lift up EnsureProperties and ClearProperties to resolve ambiguity between ToggleSplitButtonProperties and SplitButtonProperties
    using ToggleSplitButtonProperties::EnsureProperties;
    using ToggleSplitButtonProperties::ClearProperties;

    // IFrameworkElement
    void OnApplyTemplate();

    // IUIElement
    winrt::AutomationPeer OnCreateAutomationPeer() override;

    // Internal
    void Toggle();
    void OnClickPrimary(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args) override;
    bool InternalIsChecked() override;

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnIsCheckedChanged();
};
