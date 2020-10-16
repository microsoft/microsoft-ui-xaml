// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ExpanderExpandingEventArgs.g.h"

class ExpanderExpandingEventArgs :
    public ReferenceTracker<ExpanderExpandingEventArgs, winrt::implementation::ExpanderExpandingEventArgsT, winrt::composing, winrt::composable>
{
public:
    ExpanderExpandingEventArgs(winrt::Expander const& expander);

    winrt::IInspectable ExpandingContent();
    void ExpandingContent(winrt::IInspectable const& value);

private:
    tracker_ref<winrt::Expander> m_expander{ this };
    tracker_ref<winrt::IInspectable> m_expandingContent{ this };
};
