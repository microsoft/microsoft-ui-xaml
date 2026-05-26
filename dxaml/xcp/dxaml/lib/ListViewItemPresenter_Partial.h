// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ListViewItemPresenter.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(ListViewItemPresenter)
    {

    public:
        ListViewItemPresenter()
        { }
        _Check_return_ HRESULT get_ListViewItemPresenterPaddingImpl(_Out_ xaml::Thickness* pValue);
        _Check_return_ HRESULT put_ListViewItemPresenterPaddingImpl(_In_ xaml::Thickness value);
        _Check_return_ HRESULT get_ListViewItemPresenterHorizontalContentAlignmentImpl(_Out_ xaml::HorizontalAlignment* pValue);
        _Check_return_ HRESULT put_ListViewItemPresenterHorizontalContentAlignmentImpl(_In_ xaml::HorizontalAlignment value);
        _Check_return_ HRESULT get_ListViewItemPresenterVerticalContentAlignmentImpl(_Out_ xaml::VerticalAlignment* pValue);
        _Check_return_ HRESULT put_ListViewItemPresenterVerticalContentAlignmentImpl(_In_ xaml::VerticalAlignment value);
        ~ListViewItemPresenter() override
        { }
        
    protected:
        _Check_return_ HRESULT GoToElementStateCoreImpl(
            _In_ HSTRING stateName,
            _In_ BOOLEAN useTransitions,
            _Out_ BOOLEAN* returnValue) override;

#ifdef DBG
    private:
        void SetRoundedListViewBaseItemChromeFallbackColor(KnownPropertyIndex propertyIndex, XUINT32 color);
        void SetRoundedListViewBaseItemChromeFallbackColors();

        bool m_roundedListViewBaseItemChromeFallbackColorsSet{ false };
#endif // DBG
    };
}
