// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewCollapsedEventArgs.h"

winrt::NavigationViewItemBase NavigationViewCollapsedEventArgs::CollapsedItemContainer()
{
    return m_collapsedItemContainer.get();
}

void NavigationViewCollapsedEventArgs::CollapsedItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_collapsedItemContainer.set(value);
}
