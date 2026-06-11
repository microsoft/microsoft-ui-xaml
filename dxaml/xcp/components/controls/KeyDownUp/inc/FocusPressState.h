// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CUIElement;

namespace KeyPress {

    void StartFocusPress(_In_ CUIElement* const focused);
    void EndFocusPress(_In_ CUIElement* const focused);
}