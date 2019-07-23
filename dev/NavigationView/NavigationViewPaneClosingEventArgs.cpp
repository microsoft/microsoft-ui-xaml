// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationView.h"
#include "NavigationViewPaneClosingEventArgs.h"
#include "common.h"

bool NavigationViewPaneClosingEventArgs::Cancel()
{
    return m_cancelled;
}

void NavigationViewPaneClosingEventArgs::Cancel(bool value)
{
    m_cancelled = value;

    if (auto args = m_splitViewClosingArgs.get())
    {
        args.Cancel(value);
    }
}

void NavigationViewPaneClosingEventArgs::SplitViewClosingArgs(winrt::SplitViewPaneClosingEventArgs const& value)
{
    m_splitViewClosingArgs.set(value);
}
