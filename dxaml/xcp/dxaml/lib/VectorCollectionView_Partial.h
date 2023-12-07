// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A collection view implementation over an IVector<IInspectable *>

#pragma once

#include "VectorCollectionView.g.h"

namespace DirectUI
{
    PARTIAL_CLASS(VectorCollectionView)
    {
    protected:

        VectorCollectionView();
        ~VectorCollectionView() override;

    public:

        // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(_In_opt_ unsigned index, _Out_  IInspectable **item) override;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) override;

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable *>** view) override;

        IFACEMETHOD(IndexOf)(_In_opt_ IInspectable * value, _Out_ unsigned *index, _Out_ BOOLEAN *found) override;

        IFACEMETHOD(SetAt)(_In_ unsigned index, _In_opt_ IInspectable *item) override;

        IFACEMETHOD(InsertAt)(_In_ unsigned index, _In_ IInspectable *item) override;

        IFACEMETHOD(RemoveAt)(_In_ unsigned index) override;

        IFACEMETHOD(Append)(_In_opt_ IInspectable * item) override;

        IFACEMETHOD(RemoveAtEnd)() override;

        IFACEMETHOD(Clear)() override;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_Outptr_ wfc::IIterator<IInspectable *> **value) override;

        // ISupportIncrementalLoading portion of ICollectionView
        _Check_return_ HRESULT get_HasMoreItemsImpl(_Out_ BOOLEAN *value) override;
        _Check_return_ HRESULT LoadMoreItemsAsyncImpl(
            _In_ UINT32 count,
            _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation) override;

    public:

        // Internal only factory method to create a list collection view
        static _Check_return_ HRESULT CreateInstance(
            _In_ wfc::IVector<IInspectable *> *pSource,
            _Outptr_ xaml_data::ICollectionView  **instance);

    protected:

        _Check_return_ HRESULT SetSource(_In_ wfc::IVector<IInspectable *> *pSource);

    private:

        _Check_return_ HRESULT OnSourceVectorChanged(
            _In_ wfc::IObservableVector<IInspectable *> *pSender,
            _In_ IInspectable *pArgs);

    private:

        TrackerPtr<wfc::IVector<IInspectable *>> m_tpSource;
        TrackerPtr<xaml_data::ISupportIncrementalLoading> m_tpSupportIncrementalLoading;
        ctl::EventPtr<VectorChangedEventCallback> m_epVectorChangedHandler;
    };
}
