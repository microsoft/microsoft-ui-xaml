// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ExpanderCollapsedEventArgs.g.h"

class ExpanderCollapsedEventArgs :
    public ReferenceTracker<ExpanderCollapsedEventArgs, winrt::implementation::ExpanderCollapsedEventArgsT, winrt::composing, winrt::composable>
{
public:
    ExpanderCollapsedEventArgs(winrt::Expander const& expander);

    winrt::IInspectable CollapsedContent();
    void CollapsedContent(winrt::IInspectable const& value);

private:
    tracker_ref<winrt::Expander> m_expander{ this };
    tracker_ref<winrt::IInspectable> m_collapsedContainer{ this };
};
