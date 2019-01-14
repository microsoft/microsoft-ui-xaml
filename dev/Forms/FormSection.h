// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "FormSection.g.h"
#include "FormSection.properties.h"

class FormSection :
    public ReferenceTracker<FormSection, winrt::implementation::FormSectionT>,
    public FormSectionProperties
{

public:
    FormSection();
    ~FormSection() {}

    static constexpr winrt::GridLength s_defaultLength{ 1, winrt::GridUnitType::Star };

    // IFrameworkElementOverrides
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

    static void OnLengthPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    static void OnBuddiesPropertyChanged(
        const winrt::DependencyObject& sender,
        const winrt::DependencyPropertyChangedEventArgs& args);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
