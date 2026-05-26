// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "BreadcrumbBarItemClickedEventArgs.h"

winrt::IInspectable BreadcrumbBarItemClickedEventArgs::Item()
{
    return m_item;
}

void BreadcrumbBarItemClickedEventArgs::Item(const winrt::IInspectable& item)
{
    m_item = item;
}

int BreadcrumbBarItemClickedEventArgs::Index()
{
    return m_index;
}

void BreadcrumbBarItemClickedEventArgs::Index(const int& index)
{
    m_index = index;
}
