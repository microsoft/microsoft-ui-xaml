// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "SortIndicator.g.h"
#include "SortIndicator.properties.h"

class SortIndicator :
    public ReferenceTracker<SortIndicator, winrt::implementation::SortIndicatorT>,
    public SortIndicatorProperties
{
public:
    SortIndicator();

    // IFrameworkElement overrides
    void OnApplyTemplate();

    // IUIElement overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    void OnDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void UpdateVisualState(bool useTransitions);
    tracker_ref<winrt::FrameworkElement> m_layoutRoot{ this };
    tracker_ref<winrt::FontIcon> m_glyphIcon{ this };
};
