// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "TypeLogging.h"
#include "ScrollPresenterTypeLogging.h"
#include "ViewChange.h"

ViewChange::ViewChange(
    ScrollPresenterViewKind viewKind,
    winrt::IInspectable const& options)
    : m_viewKind(viewKind)
    , m_options(options)
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH_PTR_STR, METH_NAME, this,
        options,
        TypeLogging::ScrollPresenterViewKindToString(viewKind).c_str());
}

ViewChange::~ViewChange()
{
    SCROLLPRESENTER_TRACE_VERBOSE(nullptr, TRACE_MSG_METH, METH_NAME, this);
}
