// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GroupStyleSelector.g.h"

namespace DirectUI
{
    // GroupStyleSelector allows the app writer to provide custom group style selection logic.
    PARTIAL_CLASS(GroupStyleSelector)
    {
    public:
        // Override this method to return an app specific GroupStyle.
        // Returns an app-specific style to apply, or null.
        _Check_return_ HRESULT SelectGroupStyleCoreImpl(_In_opt_ IInspectable* group, _In_ UINT level, _Outptr_ xaml_controls::IGroupStyle** returnValue)
        {
            *returnValue = NULL;
            return S_OK;
        }
    };
}
