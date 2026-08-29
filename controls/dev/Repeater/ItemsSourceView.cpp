// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "ItemsSourceView.h"
#include "InspectingDataSource.h"

#pragma region IDataSource

int32_t ItemsSourceView::Count()
{
    if (m_cachedSize == -1)
    {
        // Call the override the very first time. After this,
        // we can just update the size when there is a data source change.
        m_cachedSize = GetSizeCore();
    }

    return m_cachedSize;
}

winrt::IInspectable ItemsSourceView::GetAt(int index)
{
    return GetAtCore(index);
}

bool ItemsSourceView::HasKeyIndexMapping()
{
    return HasKeyIndexMappingCore();
}

winrt::hstring ItemsSourceView::KeyFromIndex(int index)
{
    return KeyFromIndexCore(index);
}

int ItemsSourceView::IndexFromKey(winrt::hstring const& id)
{
    return IndexFromKeyCore(id);
}

int ItemsSourceView::IndexOf(winrt::IInspectable const& value)
{
    return IndexOfCore(value);
}

winrt::event_token ItemsSourceView::CollectionChanged(winrt::NotifyCollectionChangedEventHandler const& value)
{
    return m_collectionChangedEventSource.add(value);
}

void ItemsSourceView::CollectionChanged(winrt::event_token const& token)
{
    m_collectionChangedEventSource.remove(token);
}

#pragma endregion

#pragma region IDataSourceProtected

void ItemsSourceView::OnItemsSourceChanged(winrt::NotifyCollectionChangedEventArgs const& args)
{
    m_cachedSize = GetSizeCore();
    m_collectionChangedEventSource(*this, args);
}

#pragma endregion

#pragma region IDataSourceOverrides

int32_t ItemsSourceView::GetSizeCore()
{
    throw winrt::hresult_not_implemented();
}

winrt::IInspectable ItemsSourceView::GetAtCore(int /* index */)
{
    throw winrt::hresult_not_implemented();
}

bool ItemsSourceView::HasKeyIndexMappingCore()
{
    throw winrt::hresult_not_implemented();
}

winrt::hstring ItemsSourceView::KeyFromIndexCore(int /* index */)
{
    throw winrt::hresult_not_implemented();
}

int ItemsSourceView::IndexFromKeyCore(winrt::hstring const& /* id */)
{
    throw winrt::hresult_not_implemented();
}

#pragma endregion

int ItemsSourceView::IndexOfCore(winrt::IInspectable const& value)
{
    throw winrt::hresult_not_implemented();
}