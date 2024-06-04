// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "DataTemplateSelector.g.h"

namespace DirectUI
{
    // DataTemplateSelector allows the app writer to provide custom template selection logic.
    // For example, with a class Bug as the Content,
    // use a particular template for Pri1 bugs and a different template for Pri2 bugs.
    // An application writer can override the SelectTemplate method in a derived
    // selector class and assign an instance of this class to the ContentTemplateSelector property on
    // ContentPresenter class.

    PARTIAL_CLASS(DataTemplateSelector)
    {

    public:
        // Override this method to return an app specific DataTemplate.
        // Returns an app-specific template to apply, or null.
        _Check_return_ HRESULT SelectTemplateCoreImpl(_In_opt_ IInspectable* item, _In_ xaml::IDependencyObject* container, _Outptr_ xaml::IDataTemplate** returnValue);

        // Override this method to return an app specific DataTemplate.
        // Returns an app-specific template to apply, or null.
        _Check_return_ HRESULT SelectTemplateForItemCoreImpl(_In_opt_ IInspectable* item, _Outptr_ xaml::IDataTemplate** returnValue);

        _Check_return_ HRESULT GetElementImpl(_In_ xaml::IElementFactoryGetArgs* args, _Outptr_ xaml::IUIElement** result);
        _Check_return_ HRESULT RecycleElementImpl(_In_ xaml::IElementFactoryRecycleArgs* args);
    };
}
