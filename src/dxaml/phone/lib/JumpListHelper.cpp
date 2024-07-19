// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace Private
{

/// <summary>
///  Returns true if the group has items
/// </summary>
_Check_return_ HRESULT JumpListHelper::HasItems(_In_opt_ IInspectable *pGroup, _Out_ BOOLEAN *pResult)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_data::ICollectionViewGroup> spGroup;

    *pResult = FALSE;

    if ( pGroup != nullptr && SUCCEEDED(pGroup->QueryInterface(_uuidof(xaml_data::ICollectionViewGroup), &spGroup)))
    {
        wrl::ComPtr<wfc::IObservableVector<IInspectable*>> spGroupItems;
        wrl::ComPtr<wfc::IVector<IInspectable*>> spGroupItemsAsV;
        UINT groupSize = 0;

        IFC(spGroup->get_GroupItems(&spGroupItems));
        IFC(spGroupItems.As<wfc::IVector<IInspectable*>>(&spGroupItemsAsV));
        IFC(spGroupItemsAsV->get_Size(&groupSize));
        if(groupSize > 0)
        {
            *pResult = TRUE;
        }
    }

Cleanup:
    RRETURN(hr);
}

}
