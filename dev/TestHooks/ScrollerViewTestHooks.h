// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollerView.h"
#include "ScrollerViewTestHooks.g.h"

class ScrollerViewTestHooks :
    public winrt::implementation::ScrollerViewTestHooksT<ScrollerViewTestHooks>
{
public:
    static winrt::Scroller GetScrollerPart(const winrt::ScrollerView& scrollerView);
};