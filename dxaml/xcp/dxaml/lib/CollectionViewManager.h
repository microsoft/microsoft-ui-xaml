// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{

    class CollectionViewManager
    {
    public:

        static _Check_return_ HRESULT GetViewRecord(
            _In_ IInspectable *pSource,
            _In_ xaml_data::ICollectionViewSource *pCVS,
            _Outptr_ xaml_data::ICollectionView **ppView);

        static _Check_return_ HRESULT GetVectorFromSource(
            _In_ IInspectable *pSource,
            _Outptr_ wfc::IVector<IInspectable *> **ppVector);

    private:

        static _Check_return_ HRESULT CreateNewView(
            _In_ IInspectable *pSource,
            _Outptr_ xaml_data::ICollectionView **ppResult);

        static _Check_return_ HRESULT CreateGroupedView(
            _In_ IInspectable *pSource,
            _In_ xaml::IPropertyPath *pItemsPath,
            _Outptr_ xaml_data::ICollectionView **ppResult);

    };
}
