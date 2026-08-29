// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "StyleSelector.g.h"

namespace DirectUI
{
    // StyleSelector allows the app writer to provide custom style selection logic.
    // For example, with a class Bug as the Content,
    // use a particular style for Pri1 bugs and a different style for Pri2 bugs.
    // An application writer can override the SelectStyle method in a derived
    // selector class and assign an instance of this class to the StyleSelector property on
    // ContentPresenter class.
    PARTIAL_CLASS(StyleSelector)
    {
    public:
        // Override this method to return an app specific Style.
        // Returns an app-specific style to apply, or null.
        _Check_return_ HRESULT SelectStyleCoreImpl(_In_ IInspectable* item, _In_ xaml::IDependencyObject* container, _Outptr_ xaml::IStyle** returnValue)
        {
            *returnValue = NULL;
            RRETURN(S_OK);
        }
    };
}
