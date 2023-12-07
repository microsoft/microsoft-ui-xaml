// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// This is a class to help move XAML to OneCoreTransforms mode in stages.
// We are iteratively moving the platform (in XAML and also other components) toward
// APIs that are in "visual-relative" coordinates (not screen coordinates) and away
// from desktop-specific constructs like HWNDs.
#pragma once

// _ONECORETRANSFORMS_REMOVED_
// In the past we had a special mode called "OneCoreTransforms" to help us support
// Windows 10x.  We expect it to come back in some form when we support 10x gain.
// Please see /design-notes/OneCoreTransforms.md for more information.
class XamlOneCoreTransforms
{
public:

    enum class InitMode
    {
        Normal,
        ForceDisable
    };

    static void EnsureInitialized(InitMode mode);
    static bool IsEnabled();
    static void FailFastIfEnabled();

private:
        
    static bool s_enabled;
    static bool s_initialized;
};
