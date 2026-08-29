// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "GridViewItemPresenter.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

using namespace Microsoft::WRL;

_Check_return_ HRESULT GridViewItemPresenter::GoToElementStateCoreImpl(
    _In_ HSTRING stateName,
    _In_ BOOLEAN useTransitions,
    _Out_ BOOLEAN* returnValue)
{
    CListViewBaseItemChrome* pChrome = static_cast<CListViewBaseItemChrome*>(GetHandle());
    bool wentToState = false;

    *returnValue = FALSE;

    IFC_RETURN(pChrome->GoToChromedState(
        HStringUtil::GetRawBuffer(stateName, nullptr),
        !!useTransitions,
        &wentToState));

    IFC_RETURN(ProcessAnimationCommands());

    *returnValue = !!wentToState;

    IFC_RETURN(__super::GoToElementStateCoreImpl(stateName, useTransitions, returnValue));

    return S_OK;
}

_Check_return_ HRESULT GridViewItemPresenter::get_GridViewItemPresenterPaddingImpl(_Out_ xaml::Thickness* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_Padding, pValue));
}
_Check_return_ HRESULT GridViewItemPresenter::put_GridViewItemPresenterPaddingImpl(_In_ xaml::Thickness value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_Padding, value));
}
_Check_return_ HRESULT GridViewItemPresenter::get_GridViewItemPresenterHorizontalContentAlignmentImpl(_Out_ xaml::HorizontalAlignment* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment, pValue));
}
_Check_return_ HRESULT GridViewItemPresenter::put_GridViewItemPresenterHorizontalContentAlignmentImpl(_In_ xaml::HorizontalAlignment value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_HorizontalContentAlignment, value));
}
_Check_return_ HRESULT GridViewItemPresenter::get_GridViewItemPresenterVerticalContentAlignmentImpl(_Out_ xaml::VerticalAlignment* pValue)
{
    RRETURN(GetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment, pValue));
}
_Check_return_ HRESULT GridViewItemPresenter::put_GridViewItemPresenterVerticalContentAlignmentImpl(_In_ xaml::VerticalAlignment value)
{
    RRETURN(SetValueByKnownIndex(KnownPropertyIndex::ContentPresenter_VerticalContentAlignment, value));
}

