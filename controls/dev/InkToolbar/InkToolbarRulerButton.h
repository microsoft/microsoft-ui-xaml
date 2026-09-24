// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarRulerButton.g.h"

#include "InkToolbarToggleButton.h"

// Dedicated ruler toggle button (WUXC parity: Windows.UI.Xaml.Controls.InkToolbarRulerButton).
// Structurally an InkToolbarToggleButton whose ToggleKind is Ruler, so GetToggleButton(Ruler)
// returns a strongly-typed InkToolbarRulerButton and apps can add one to InkToolbar.Children.
class InkToolbarRulerButton :
    public winrt::implementation::InkToolbarRulerButtonT<InkToolbarRulerButton, InkToolbarToggleButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarToggleButton)

    InkToolbarRulerButton()
    {
        SetToggleKind(winrt::InkToolbarToggle::Ruler);
        SetDefaultStyleKey(this);
    }
};
