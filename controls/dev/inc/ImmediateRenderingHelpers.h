// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Immediate rendering backends (WINUI_RENDERING_BACKEND=gdiplus|d2d-hw|d2d-sw) paint the retained XAML tree without a
// Composition visual tree. Composition animations started on hand-off visuals or UIElement facades are never seen by
// those backends (and force the window back to Composition), so controls that drive their content with
// InteractionTracker / ExpressionAnimations compute the same values on the UI thread instead and set them as XAML
// properties. See docs/design-notes/immediate-rendering-backends-proposal.md, "Full XAML coverage: scrolling".
namespace ImmediateRendering
{
    inline bool IsRequested()
    {
        static const bool requested = []()
        {
            wchar_t value[32]{};
            const DWORD length = GetEnvironmentVariableW(L"WINUI_RENDERING_BACKEND", value, ARRAYSIZE(value));
            return length > 0 && length < ARRAYSIZE(value) && _wcsicmp(value, L"composition") != 0;
        }();
        return requested;
    }
}
