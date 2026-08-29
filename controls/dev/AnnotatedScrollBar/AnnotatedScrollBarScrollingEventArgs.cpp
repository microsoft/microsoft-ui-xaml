// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBarScrollingEventArgs.h"

AnnotatedScrollBarScrollingEventArgs::AnnotatedScrollBarScrollingEventArgs(double scrollOffset, winrt::AnnotatedScrollBarScrollingEventKind scrollingEventKind)
{
    m_scrollOffset = scrollOffset;
    m_scrollingEventKind = scrollingEventKind;
}

#pragma region IAnnotatedScrollBarScrollingEventArgs

winrt::AnnotatedScrollBarScrollingEventKind AnnotatedScrollBarScrollingEventArgs::ScrollingEventKind()
{
    return m_scrollingEventKind;
}

double AnnotatedScrollBarScrollingEventArgs::ScrollOffset()
{
    return m_scrollOffset;
}

void AnnotatedScrollBarScrollingEventArgs::Cancel(bool cancel)
{
    m_cancel = cancel;
}

bool AnnotatedScrollBarScrollingEventArgs::Cancel()
{
    return m_cancel;
}

#pragma endregion
