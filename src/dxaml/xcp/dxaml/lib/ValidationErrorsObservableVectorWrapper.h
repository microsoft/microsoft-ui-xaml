// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A wrapper on the ValidationErrorsCollectionClass and turns it into the standard IObservableVector<IInspectable *>.
// This is required because template collection types are complicated and a QI for IObservableVector<IInspectable *>
// on a collection of IObservableVector<ValidationError*> will not succeed. We have the IBindableVector to work around
// this limitation at the language projection level for user defined collections, however internal collections are different.
// We specifically want to depend on the internal collecion implementation because we want any external collections of validation
// errors (while incredibly unlikely) to behave like any other collection.

#pragma once

#include "ValidationErrorsCollection_Partial.h"
namespace DirectUI
{
    class ValidationErrorsObservableVectorWrapper :
        public wfc::IObservableVector<IInspectable *>,
        public wfc::IVector<IInspectable *>,
        public wfc::IIterable<IInspectable *>,
        public ctl::WeakReferenceSource
    {
    protected:

        ValidationErrorsObservableVectorWrapper();
        ~ValidationErrorsObservableVectorWrapper() override;

    protected:

        BEGIN_INTERFACE_MAP(ValidationErrorsObservableVectorWrapper, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(ValidationErrorsObservableVectorWrapper, wfc::IObservableVector<IInspectable *>)
            INTERFACE_ENTRY(ValidationErrorsObservableVectorWrapper, wfc::IVector<IInspectable *>)
            INTERFACE_ENTRY(ValidationErrorsObservableVectorWrapper, wfc::IIterable<IInspectable *>)
        END_INTERFACE_MAP(ValidationErrorsObservableVectorWrapper, ctl::WeakReferenceSource)

        HRESULT QueryInterfaceImpl(const IID& riid, _COM_Outptr_ void** ppObject) final;

    public:

        // IObservableVector<IInspectable *>
        IFACEMETHOD(add_VectorChanged)(
            _In_ wfc::VectorChangedEventHandler<IInspectable *> *pHandler,
            _Out_ EventRegistrationToken *token) final;

        IFACEMETHOD(remove_VectorChanged)(
            EventRegistrationToken token) final;

        // IVector<IInspectable *>
        IFACEMETHOD(GetAt)(unsigned index, _COM_Outptr_ IInspectable **item) final;

        IFACEMETHOD(get_Size)(_Out_ unsigned *size) final;

        IFACEMETHOD(GetView)(_COM_Outptr_ wfc::IVectorView<IInspectable *>** view) final;

        IFACEMETHOD(IndexOf)(_In_ IInspectable * value, _Out_ unsigned *index, _Out_ boolean *found) final;

        IFACEMETHOD(SetAt)(unsigned index, _In_ IInspectable *item) final;

        IFACEMETHOD(InsertAt)(unsigned index, _In_ IInspectable *item) final;

        IFACEMETHOD(RemoveAt)(unsigned index) final;

        IFACEMETHOD(Append)(_In_ IInspectable * item) final;

        IFACEMETHOD(RemoveAtEnd)() final;

        IFACEMETHOD(Clear)() final;

        // IIterable<IInspectable *>
        IFACEMETHOD(First)(_COM_Outptr_ wfc::IIterator<IInspectable *> **value) final;
    public:

        static ctl::ComPtr<wfc::IIterable<IInspectable *>> CreateInstance(_In_ ValidationErrorsCollection* observableVector);
        static ctl::ComPtr<wfc::IVector<IInspectable *>> CreateInstanceAsVector(_In_ ValidationErrorsCollection* observableVector);

    private:
        void SetVector(_In_ ValidationErrorsCollection* vector);

        // Needed to walk the event source
        void OnReferenceTrackerWalk(INT walkType) final;

    private:
        _Check_return_ HRESULT ProcessVectorChange(_In_ IInspectable* args);
        _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::IVectorChangedEventArgs* args);

    private:

        TrackerEventSource<
            wfc::VectorChangedEventHandler<IInspectable*>,
            wfc::IObservableVector<IInspectable*>,
            wfc::IVectorChangedEventArgs> m_vectorChangedHandlers;
        ctl::ComPtr<ValidationErrorsCollection> m_vector;
        ctl::EventPtr<ValidationErrorsVectorChangedEventCallback> m_vectorChangedHandler;
    };
}
