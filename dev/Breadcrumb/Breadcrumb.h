// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "Breadcrumb.g.h"
#include "Breadcrumb.properties.h"

#include "SplitButton.h"

class Breadcrumb :
    public ReferenceTracker<Breadcrumb, winrt::implementation::BreadcrumbT>,
    public BreadcrumbProperties
{

public:
    Breadcrumb();
    ~Breadcrumb() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnElementPreparedEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementPreparedEventArgs args);
    void OnElementClearingEvent(winrt::ItemsRepeater sender, winrt::ItemsRepeaterElementClearingEventArgs args);

    winrt::ItemsRepeater::ElementPrepared_revoker m_itemRepeaterElementPreparedRevoker{};
    winrt::ItemsRepeater::ElementClearing_revoker m_itemRepeaterElementClearingRevoker{};
    tracker_ref<winrt::ItemsRepeater> m_breadcrumbItemRepeater { this };
};
