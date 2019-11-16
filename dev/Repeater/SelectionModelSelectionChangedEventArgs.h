// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectionModelSelectionChangedEventArgs.g.h"

class SelectionModelSelectionChangedEventArgs :
    public winrt::implementation::SelectionModelSelectionChangedEventArgsT<SelectionModelSelectionChangedEventArgs>
{
public:
    SelectionModelSelectionChangedEventArgs() = default;
    SelectionModelSelectionChangedEventArgs(
        winrt::IVectorView<winrt::IndexPath> previousSelectedIndices);

    winrt::IVectorView<winrt::IndexPath> PreviousSelectedIndices();

private:
    winrt::IVectorView<winrt::IndexPath> m_previousSelectedIndices;
};
