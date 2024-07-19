// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides a mechanism to wrap an existing collection and provide
//      IInspectable versions of it.  This is useful when binding a
//      collection to an ItemsControl.

#pragma once

namespace DirectUI
{
    template<typename T>
    class IterableWrappedCollection:
        public wfc::IIterable<IInspectable*>,
        public wfc::IVector<IInspectable*>,
        public ctl::WeakReferenceSource
    {
    protected:
        IterableWrappedCollection()
        {
        }

        ~IterableWrappedCollection() override
        {
        }

    protected:
        BEGIN_INTERFACE_MAP(IterableWrappedCollection<T>, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(IterableWrappedCollection<T>, wfc::IIterable<IInspectable*>)
            INTERFACE_ENTRY(IterableWrappedCollection<T>, wfc::IVector<IInspectable*>)
        END_INTERFACE_MAP(IterableWrappedCollection<T>, ctl::WeakReferenceSource)

        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IIterable<IInspectable*>*>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IVector<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:
        //
        // Implementation
        //
        _Check_return_ HRESULT SetWrappedCollection(_In_ wfc::IVector<T*>* collection)
        {
            HRESULT hr = S_OK;

            IFCPTR(collection);
            SetPtrValue(m_tpWrappedCollection, collection);

        Cleanup:
            RRETURN(hr);
        }

        //
        // IIterable<IInspectable*>
        //
        IFACEMETHODIMP
        First(_Outptr_ wfc::IIterator<IInspectable*> **iterator) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IIterator<IInspectable*>> spResult;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(iterator);

            IFC(ctl::ComObject<TrackerIterator<IInspectable*>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
            spResult.Cast<TrackerIterator<IInspectable*>>()->SetCollection(this);

            *iterator = spResult.Detach();
        Cleanup:
            RRETURN(hr);
        }

        //
        // IVector<IInspectable*>
        //
        IFACEMETHODIMP
        Append(_In_opt_ IInspectable* item) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<T> spAsT;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(ctl::do_query_interface(spAsT, item));
            IFC(m_tpWrappedCollection->Append(spAsT.Get()));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        Clear() override
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(m_tpWrappedCollection->Clear());

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        get_Size(_Out_ unsigned *value) override
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(m_tpWrappedCollection->get_Size(value));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        GetAt(_In_ UINT index, _Outptr_ IInspectable** item) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<T> spAsT;

            ARG_VALIDRETURNPOINTER(item);

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(m_tpWrappedCollection->GetAt(index, spAsT.ReleaseAndGetAddressOf()));

            *item = spAsT.Detach();

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        GetView(_Outptr_result_maybenull_ wfc::IVectorView<IInspectable*>** view) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IVectorView<IInspectable*>> spResult;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(view);

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(ctl::ComObject<TrackerView<IInspectable*>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
            spResult.Cast<TrackerView<IInspectable*>>()->SetCollection(this);

            *view = spResult.Detach();

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        IndexOf(_In_opt_ IInspectable* item, _Out_ unsigned *index, _Out_ BOOLEAN *found) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<T> spAsT;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(ctl::do_query_interface(spAsT, item));
            IFC(m_tpWrappedCollection->IndexOf(spAsT.Get(), index, found));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        InsertAt(_In_ unsigned index, _In_ IInspectable* item) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<T> spAsT;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(ctl::do_query_interface(spAsT, item));
            IFC(m_tpWrappedCollection->InsertAt(index, spAsT.Get()));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        RemoveAt(_In_ UINT index) override
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(m_tpWrappedCollection->RemoveAt(index));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        RemoveAtEnd() override
        {
            HRESULT hr = S_OK;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(m_tpWrappedCollection->RemoveAtEnd());

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP
        SetAt(_In_ UINT index, _In_opt_ IInspectable* item) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<T> spAsT;

            IFCEXPECT(m_tpWrappedCollection.Get() != NULL);
            IFC(ctl::do_query_interface(spAsT, item));
            IFC(m_tpWrappedCollection->SetAt(index, spAsT.Get()));

        Cleanup:
            RRETURN(hr);
        }

    protected:
        TrackerPtr<wfc::IVector<T*>> m_tpWrappedCollection;

    }; // class IterableWrappedCollection

    template<typename T>
    class IterableWrappedObservableCollection:
        public wfc::IObservableVector<IInspectable*>,
        public IterableWrappedCollection<T>
    {
    protected:
        IterableWrappedObservableCollection()
        {
        }

        ~IterableWrappedObservableCollection() override
        {
            IGNOREHR(UnsubscribeFromCurrentCollection());
        }

    protected:
        BEGIN_INTERFACE_MAP(IterableWrappedObservableCollection<T>, IterableWrappedCollection<T>)
            INTERFACE_ENTRY(IterableWrappedObservableCollection<T>, wfc::IObservableVector<IInspectable*>)
        END_INTERFACE_MAP(IterableWrappedObservableCollection<T>, IterableWrappedCollection<T>)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IObservableVector<IInspectable*>)))
            {
                *ppObject = static_cast<wfc::IObservableVector<IInspectable*>*>(this);
            }
            else
            {
                RRETURN(IterableWrappedCollection<T>::QueryInterfaceImpl(iid, ppObject));
            }

