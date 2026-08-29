// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "FocusPressState.h"

#include "ccontrol.h"
#include "corep.h"
#include "focusmgr.h"

namespace KeyPress {

    void StartFocusPress(_In_ CUIElement* const focused)
    {
        VisualTree::GetFocusManagerForElement(focused)->OnFocusedElementKeyPressed();
    }

    void EndFocusPress(_In_ CUIElement* const focused)
    {
        VisualTree::GetFocusManagerForElement(focused)->OnFocusedElementKeyReleased();
    }
}