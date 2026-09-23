// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarBallpointPenButton.g.h"

#include "InkToolbarPenButton.h"
#include "ResourceAccessor.h"

class InkToolbarBallpointPenButton :
    public winrt::implementation::InkToolbarBallpointPenButtonT<InkToolbarBallpointPenButton, InkToolbarPenButton>
{
public:
    ForwardRefToBaseReferenceTracker(InkToolbarPenButton)

    InkToolbarBallpointPenButton()
    {
        SetToolKind(winrt::InkToolbarTool::BallpointPen);
        SetDefaultStyleKey(this);
    }

    winrt::hstring GetLocalizedToolName() override
    {
        return ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarBallpointPenButtonName);
    }

    // UWP InkToolbarBallpointPenButton::CreateInkDrawingAttributes: solid color, circle tip.
    winrt::InkDrawingAttributes CreateInkDrawingAttributes() override
    {
        auto attrs = winrt::InkDrawingAttributes();
        if (auto solid = SelectedBrush().try_as<winrt::SolidColorBrush>())
        {
            attrs.Color(solid.Color());
        }
        auto self = this->try_as<winrt::InkToolbarPenButton>();
        float w = InkToolbarPenButton::DetermineStrokeWidth(self);
        attrs.Size({ w, w });
        attrs.PenTip(winrt::Windows::UI::Input::Inking::PenTipShape::Circle);
        return attrs;
    }
};

