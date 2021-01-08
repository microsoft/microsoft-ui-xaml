// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "Vector.h"

class BreadcrumbIterable :
    public ReferenceTracker<BreadcrumbIterable, reference_tracker_implements_t<winrt::IIterable<winrt::IInspectable>>::type>
{
public:
    BreadcrumbIterable();
    void ItemsSource(const winrt::IInspectable& itemsSource);
    winrt::IIterator<winrt::IInspectable> First();

private:
    tracker_ref<winrt::IInspectable> m_itemsSource{ this };
};
