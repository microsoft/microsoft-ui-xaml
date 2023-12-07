// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A collection view implementation over an IVectorView<IInspectable *>

#pragma once

#include "VectorViewCollectionView.g.h"
#include "ReadOnlyCollectionView.h"

namespace DirectUI
{
    class __declspec(uuid(__VectorViewCollectionView_GUID)) VectorViewCollectionView:
        public ReadOnlyCollectionView<VectorViewCollectionViewGenerated>
    {
    protected:

        VectorViewCollectionView();
        ~VectorViewCollectionView() override;

    public:

        // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(_In_opt_ unsigned index, _Out_  IInspectable **item) override;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) override;

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view) override;

        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found) override;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<IInspectable *> **value) override;

    public:

        // Jupiter only factory method to create a list collection view
        static _Check_return_ HRESULT CreateInstance(
            _In_ wfc::IVectorView<IInspectable *> *pSource,
            _Outptr_ xaml_data::ICollectionView  **instance);

    protected:

        void SetSource(_In_ wfc::IVectorView<IInspectable *>* const pSource);

    private:

        TrackerPtr<wfc::IVectorView<IInspectable *>> m_tpSource;
    };
}
