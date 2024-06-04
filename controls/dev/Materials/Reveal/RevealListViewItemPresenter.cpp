// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RevealListViewItemPresenter.h"
#include "RevealBrush.h"

bool RevealListViewItemPresenter::GoToElementStateCore(winrt::hstring const& state, bool useTransitions) 
{
    RevealBrush::EnsureProperties();

    const auto goToState = [this](winrt::RevealBrushState state) {
        SetValue(RevealBrush::s_StateProperty, box_value(state));
    };

    // Handle all of the states in the ListViewItemPresenter "Common" state group.
    if (state == L"Normal" || state == L"Selected" || state == L"Disabled")
    {
        goToState(winrt::RevealBrushState::Normal);
    }
    else if (state == L"PointerOver" || state == L"PointerOverSelected")
    {
        goToState(winrt::RevealBrushState::PointerOver);
    }
    else if (state == L"Pressed" || state == L"PointerOverPressed" || state == L"PressedSelected")
    {
        goToState(winrt::RevealBrushState::Pressed);
    }

    return __super::GoToElementStateCore(state, useTransitions);
}
