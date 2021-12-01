// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "InfoBarPanel.g.h"
#include "InfoBarPanel.properties.h"

class InfoBarPanel :
    public ReferenceTracker<InfoBarPanel, winrt::implementation::InfoBarPanelT>,
    public InfoBarPanelProperties
{
public:
    winrt::Size MeasureOverride(winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

private:
    bool m_isVertical{ false };
};
