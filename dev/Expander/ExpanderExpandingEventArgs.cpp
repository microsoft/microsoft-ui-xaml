// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "ExpanderExpandingEventArgs.h"

ExpanderExpandingEventArgs::ExpanderExpandingEventArgs(winrt::Expander const& expander)
{
    m_expander.set(expander);
}

winrt::IInspectable ExpanderExpandingEventArgs::ExpandingContent()
{
    return m_expandingContent.get();
}

void ExpanderExpandingEventArgs::ExpandingContent(winrt::IInspectable const& value)
{
    m_expandingContent.set(value);
}
