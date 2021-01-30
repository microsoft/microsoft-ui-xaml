// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbItemClickedEventArgs.h"

winrt::IInspectable BreadcrumbItemClickedEventArgs::Item()
{
    return m_item;
}

void BreadcrumbItemClickedEventArgs::Item(const winrt::IInspectable& item)
{
    m_item = item;
}
