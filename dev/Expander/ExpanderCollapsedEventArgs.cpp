// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "ExpanderCollapsedEventArgs.h"

ExpanderCollapsedEventArgs::ExpanderCollapsedEventArgs(winrt::Expander const& expander)
{
    m_expander.set(expander);
}

winrt::IInspectable ExpanderCollapsedEventArgs::CollapsedContent()
{
    return m_collapsedContainer.get();
}

void ExpanderCollapsedEventArgs::CollapsedContent(winrt::IInspectable const& value)
{
   m_collapsedContainer.set(value);
}
