// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      A wrapper on an IBindableVector that also has INotifyCollectionChanged
//      into the standard IObservableVector<IInspectable *>

#pragma once

#include "BindableVectorWrapper.h"

namespace DirectUI
{
    class BindableObservableVectorWrapper:
        public wfc::IObservableVector<IInspectable *>,
        public xaml_data::ISupportIncrementalLoading,
        public BindableVectorWrapper
    {
    protected:

        BindableObservableVectorWrapper();
        ~BindableObservableVectorWrapper() override;

    protected:

        BEGIN_INTERFACE_MAP(BindableObservableVectorWrapper, BindableVectorWrapper)
            INTERFACE_ENTRY(BindableObservableVectorWrapper, wfc::IObservableVector<IInspectable *>)
            INTERFACE_ENTRY(BindableObservableVectorWrapper, xaml_data::ISupportIncrementalLoading)
        END_INTERFACE_MAP(BindableObservableVectorWrapper, BindableVectorWrapper)

        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Outptr_ void** ppObject) override;

    public:


        // IObservableVector<IInspectable *>
        IFACEMETHOD(add_VectorChanged)(
            _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
            _Out_ EventRegistrationToken *token) override;

        IFACEMETHOD(remove_VectorChanged)(
            _In_ EventRegistrationToken token) override;

        // ISupportIncrementalLoading
        IFACEMETHOD(get_HasMoreItems)(_Out_ boolean *value) override;
        IFACEMETHOD(LoadMoreItemsAsync)(
            _In_ UINT32 count,
            _Outptr_ wf::IAsyncOperation<xaml_data::LoadMoreItemsResult> **operation) override;

    public:

        static _Check_return_ HRESULT CreateInstance(
            _In_ xaml_interop::IBindableVector *pVector,
            _In_ xaml_interop::INotifyCollectionChanged *pINCC,
            _Outptr_ wfc::IVector<IInspectable *> **ppVector);

        static _Check_return_ HRESULT CreateInstance(
            _In_ xaml_interop::IBindableObservableVector *pObservableVector,
            _Outptr_ wfc::IVector<IInspectable *> **ppVector);

    protected:

        _Check_return_ HRESULT InitializeInstance(
            _In_ xaml_interop::IBindableVector *pVector,
            _In_ xaml_interop::INotifyCollectionChanged *pINCC,
            _In_ IInspectable *pVirtualizingInterfaces );

        _Check_return_ HRESULT InitializeInstance(
            _In_ xaml_interop::IBindableVector *pVector,
            _In_ xaml_interop::IBindableObservableVector *pObservableVector,
            _In_ IInspectable *pVirtualizingInterfaces );


        _Check_return_ HRESULT SetINCC(_In_ xaml_interop::INotifyCollectionChanged *pINCC);
        _Check_return_ HRESULT SetObservableVector(_In_ xaml_interop::IBindableObservableVector *pObservableVector);
        void SetVirtualizingInterface(_In_ IInspectable* const pSource);

        // Needed to walk the event source
        void OnReferenceTrackerWalk(INT walkType) final;

    private:

        _Check_return_ HRESULT ProcessCollectionChange(_In_ xaml_interop::INotifyCollectionChangedEventArgs *pArgs);
        _Check_return_ HRESULT ProcessVectorChange(_In_ IInspectable *pArgs);

        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index);
        _Check_return_ HRESULT RaiseVectorChanged(wfc::IVectorChangedEventArgs* pArgs);

    private:

        TrackerEventSource<
            wfc::VectorChangedEventHandler<IInspectable *>,
            wfc::IObservableVector<IInspectable *>,
            wfc::IVectorChangedEventArgs> m_vectorChangedHandlers;

        TrackerPtr<xaml_interop::INotifyCollectionChanged> m_tpINCC;
        ctl::EventPtr<CollectionChangedEventCallback> m_epCollectionChangedHandler;

        TrackerPtr<xaml_interop::IBindableObservableVector> m_tpObservableVector;
        ctl::EventPtr<BindableVectorChangedEventCallback> m_epBindableVectorChangedHandler;

        // Virtualization interfaces
        TrackerPtr<xaml_data::ISupportIncrementalLoading> m_tpSupportIncrementalLoading;
    };
}


