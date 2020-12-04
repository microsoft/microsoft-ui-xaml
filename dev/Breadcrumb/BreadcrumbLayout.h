// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "StackLayout.h"
#include "VirtualizingLayout.h"

#include "BreadcrumbLayout.g.h"

struct BreadcrumbLayout :
    winrt::implementation::BreadcrumbLayoutT<BreadcrumbLayout, StackLayout>
{
public:
    BreadcrumbLayout();
    ~BreadcrumbLayout();

    winrt::Size MeasureOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::VirtualizingLayoutContext const& context, winrt::Size const& finalSize);

private:
    void InstantiateEllipsisButton(winrt::VirtualizingLayoutContext const& context);
    void ArrangeItem(winrt::VirtualizingLayoutContext const& context, int index, float& accumulatedWidths, float& maxElementHeight);
    void HideItem(winrt::VirtualizingLayoutContext const& context, int index);
    int GetFirstBreadcrumbItemToArrange(winrt::VirtualizingLayoutContext const& context);

    bool justCreatedEllipsisButton{};
    winrt::Size m_availableSize;
    tracker_ref<winrt::BreadcrumbItem> m_ellipsisButton{ this };
};
