// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MenuBarItemFlyout.g.h"

class MenuBarItemFlyout : 
    public ReferenceTracker<MenuBarItemFlyout, winrt::implementation::MenuBarItemFlyoutT>
{
public:
    // Property changed event handler.
    static void OnPropertyChanged(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args) {}

    winrt::Control CreatePresenter();

    tracker_ref<winrt::Control> m_presenter{ this };
};

