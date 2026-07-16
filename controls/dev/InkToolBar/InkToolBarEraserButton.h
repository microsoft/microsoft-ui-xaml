// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarEraserButton.g.h"
#include "InkToolBarEraserButton.properties.h"

#include "InkToolBarToolButton.h"

class InkToolBarEraserButton :
    public winrt::implementation::InkToolBarEraserButtonT<InkToolBarEraserButton, InkToolBarToolButton>, 
    public InkToolBarEraserButtonProperties
{
public:
    ForwardRefToBaseReferenceTracker(InkToolBarToolButton)

    InkToolBarEraserButton()
    {
        SetToolKind(winrt::InkToolBarTool::Eraser);
    }

    // These functions are ambiguous with InkToolBarToolButton, disambiguate
    using InkToolBarEraserButtonProperties::EnsureProperties;
    using InkToolBarEraserButtonProperties::ClearProperties;

    winrt::InkToolBarEraserKind SelectedEraser() { return m_selectedEraser; }
    void SelectedEraser(winrt::InkToolBarEraserKind value) { m_selectedEraser = value; }
    
    bool IsClearAllVisible() { return m_isClearAllVisible; }
    void IsClearAllVisible(bool value) { m_isClearAllVisible = value; }
    
    bool IsStrokeEraserVisible() { return m_isStrokeEraserVisible; }
    void IsStrokeEraserVisible(bool value) { m_isStrokeEraserVisible = value; }
    
    bool ArePrecisionErasersVisible() { return m_arePrecisionErasersVisible; }
    void ArePrecisionErasersVisible(bool value) { m_arePrecisionErasersVisible = value; }

private:
    winrt::InkToolBarEraserKind m_selectedEraser{ winrt::InkToolBarEraserKind::Stroke };
    bool m_isClearAllVisible{ true };
    bool m_isStrokeEraserVisible{ true };
    bool m_arePrecisionErasersVisible{ true };
};

