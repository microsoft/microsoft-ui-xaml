// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A collection view implementation over an IIterable<IInspectable *>

#pragma once

#include "IterableCollectionView.g.h"
#include "ReadOnlyCollectionView.h"

namespace DirectUI
{
    class __declspec(uuid(__IterableCollectionView_GUID)) IterableCollectionView:
        public ReadOnlyCollectionView<IterableCollectionViewGenerated>
    {

    public:

        // Jupiter only factory method
        static _Check_return_ HRESULT CreateInstance(
            _In_ wfc::IIterable<IInspectable *> *pSource,
            _Outptr_ xaml_data::ICollectionView **instance);

    };
}
