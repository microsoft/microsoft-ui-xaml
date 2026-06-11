// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SelectorBar.h"
#include "SelectorBarTestHooks.g.h"

class SelectorBarTestHooks :
    public winrt::implementation::SelectorBarTestHooksT<SelectorBarTestHooks>
{
public:
    static winrt::ItemsView GetItemsViewPart(const winrt::SelectorBar& selectorBar);
};
