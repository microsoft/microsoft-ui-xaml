﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ExpanderTemplateSettings.h"

#include "Expander.g.h"
#include "Expander.properties.h"

class Expander :
    public ReferenceTracker<Expander, winrt::implementation::ExpanderT>,
    public ExpanderProperties
{
public:
    Expander();
    ~Expander() {}

    // IUIElement
    virtual winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();


    void RaiseExpandingEvent(const winrt::Expander& container);
    void RaiseCollapsedEvent(const winrt::Expander& container);

    void OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnExpandDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void UpdateExpandDirection(bool useTransitions);
    void UpdateExpandState(bool useTransitions);
    void OnContentSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args);

    winrt::Border::SizeChanged_revoker m_expanderContentSizeChangedRevoker{};
};
