// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VisualStateGroup.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(VisualStateGroup)
    {
    public:
        _Check_return_ HRESULT get_NameImpl(_Out_ HSTRING* pValue);

        _Check_return_ HRESULT get_CurrentStateImpl(_Outptr_ xaml::IVisualState** pValue);

        _Check_return_ HRESULT GetState(
            _In_ HSTRING hStateName,
            _Outptr_ xaml::IVisualState** ppState);

        _Check_return_ HRESULT RaiseCurrentStateChanging(
            _In_ xaml::IFrameworkElement* pElement,
            _In_ xaml::IVisualState* pOldState,
            _In_ xaml::IVisualState* pNewState,
            _In_ xaml_controls::IControl* pControl);

        _Check_return_ HRESULT RaiseCurrentStateChanged(
            _In_ xaml::IFrameworkElement* pElement,
            _In_ xaml::IVisualState* pOldState,
            _In_ xaml::IVisualState* pNewState,
            _In_ xaml_controls::IControl* pControl);
    };
}
