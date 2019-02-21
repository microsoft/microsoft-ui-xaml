// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "OffsetsChange.h"

OffsetsChange::OffsetsChange(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    ScrollerViewKind offsetsKind,
    winrt::ScrollOptions const& options) :
        m_zoomedHorizontalOffset(zoomedHorizontalOffset),
        m_zoomedVerticalOffset(zoomedVerticalOffset),
        ViewChange(offsetsKind, options)
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL_DBL, METH_NAME, this,
        zoomedHorizontalOffset, zoomedVerticalOffset);
}

OffsetsChange::~OffsetsChange()
{
    SCROLLER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
