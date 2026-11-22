// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PropertyGroupDescription.g.h"

namespace DirectUI
{
    // WPF's PropertyGroupDescription. The view reads PropertyName, Converter and StringComparison directly instead of
    // calling the overridables; a change to any of them regroups the views that use it.
    PARTIAL_CLASS(PropertyGroupDescription)
    {
    protected:
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    public:
        _Check_return_ HRESULT GroupNameFromItemImpl(_In_opt_ IInspectable* pItem, INT level, _In_opt_ HSTRING culture, _Outptr_result_maybenull_ IInspectable** ppResult) override;
        _Check_return_ HRESULT NamesMatchImpl(_In_opt_ IInspectable* pGroupName, _In_opt_ IInspectable* pItemName, _Out_ BOOLEAN* pResult) override;
    };
}
