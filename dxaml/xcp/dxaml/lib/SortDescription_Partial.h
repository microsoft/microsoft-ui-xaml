// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SortDescription.g.h"

namespace DirectUI
{
    // WPF's SortDescription is a struct that a SortDescriptionCollection seals; this one is sealed by the first
    // SortDescriptions collection that holds it, and a change after that throws.
    PARTIAL_CLASS(SortDescription)
    {
    public:
        _Check_return_ HRESULT get_PropertyNameImpl(_Out_ HSTRING* pValue);
        _Check_return_ HRESULT put_PropertyNameImpl(_In_opt_ HSTRING value);
        _Check_return_ HRESULT get_DirectionImpl(_Out_ xaml_data::CollectionViewSortDirection* pValue);
        _Check_return_ HRESULT put_DirectionImpl(xaml_data::CollectionViewSortDirection value);

        void Seal()
        {
            m_isSealed = true;
        }

    private:
        _Check_return_ HRESULT CheckNotSealed();

        bool m_isSealed = false;
    };
}
