// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarToolButton.g.h"
#include "InkToolBarToolButton.properties.h"

class InkToolBarToolButton :
    public ReferenceTracker<InkToolBarToolButton, winrt::implementation::InkToolBarToolButtonT, winrt::composable>, 
    public InkToolBarToolButtonProperties
{
public:
    InkToolBarToolButton() = default;

    winrt::InkToolBarTool ToolKind() { return m_toolKind; }
    
    bool IsExtensionGlyphShown() { return m_isExtensionGlyphShown; }
    void IsExtensionGlyphShown(bool value) { m_isExtensionGlyphShown = value; }

    // Echo the tool's toggle association. Custom is the safe default for non-toggle tools.
    winrt::InkToolBarToggle ToggleKind() { return m_toggleKind; }
    void ToggleKind(winrt::InkToolBarToggle value) { m_toggleKind = value; }

protected:
    void SetToolKind(winrt::InkToolBarTool kind) { m_toolKind = kind; }

private:
    winrt::InkToolBarTool m_toolKind{ winrt::InkToolBarTool::BallpointPen };
    bool m_isExtensionGlyphShown{ false };
    winrt::InkToolBarToggle m_toggleKind{ winrt::InkToolBarToggle::Custom };
};

