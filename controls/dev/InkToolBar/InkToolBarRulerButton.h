// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarRulerButton.g.h"

#include "InkToolBarToggleButton.h"

// Dedicated ruler toggle button (WUXC parity: Windows.UI.Xaml.Controls.InkToolBarRulerButton).
// Structurally an InkToolBarToggleButton whose ToggleKind is Ruler, so GetToggleButton(Ruler)
// returns a strongly-typed InkToolBarRulerButton and apps can add one to InkToolBar.Children.
class InkToolBarRulerButton :
    public winrt::implementation::InkToolBarRulerButtonT<InkToolBarRulerButton, InkToolBarToggleButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarToggleButton)

    InkToolBarRulerButton()
    {
        SetToggleKind(winrt::InkToolBarToggle::Ruler);
    }
};
