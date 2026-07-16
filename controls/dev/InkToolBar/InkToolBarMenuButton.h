// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolBarMenuButton.g.h"
#include "InkToolBarMenuButton.properties.h"

class InkToolBarMenuButton :
    public ReferenceTracker<InkToolBarMenuButton, winrt::implementation::InkToolBarMenuButtonT, winrt::composable>, 
    public InkToolBarMenuButtonProperties
{
public:

    winrt::InkToolBarMenuKind MenuKind() { return m_menuKind; }
    
    bool IsExtensionGlyphShown() { return m_isExtensionGlyphShown; }
    void IsExtensionGlyphShown(bool value) { m_isExtensionGlyphShown = value; }

protected:
    void SetMenuKind(winrt::InkToolBarMenuKind value) { m_menuKind = value; }

private:
    winrt::InkToolBarMenuKind m_menuKind{ winrt::InkToolBarMenuKind::Stencil };
    bool m_isExtensionGlyphShown{ true };
};

