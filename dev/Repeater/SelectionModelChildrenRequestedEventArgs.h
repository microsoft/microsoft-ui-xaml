// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectionModelChildrenRequestedEventArgs.g.h"

class SelectionModelChildrenRequestedEventArgs :
    public ReferenceTracker<SelectionModelChildrenRequestedEventArgs, winrt::implementation::SelectionModelChildrenRequestedEventArgsT, winrt::composable, winrt::composing>
{
public:
    SelectionModelChildrenRequestedEventArgs(const winrt::IInspectable& data, SelectionNode* sourceNode);

#pragma region ISelectionModelChildrenRequestedEventArgs
    winrt::IInspectable Source();
    winrt::IndexPath SourceIndex();
    winrt::IInspectable Children();
    void Children(winrt::IInspectable const& value);
#pragma endregion

    void Initialize(const winrt::IInspectable& source, SelectionNode* sourceNode);

private:
    tracker_ref<winrt::IInspectable> m_source{ this };
    tracker_ref<winrt::IInspectable> m_children{ this };

    // Lifetime of sourceNode is owned by the caller and is guaranteed to be alive
    // during the event handler call.
    SelectionNode* m_sourceNode{ nullptr };
};
