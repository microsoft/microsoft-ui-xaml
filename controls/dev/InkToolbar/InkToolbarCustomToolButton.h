// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarCustomToolButton.g.h"
#include "InkToolbarCustomToolButton.properties.h"

#include "InkToolbarToolButton.h"

class InkToolbarCustomToolButton :
    public winrt::implementation::InkToolbarCustomToolButtonT<InkToolbarCustomToolButton, InkToolbarToolButton>, 
    public InkToolbarCustomToolButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarToolButton)

    InkToolbarCustomToolButton()
    {
        SetToolKind(winrt::InkToolbarTool::CustomTool);
        SetDefaultStyleKey(this);
    }

    // These functions are ambiguous with InkToolbarToolButton, disambiguate
    using InkToolbarCustomToolButtonProperties::EnsureProperties;
    using InkToolbarCustomToolButtonProperties::ClearProperties;
};

