// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "AnnotatedScrollBarDetailLabelRequestedEventArgs.h"

AnnotatedScrollBarDetailLabelRequestedEventArgs::AnnotatedScrollBarDetailLabelRequestedEventArgs(double scrollOffset)
{
    m_scrollOffset = scrollOffset;
}

#pragma region IAnnotatedScrollBarDetailLabelRequestedEventArgs

winrt::IInspectable AnnotatedScrollBarDetailLabelRequestedEventArgs::Content()
{
    return m_content.get();
}

void AnnotatedScrollBarDetailLabelRequestedEventArgs::Content(const winrt::IInspectable& content)
{
    m_content.set(content);
}

double AnnotatedScrollBarDetailLabelRequestedEventArgs::ScrollOffset()
{
    return m_scrollOffset;
}

#pragma endregion
