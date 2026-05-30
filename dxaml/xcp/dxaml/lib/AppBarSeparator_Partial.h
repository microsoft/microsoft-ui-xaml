// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AppBarSeparator.g.h"

namespace DirectUI
{
    // Represents the AppBarSeparator control
    PARTIAL_CLASS(AppBarSeparator)
    {
    public:
        // ICommanBarElement2 implementation
        _Check_return_ HRESULT get_IsInOverflowImpl(_Out_ BOOLEAN* pValue);

    protected:
        IFACEMETHOD(OnApplyTemplate)() override;
            
        // Updates our visual state when the value of our IsCompact property changes 
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;
                
        _Check_return_ HRESULT ChangeVisualState(_In_ bool useTransitions = true) override;

        _Check_return_ HRESULT OnVisibilityChanged() override;
    };
}
