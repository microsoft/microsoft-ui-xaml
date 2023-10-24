// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs.h"

AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs::AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs(double value)
{
    m_value = value;
}

#pragma region IAnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs


double AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs::Value()
{
    return m_value;
}

double AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs::ScrollOffset()
{
    return m_scrollOffset;
}

void AnnotatedScrollBarScrollOffsetFromValueRequestedEventArgs::ScrollOffset(double scrollOffset)
{
    m_scrollOffset = scrollOffset;
}

#pragma endregion
