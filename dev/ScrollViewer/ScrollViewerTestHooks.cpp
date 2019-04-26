// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ScrollViewerTestHooksFactory.h"

winrt::Scroller ScrollViewerTestHooks::GetScrollerPart(const winrt::ScrollViewer& scrollViewer)
{
    if (scrollViewer)
    {
        return winrt::get_self<ScrollViewer>(scrollViewer)->GetScrollerPart();
    }

    return nullptr;
}
