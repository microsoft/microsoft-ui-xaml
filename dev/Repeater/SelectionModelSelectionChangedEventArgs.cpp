// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemsRepeater.common.h"
#include "SelectionModelSelectionChangedEventArgs.h"

SelectionModelSelectionChangedEventArgs::SelectionModelSelectionChangedEventArgs(winrt::IVectorView<winrt::IndexPath> previousSelectedIndices) :
    m_previousSelectedIndices(previousSelectedIndices)
{
}

#pragma region ISelectionModelSelectionChangedEventArgs
winrt::IVectorView<winrt::IndexPath> SelectionModelSelectionChangedEventArgs::PreviousSelectedIndices()
{
    return m_previousSelectedIndices;
}
#pragma endregion
