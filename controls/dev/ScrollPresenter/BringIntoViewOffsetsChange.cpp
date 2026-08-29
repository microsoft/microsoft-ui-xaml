// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTrace.h"
#include "BringIntoViewOffsetsChange.h"

BringIntoViewOffsetsChange::BringIntoViewOffsetsChange(
    const ITrackerHandleManager* owner,
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    ScrollPresenterViewKind offsetsKind,
    winrt::IInspectable const& options,
    winrt::UIElement const& element,
    winrt::Rect const& elementRect,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double horizontalOffset,
    double verticalOffset) :
        m_owner(owner),
        m_elementRect(elementRect),
        m_horizontalAlignmentRatio(horizontalAlignmentRatio),
        m_verticalAlignmentRatio(verticalAlignmentRatio),
        m_horizontalOffset(horizontalOffset),
        m_verticalOffset(verticalOffset),
        OffsetsChange(zoomedHorizontalOffset, zoomedVerticalOffset, offsetsKind, options)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);

    m_element.set(element);
}

BringIntoViewOffsetsChange::~BringIntoViewOffsetsChange()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
