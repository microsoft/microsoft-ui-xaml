// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataTemplate.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(DataTemplate)
    {
    public:
        _Check_return_  HRESULT LoadContentImpl(_Outptr_ xaml::IDependencyObject** returnValue);

        _Check_return_ HRESULT GetElementImpl(_In_ xaml::IElementFactoryGetArgs* args, _Outptr_ xaml::IUIElement** ppResult);
        _Check_return_ HRESULT RecycleElementImpl(_In_ xaml::IElementFactoryRecycleArgs* args);

    private:
        TrackerPtr<TrackerCollection<xaml::UIElement*>> m_elements;
        std::vector<ctl::WeakRefPtr> m_parents;
    };
}
