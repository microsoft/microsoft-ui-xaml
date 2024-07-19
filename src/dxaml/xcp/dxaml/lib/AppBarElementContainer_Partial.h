// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarElementContainer.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(AppBarElementContainer)
    {
    public:
        // ICommandBarElement2 implementation
        _Check_return_ HRESULT get_IsInOverflowImpl(_Out_ BOOLEAN* pValue);

    protected:
        AppBarElementContainer();

        IFACEMETHOD(OnApplyTemplate)() override;
    };
}
