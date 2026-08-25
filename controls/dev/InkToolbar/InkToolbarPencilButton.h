// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarPencilButton.g.h"

#include "InkToolbarPenButton.h"
#include "ResourceAccessor.h"

class InkToolbarPencilButton :
    public winrt::implementation::InkToolbarPencilButtonT<InkToolbarPencilButton, InkToolbarPenButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarPenButton)

    InkToolbarPencilButton()
    {
        SetToolKind(winrt::InkToolbarTool::Pencil);
        SetDefaultStyleKey(this);
    }

    winrt::hstring GetLocalizedToolName() override
    {
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarPencilButtonName);
    }

    // UWP InkToolbarPencilButton::CreateInkDrawingAttributes: pencil-specific attributes.
    winrt::InkDrawingAttributes CreateInkDrawingAttributes() override
    {
        auto attrs = winrt::InkDrawingAttributes::CreateForPencil();
        if (auto solid = SelectedBrush().try_as<winrt::SolidColorBrush>())
        {
            attrs.Color(solid.Color());
        }
        auto self = this->try_as<winrt::InkToolbarPenButton>();
        float w = InkToolbarPenButton::DetermineStrokeWidth(self);
        attrs.Size({ w, w });
        return attrs;
    }
};

