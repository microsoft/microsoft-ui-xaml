// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GridViewItemPresenter.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(GridViewItemPresenter)
    {

    public:
        GridViewItemPresenter()
        { }
        _Check_return_ HRESULT get_GridViewItemPresenterPaddingImpl(_Out_ xaml::Thickness* pValue);
        _Check_return_ HRESULT put_GridViewItemPresenterPaddingImpl(_In_ xaml::Thickness value);
        _Check_return_ HRESULT get_GridViewItemPresenterHorizontalContentAlignmentImpl(_Out_ xaml::HorizontalAlignment* pValue);
        _Check_return_ HRESULT put_GridViewItemPresenterHorizontalContentAlignmentImpl(_In_ xaml::HorizontalAlignment value);
        _Check_return_ HRESULT get_GridViewItemPresenterVerticalContentAlignmentImpl(_Out_ xaml::VerticalAlignment* pValue);
        _Check_return_ HRESULT put_GridViewItemPresenterVerticalContentAlignmentImpl(_In_ xaml::VerticalAlignment value);
        ~GridViewItemPresenter() override
        { }
        
    protected:
        _Check_return_ HRESULT GoToElementStateCoreImpl(
            _In_ HSTRING stateName,
            _In_ BOOLEAN useTransitions,
            _Out_ BOOLEAN* returnValue) override;
    };
}
