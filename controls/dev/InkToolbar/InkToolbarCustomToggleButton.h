// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarCustomToggleButton.g.h"

#include "InkToolbarToggleButton.h"

class InkToolbarCustomToggleButton :
    public winrt::implementation::InkToolbarCustomToggleButtonT<InkToolbarCustomToggleButton, InkToolbarToggleButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarToggleButton)

    InkToolbarCustomToggleButton()
    {
        SetToggleKind(winrt::InkToolbarToggle::Custom);
        SetDefaultStyleKey(this);
    }
};

