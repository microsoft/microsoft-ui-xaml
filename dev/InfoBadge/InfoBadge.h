// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InfoBadgeTemplateSettings.h"

#include "InfoBadge.g.h"
#include "InfoBadge.properties.h"

class InfoBadge :
    public ReferenceTracker<InfoBadge, winrt::implementation::InfoBadgeT>,
    public InfoBadgeProperties
{

public:
    InfoBadge();
    ~InfoBadge() {}

    // IFrameworkElement
    void OnApplyTemplate();
    winrt::Size MeasureOverride(winrt::Size const& availableSize);

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    void OnIsOpenChanged();
    void OnDisplayKindPropertiesChanged();
    void OnSizeChanged(const winrt::IInspectable&, const winrt::SizeChangedEventArgs& args);
};
