// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ItemsSourceView.g.h"

class ItemsSourceView :
    public ReferenceTracker<ItemsSourceView, winrt::implementation::ItemsSourceViewT, winrt::composing>
{
public:

#pragma region IDataSource
    int32_t Count();
    winrt::IInspectable GetAt(int index);

    bool HasKeyIndexMapping();
    winrt::hstring KeyFromIndex(int index);
    int IndexFromKey(winrt::hstring const& id);

    winrt::event_token CollectionChanged(winrt::NotifyCollectionChangedEventHandler const& value);
    void CollectionChanged(winrt::event_token const& token);
#pragma endregion

    int IndexOf(winrt::IInspectable const& value);

#pragma region Consume API for internal use only.
    void OnItemsSourceChanged(winrt::NotifyCollectionChangedEventArgs const& args);

    virtual int32_t GetSizeCore();
    virtual winrt::IInspectable GetAtCore(int index);

    virtual bool HasKeyIndexMappingCore();
    virtual winrt::hstring KeyFromIndexCore(int index);
    virtual int IndexFromKeyCore(winrt::hstring const& id);
    virtual int IndexOfCore(winrt::IInspectable const& value);
#pragma endregion

private:
    event_source<winrt::NotifyCollectionChangedEventHandler> m_collectionChangedEventSource{ this };
    int m_cachedSize{ -1 };
};
