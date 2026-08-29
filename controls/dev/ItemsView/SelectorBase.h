// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsViewTrace.h"

class SelectorBase
{
public:
    SelectorBase();
    ~SelectorBase();

    void SetSelectionModel(winrt::SelectionModel const& selectionModel);
    void DeselectWithAnchorPreservation(int index);
    virtual void SelectAll();
    virtual void Clear();

    virtual void OnInteractedAction(winrt::IndexPath const& index, bool ctrl, bool shift) { };
    virtual void OnFocusedAction(winrt::IndexPath const& index, bool ctrl, bool shift) { };

protected:
    winrt::SelectionModel GetSelectionModel()
    {
        return m_selectionModel;
    }

    bool IsSelected(winrt::IndexPath const& index);
    virtual bool CanSelect(winrt::IndexPath const& index);

private:
    winrt::SelectionModel m_selectionModel{ nullptr };
};
