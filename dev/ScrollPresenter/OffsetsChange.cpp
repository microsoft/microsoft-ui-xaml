// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTrace.h"
#include "OffsetsChange.h"

OffsetsChange::OffsetsChange(
    double zoomedHorizontalOffset,
    double zoomedVerticalOffset,
    ScrollPresenterViewKind offsetsKind,
    winrt::IInspectable const& options) :
        m_zoomedHorizontalOffset(zoomedHorizontalOffset),
        m_zoomedVerticalOffset(zoomedVerticalOffset),
        ViewChange(offsetsKind, options)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL_DBL, METH_NAME, this,
        zoomedHorizontalOffset, zoomedVerticalOffset);
}

OffsetsChange::~OffsetsChange()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}

void OffsetsChange::ZoomedHorizontalOffset(double zoomedHorizontalOffset)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, zoomedHorizontalOffset);

    m_zoomedHorizontalOffset = zoomedHorizontalOffset;
}

void OffsetsChange::ZoomedVerticalOffset(double zoomedVerticalOffset)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_DBL, METH_NAME, this, zoomedVerticalOffset);

    m_zoomedVerticalOffset = zoomedVerticalOffset;
}
