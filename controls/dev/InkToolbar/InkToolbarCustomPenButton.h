// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarCustomPenButton.g.h"
#include "InkToolbarCustomPenButton.properties.h"

#include "InkToolbarPenButton.h"

class InkToolbarCustomPenButton :
    public winrt::implementation::InkToolbarCustomPenButtonT<InkToolbarCustomPenButton, InkToolbarPenButton>, 
    public InkToolbarCustomPenButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarPenButton)

    InkToolbarCustomPenButton()
    {
        SetToolKind(winrt::InkToolbarTool::CustomPen);
        SetDefaultStyleKey(this);
    }

    // These functions are ambiguous with InkToolbarPenButton, disambiguate
    using InkToolbarCustomPenButtonProperties::EnsureProperties;
    using InkToolbarCustomPenButtonProperties::ClearProperties;

    // UWP InkToolbarCustomPenButton::CreateInkDrawingAttributes: delegate to the attached custom pen,
    // passing the currently selected brush and the resolved stroke width.
    winrt::InkDrawingAttributes CreateInkDrawingAttributes() override
    {
        if (auto customPen = CustomPen())
        {
            auto self = this->try_as<winrt::InkToolbarPenButton>();
            float w = InkToolbarPenButton::DetermineStrokeWidth(self);
            return customPen.CreateInkDrawingAttributes(SelectedBrush(), w);
        }
        return nullptr;
    }
};

