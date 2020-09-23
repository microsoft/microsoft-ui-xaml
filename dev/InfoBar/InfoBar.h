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

    // UIElement
    winrt::AutomationPeer OnCreateAutomationPeer();

    // Property change handlers
    void OnIsOpenPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSeverityPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIconSourcePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsIconVisiblePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsClosablePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnCloseButtonClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    void RaiseClosingEvent();
    void RaiseClosedEvent();

    void UpdateVisibility(bool notify = true, bool force = false);
    void UpdateSeverity();
    void UpdateIcon();
    void UpdateIconVisibility();
    void UpdateCloseButton();

    winrt::InfoBarCloseReason m_lastCloseReason{ winrt::InfoBarCloseReason::Programmatic };

    winrt::Button::Click_revoker m_closeButtonClickRevoker{};

    bool m_applyTemplateCalled{ false };
    bool m_notifyOpen{ false };
    bool m_isVisible{ false };
};
