// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h" 
#include "AnnotatedScrollBarLabel.h"

AnnotatedScrollBarLabel::AnnotatedScrollBarLabel(const winrt::IInspectable& content, double scrollOffset)
{
    m_content.set(content);
    m_scrollOffset = scrollOffset;
}

winrt::IInspectable AnnotatedScrollBarLabel::Content()
{
    return m_content.get();
}

double AnnotatedScrollBarLabel::ScrollOffset()
{
    return m_scrollOffset;
}