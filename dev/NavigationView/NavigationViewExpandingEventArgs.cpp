// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewExpandingEventArgs.h"

winrt::NavigationViewItemBase NavigationViewExpandingEventArgs::ExpandingItemContainer()
{
    return m_expandingItemContainer.get();
}

void NavigationViewExpandingEventArgs::ExpandingItemContainer(winrt::NavigationViewItemBase const& value)
{
    m_expandingItemContainer.set(value);
}
