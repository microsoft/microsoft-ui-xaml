// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarToggleButton.g.h"

class InkToolBarToggleButton :
    public ReferenceTracker<InkToolBarToggleButton, winrt::implementation::InkToolBarToggleButtonT, winrt::composable>
{
public:

    winrt::InkToolBarToggle ToggleKind() { return m_toggleKind; }

    // Internal (not projected): lets the InkToolBar register a built-in toggle (e.g. the ruler)
    // with a specific ToggleKind so GetToggleButton(kind) can resolve it.
    void SetToggleKind(winrt::InkToolBarToggle value) { m_toggleKind = value; }

private:
    winrt::InkToolBarToggle m_toggleKind{ winrt::InkToolBarToggle::Custom };
};

