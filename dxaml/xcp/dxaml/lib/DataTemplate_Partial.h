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

        // Stores the app-provided callback used by this DataTemplate to create the element tree.
        // Called once at construction with a non-null factory (see CreateInstanceFromCallbackImpl).
        _Check_return_ HRESULT SetElementFactory(_In_ xaml::IDataTemplateElementFactory* value);

        // Invoked (via FxCallbacks) from CDataTemplate::LoadContent to build the element tree by
        // calling the app-provided callback. Returns a context-addref'd core element (caller releases),
        // matching the parsed-content LoadContent behavior.
        static _Check_return_ HRESULT CreateElementFromFactory(
            _In_ CDependencyObject* nativeTemplate,
            _Outptr_result_maybenull_ CDependencyObject** ppResult);

    private:
        TrackerPtr<xaml::IDataTemplateElementFactory> m_elementFactory;
        TrackerPtr<TrackerCollection<xaml::UIElement*>> m_elements;
        std::vector<ctl::WeakRefPtr> m_parents;
    };
}
