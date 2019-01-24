// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ScrollViewer.h"
#include "ScrollViewerTestHooks.g.h"

class ScrollViewerTestHooks :
    public winrt::implementation::ScrollViewerTestHooksT<ScrollViewerTestHooks>
{
public:
    static winrt::Scroller GetScrollerPart(const winrt::ScrollViewer& scrollViewer);
};