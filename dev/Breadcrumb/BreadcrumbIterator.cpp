// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

#include "BreadcrumbIterator.h"

BreadcrumbIterator::BreadcrumbIterator(const winrt::IInspectable& itemsSource)
{
    m_currentIndex = 0;

    if (itemsSource)
    {
        m_breadcrumbItemsSourceView = winrt::ItemsSourceView(itemsSource);

        // Add 1 to account for the leading null/ellipsis element
        m_size = static_cast<uint32_t>(m_breadcrumbItemsSourceView.Count() + 1);
    }
    else
    {
        m_size = 1;
    }
}

winrt::IInspectable BreadcrumbIterator::Current()
{
    if (m_currentIndex == 0)
    {
        return nullptr;
    }
    else if (HasCurrent())
    {
        return m_breadcrumbItemsSourceView.GetAt(m_currentIndex - 1);
    }
    else
    {
        throw winrt::hresult_out_of_bounds();
    }
}

bool BreadcrumbIterator::HasCurrent()
{
    return (m_currentIndex < m_size);
}

uint32_t BreadcrumbIterator::GetMany(winrt::array_view<winrt::IInspectable> items)
{
    uint32_t howMany{};
    if (HasCurrent())
    {
        do
        {
            if (howMany >= items.size()) break;

            items[howMany] = Current();
            howMany++;
        } while (MoveNext());
    }

    return howMany;
}

bool BreadcrumbIterator::MoveNext()
{
    if (HasCurrent())
    {
        ++m_currentIndex;
        return HasCurrent();
    }
    else
    {
        throw winrt::hresult_out_of_bounds();
    }

    return false;
}
