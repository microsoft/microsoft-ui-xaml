// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "GroupStyle.g.h"

namespace DirectUI
{
    // The GroupStyle describes how to display the items in a Collection,
    // such as the collection obtained from ICollectionViewGroup.GroupItems
    PARTIAL_CLASS(GroupStyle)
    {
    protected:
        GroupStyle();
        ~GroupStyle() override;

        // Handle the custom property changed event and call the
        // OnPropertyChanged methods. 
        _Check_return_ HRESULT OnPropertyChanged2(_In_ const PropertyChangedParams& args) override;

    private:
        _Check_return_ HRESULT OnPropertyChanged(
            _In_reads_(nLength) const WCHAR* name,
            _In_ const XUINT32 nLength);
    };
}
