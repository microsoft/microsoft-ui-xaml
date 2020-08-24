// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InfoBar.g.h"
#include "InfoBar.properties.h"

class InfoBar :
    public ReferenceTracker<InfoBar, winrt::implementation::InfoBarT>,
    public InfoBarProperties
{

public:
    InfoBar();
    ~InfoBar() {}

    // IFrameworkElement
    void OnApplyTemplate();

    // Property change handlers
    void OnSeverityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsIconVisiblePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsUserDismissablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void UpdateSeverity();
    void UpdateIcon();
    void UpdateIconVisibility();
    void UpdateCloseButton();
};
