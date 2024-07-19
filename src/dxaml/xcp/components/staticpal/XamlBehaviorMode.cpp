// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <xamlbehaviormode.h>
#include "DesignMode.h"
#include "DesktopUtility.h"

// DEPRECATED
// IsXamlBehaviorEnabledForCurrentSku is how we alter XAML's behavior to fit the
// platform.  Since we're trying to converge XAML, try to avoid adding new calls
// to this function.
bool IsXamlBehaviorEnabledForCurrentSku(enum XamlBehavior behavior)
{
    return !DesktopUtility::IsOnDesktop();
}
