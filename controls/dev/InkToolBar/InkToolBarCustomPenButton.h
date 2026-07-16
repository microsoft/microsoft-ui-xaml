// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarCustomPenButton.g.h"
#include "InkToolBarCustomPenButton.properties.h"

#include "InkToolBarPenButton.h"

class InkToolBarCustomPenButton :
    public winrt::implementation::InkToolBarCustomPenButtonT<InkToolBarCustomPenButton, InkToolBarPenButton>, 
    public InkToolBarCustomPenButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarPenButton)

    InkToolBarCustomPenButton()
    {
        SetToolKind(winrt::InkToolBarTool::CustomPen);
    }

    // These functions are ambiguous with InkToolBarPenButton, disambiguate
    using InkToolBarCustomPenButtonProperties::EnsureProperties;
    using InkToolBarCustomPenButtonProperties::ClearProperties;

    winrt::InkToolBarCustomPen CustomPen() { return m_customPen; }
    void CustomPen(winrt::InkToolBarCustomPen value) { m_customPen = value; }
    
    winrt::UIElement ConfigurationContent() { return m_configurationContent; }
    void ConfigurationContent(winrt::UIElement value) { m_configurationContent = value; }

private:
    winrt::InkToolBarCustomPen m_customPen{ nullptr };
    winrt::UIElement m_configurationContent{ nullptr };
};

