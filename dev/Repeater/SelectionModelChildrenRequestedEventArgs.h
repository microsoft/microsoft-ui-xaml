// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectionModelChildrenRequestedEventArgs.g.h"

class SelectionModelChildrenRequestedEventArgs :
    public ReferenceTracker<SelectionModelChildrenRequestedEventArgs, winrt::implementation::SelectionModelChildrenRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    explicit SelectionModelChildrenRequestedEventArgs(const winrt::IInspectable& source);

#pragma region ISelectionModelChildrenRequestedEventArgs
    winrt::IInspectable Source();
    winrt::IInspectable Children();
    void Children(winrt::IInspectable const& value);
#pragma endregion

    void Initialize(const winrt::IInspectable& source);

private:
    tracker_ref<winrt::IInspectable> m_source{ this };
    tracker_ref<winrt::IInspectable> m_children{ this };
};