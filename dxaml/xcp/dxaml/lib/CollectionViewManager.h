// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{

    class CollectionViewManager
    {
    public:

        // *pIsShapeable: the view is a VectorCollectionView the CollectionViewSource can shape (WPF: the
        // ListCollectionView ViewManager creates for an IList).
        static _Check_return_ HRESULT GetViewRecord(
            _In_ IInspectable *pSource,
            _In_ xaml_data::ICollectionViewSource *pCVS,
            bool isShapingRequested,
            _Outptr_ xaml_data::ICollectionView **ppView,
            _Out_ bool *pIsShapeable);

        static _Check_return_ HRESULT GetVectorFromSource(
            _In_ IInspectable *pSource,
            _Outptr_ wfc::IVector<IInspectable *> **ppVector);

    private:

        static _Check_return_ HRESULT CreateNewView(
            _In_ IInspectable *pSource,
            bool isShapingRequested,
            _Outptr_ xaml_data::ICollectionView **ppResult,
            _Out_ bool *pIsShapeable);

        static _Check_return_ HRESULT CreateGroupedView(
            _In_ IInspectable *pSource,
            _In_ xaml::IPropertyPath *pItemsPath,
            _Outptr_ xaml_data::ICollectionView **ppResult);

    };
}
