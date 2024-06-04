// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ListViewPersistenceHelper.g.h"
#include "ListViewBase.g.h"

using namespace DirectUI;

_Check_return_ HRESULT
ListViewPersistenceHelperFactory::GetRelativeScrollPositionImpl(
_In_ xaml_controls::IListViewBase* listViewBase,
_In_ xaml_controls::IListViewItemToKeyHandler* itemToKeyHandler,
_Out_ HSTRING* returnValue)
{
    HRESULT hr = S_OK;

    IFC(static_cast<ListViewBase*>(listViewBase)->GetRelativeScrollPosition(
        itemToKeyHandler,
        returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListViewPersistenceHelperFactory::SetRelativeScrollPositionAsyncImpl(
_In_ xaml_controls::IListViewBase* listViewBase,
_In_ HSTRING relativeScrollPosition,
_In_ xaml_controls::IListViewKeyToItemHandler* keyToItemHandler,
_Outptr_ wf::IAsyncAction** returnValue)
{
    HRESULT hr = S_OK;

    IFC(static_cast<ListViewBase*>(listViewBase)->SetRelativeScrollPositionAsync(
        relativeScrollPosition,
        keyToItemHandler,
        returnValue));

Cleanup:
    RRETURN(hr);
}
