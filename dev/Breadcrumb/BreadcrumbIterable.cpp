// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbIterable.h"
#include "BreadcrumbIterator.h"

BreadcrumbIterable::BreadcrumbIterable()
{
}

void BreadcrumbIterable::ItemsSource(const winrt::IInspectable& itemsSource)
{
    m_itemsSource.set(itemsSource);
}

winrt::IIterator<winrt::IInspectable> BreadcrumbIterable::First()
{
    return winrt::make<BreadcrumbIterator>(m_itemsSource.get());
}
