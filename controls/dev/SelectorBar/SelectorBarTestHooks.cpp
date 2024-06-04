// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SelectorBarTestHooksFactory.h"

winrt::ItemsView SelectorBarTestHooks::GetItemsViewPart(const winrt::SelectorBar& selectorBar)
{
    if (selectorBar)
    {
        return winrt::get_self<SelectorBar>(selectorBar)->GetItemsViewPart();
    }

    return nullptr;
}
