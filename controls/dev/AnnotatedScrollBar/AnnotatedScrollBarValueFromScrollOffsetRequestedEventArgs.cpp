// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs.h"

AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs::AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs(double scrollOffset)
{
    m_scrollOffset = scrollOffset;
}

#pragma region IAnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs


double AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs::Value()
{
    return m_value;
}

void AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs::Value(double value)
{
    m_value = value;
}

double AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs::ScrollOffset()
{
    return m_scrollOffset;
}

#pragma endregion
