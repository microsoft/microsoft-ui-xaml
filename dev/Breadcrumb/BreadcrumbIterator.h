// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#pragma once

#include "pch.h"
#include "common.h"

#include "BreadcrumbIterable.h"


class BreadcrumbIterator :
    public winrt::implements<BreadcrumbIterator, winrt::IIterator<winrt::IInspectable>>
{
public:
    BreadcrumbIterator(const winrt::IInspectable& itemsSource);

    winrt::IInspectable Current();
    bool HasCurrent();
    uint32_t GetMany(winrt::array_view<winrt::IInspectable> items);
    bool MoveNext();

private:

    uint32_t m_currentIndex{};
    uint32_t m_size{};
    winrt::ItemsSourceView m_itemsRepeaterItemsSource{nullptr};
};
