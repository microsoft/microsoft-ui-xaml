// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// _ONECORETRANSFORMS_REMOVED_
// In the past we had a special mode called "OneCoreTransforms" to help us support
// Windows 10x.  We expect it to come back in some form when we support 10x gain.
// Please see /design-notes/OneCoreTransforms.md for more information.

#include "precomp.h"

#include "XamlOneCoreTransforms.h"

void XamlOneCoreTransforms::EnsureInitialized(InitMode)
{
}

bool XamlOneCoreTransforms::IsEnabled()
{
    return false;
}

void XamlOneCoreTransforms::FailFastIfEnabled()
{
}