            this->AddRefOuter();
            RRETURN(S_OK);
        }

        // Needed to walk the event source
        void OnReferenceTrackerWalk(INT walkType) final
        {
            IterableWrappedCollection<T>::OnReferenceTrackerWalk(walkType);
            m_evtVectorChangedHandlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));
        }

    public:
        //
        // Implementation
        //
        _Check_return_ HRESULT SetWrappedCollection(_In_ wfc::IVector<T*>* collection)
        {
            ctl::ComPtr<wfc::IObservableVector<T*>> spAsObservable;

            // Unhook previous event registration, if necessary.
            IFC_RETURN(UnsubscribeFromCurrentCollection());

            // Update our wrapped collection tracker pointer.
            IFC_RETURN(IterableWrappedCollection<T>::SetWrappedCollection(collection));

            // If the new collection is observable hook up event handlers.
            // If it isn't, take no additional action.
            if (SUCCEEDED(ctl::do_query_interface(spAsObservable, collection)))
            {
                IFC_RETURN(spAsObservable->add_VectorChanged(
                    Microsoft::WRL::Callback<wfc::VectorChangedEventHandler<T*>>(this, &IterableWrappedObservableCollection<T>::OnVectorChanged).Get(),
                    &m_VectorChangedToken)
                    );
            }

            return S_OK;
        }

        //
        // IObservableVector<Inspectable*>
        //
        IFACEMETHODIMP add_VectorChanged(_In_ wfc::VectorChangedEventHandler<IInspectable*>* handler, _Out_ EventRegistrationToken* token) override
        {
            m_evtVectorChangedHandlers.AddHandler(handler, token);
            return S_OK;
        }

        IFACEMETHODIMP remove_VectorChanged(_In_ EventRegistrationToken token) override
        {
            RRETURN(m_evtVectorChangedHandlers.RemoveHandler(token));
        }

    private:
        _Check_return_ HRESULT OnVectorChanged(_In_ wfc::IObservableVector<T*>* pSender, _In_ wfc::IVectorChangedEventArgs* pArgs)
        {
            // Forward the changed event to the registered handlers.
            RRETURN(m_evtVectorChangedHandlers.Raise(this, pArgs));
        }

        _Check_return_ HRESULT UnsubscribeFromCurrentCollection()
        {
            ctl::ComPtr<wfc::IObservableVector<T*>> wrappedAsObservable;
            if (m_VectorChangedToken.value != 0 &&
                this->m_tpWrappedCollection.TryGetSafeReference(&wrappedAsObservable))
            {
                IFC_RETURN(wrappedAsObservable->remove_VectorChanged(m_VectorChangedToken));
                ZeroMemory(&m_VectorChangedToken, sizeof(m_VectorChangedToken));
            }

            return S_OK;
        }

        TrackerEventSource<
            wfc::VectorChangedEventHandler<IInspectable*>,
            wfc::IObservableVector<IInspectable*>,
            wfc::IVectorChangedEventArgs> m_evtVectorChangedHandlers;

        // Registration for our wrapped vector's changed event.
        EventRegistrationToken m_VectorChangedToken;

    }; // class IterableWrappedObservableCollection
}
