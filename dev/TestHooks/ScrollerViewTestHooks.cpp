// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollerViewTestHooksFactory.h"

winrt::Scroller ScrollerViewTestHooks::GetScrollerPart(const winrt::ScrollerView& scrollerView)
{
    if (scrollerView)
    {
        return winrt::get_self<ScrollerView>(scrollerView)->GetScrollerPart();
    }

    return nullptr;
}
