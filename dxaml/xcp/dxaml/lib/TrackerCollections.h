// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VectorChangedEventArgs.g.h"

namespace DirectUI
{
    template <typename T, typename TCollection = wfc::IVector<T>>
    class __declspec(novtable) TrackerIterator:
        public wfc::IIterator<T>,
        public ctl::WeakReferenceSource
    {
    protected:

        typedef typename wf::Internal::GetAbiType<typename wfc::IIterator<T>::T_complex>::type T_abi;

        // This class is marked novtable, so must not be instantiated directly.
        TrackerIterator()
            : m_nCurrentIndex(0)
        { }
        ~TrackerIterator() override { }

        BEGIN_INTERFACE_MAP(TrackerIterator, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(TrackerIterator, wfc::IIterator<T>)
        END_INTERFACE_MAP(TrackerIterator, ctl::WeakReferenceSource)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterator<T>)))
            {
                *ppObject = static_cast<wfc::IIterator<T> *>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }


    public:

        // IIterator<T>
        IFACEMETHODIMP get_Current(_Out_ T_abi *current) override
        {
            HRESULT hr = S_OK;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(current);

            IFC(m_tpCollectionRef->GetAt(m_nCurrentIndex, current));

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP get_HasCurrent(_Out_ BOOLEAN *hasCurrent) override
        {
            HRESULT hr = S_OK;
            unsigned nSize;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(hasCurrent);

            IFC(m_tpCollectionRef->get_Size(&nSize));
            *hasCurrent = m_nCurrentIndex < nSize;

        Cleanup:
            RRETURN(hr);
        }

        IFACEMETHODIMP MoveNext(_Out_ BOOLEAN *hasCurrent) override
        {
            HRESULT hr = S_OK;
            unsigned nSize;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(hasCurrent);

            IFC(m_tpCollectionRef->get_Size(&nSize));

            *hasCurrent = FALSE;

            m_nCurrentIndex++;

            if (m_nCurrentIndex < nSize)
            {
                // We have
                *hasCurrent = TRUE;
            }
            else
            {
                // Use this as a marker that we're done, this will make it so
                // HasCurrent will also return false
                m_nCurrentIndex = nSize;
            }

        Cleanup:
            RRETURN(hr);
        }

    public:

        void SetCollection(_In_ TCollection* const pVector)
        {
            SetPtrValue(m_tpCollectionRef, pVector);
        }

    private:

        TrackerPtr<TCollection> m_tpCollectionRef;
        UINT m_nCurrentIndex;
    };

    template <typename T>
    class __declspec(novtable) TrackerView:
        public wfc::IIterable<T>,
        public wfc::IVectorView<T>,
        public ctl::WeakReferenceSource
    {
    protected:

        typedef typename wf::Internal::GetAbiType<typename wfc::IVectorView<T>::T_complex>::type T_abi;

        // This class is marked novtable, so must not be instantiated directly.
        TrackerView()  { }
        ~TrackerView() override { }

        BEGIN_INTERFACE_MAP(TrackerView<T>, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(TrackerView<T>, wfc::IIterable<T>)
            INTERFACE_ENTRY(TrackerView<T>, wfc::IVector<T>)
        END_INTERFACE_MAP(TrackerView<T>, ctl::WeakReferenceSource)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<T>)))
            {
                *ppObject = static_cast<wfc::IIterable<T> *>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVectorView<T>)))
            {
                *ppObject = static_cast<wfc::IVectorView<T> *>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:

        // IIterable<T>
        IFACEMETHODIMP First(_Outptr_ wfc::IIterator<T> **iterator) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IIterator<T>> spResult;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(iterator);

            hr = ctl::ComObject<TrackerIterator<T, wfc::IVectorView<T>>>::CreateInstance(spResult.ReleaseAndGetAddressOf());
            IFC(hr);

            spResult.template Cast<TrackerIterator<T, wfc::IVectorView<T>>>()->SetCollection(this);

            *iterator = spResult.Detach();

        Cleanup:

            RRETURN(hr);
        }

        // IVectorView<T>
        IFACEMETHODIMP GetAt(_In_ UINT index, _Outptr_ T_abi *item) override
        {
            HRESULT hr = S_OK;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(item);

            IFC(m_tpCollectionRef->GetAt(index, item));

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHODIMP get_Size(_Out_ UINT *value) override
        {
            HRESULT hr = S_OK;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(value);

            IFC(m_tpCollectionRef->get_Size(value));

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHODIMP IndexOf(_In_ T_abi item, _Out_ UINT *index, _Out_ BOOLEAN *returnValue) override
        {
            HRESULT hr = S_OK;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(index);
            ARG_VALIDRETURNPOINTER(returnValue);

            IFC(m_tpCollectionRef->IndexOf(item, index, returnValue));

        Cleanup:

            RRETURN(hr);
        }

    public:

        void SetCollection(_In_ wfc::IVector<T>* const pVector)
        {
            SetPtrValue(m_tpCollectionRef, pVector);
        }

    private:

        TrackerPtr<wfc::IVector<T>> m_tpCollectionRef;
    };

    template <typename T>
    class __declspec(novtable) TrackerCollection:
        public wfc::IIterable<T>,
        public wfc::IVector<T>,
        public IUntypedVector,
        public ctl::WeakReferenceSource
    {
    protected:
        typedef typename wf::Internal::GetAbiType<typename wfc::IVector<T>::T_complex>::type T_abi;
        typedef typename std::remove_pointer<T_abi>::type T_type;

    protected:

        // This class is marked novtable, so must not be instantiated directly.
        TrackerCollection()  { }
        ~TrackerCollection() override { }

        // TODO: Create the right string for this T
        INSPECTABLE_CLASS(L"TrackerCollection<T>");

        BEGIN_INTERFACE_MAP(TrackerCollection<T>, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(TrackerCollection<T>, wfc::IIterable<T>)
            INTERFACE_ENTRY(TrackerCollection<T>, wfc::IVector<T>)
        END_INTERFACE_MAP(TrackerCollection<T>, ctl::WeakReferenceSource)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IIterable<T>)))
            {
                *ppObject = static_cast<wfc::IIterable<T> *>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(wfc::IVector<T>)))
            {
                *ppObject = static_cast<wfc::IVector<T> *>(this);
            }
            else if (InlineIsEqualGUID(iid, __uuidof(IUntypedVector)))
            {
                *ppObject = static_cast<IUntypedVector*>(this);
            }
            else
            {
                RRETURN(ctl::WeakReferenceSource::QueryInterfaceImpl(iid, ppObject));
            }

            AddRefOuter();
            RRETURN(S_OK);
        }

    public:

        // IIterable<T>
        IFACEMETHODIMP First(_Outptr_ wfc::IIterator<T> **iterator) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IIterator<T>> spResult;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(iterator);

            IFC(ctl::ComObject<TrackerIterator<T>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
            spResult.template Cast<TrackerIterator<T>>()->SetCollection(this);

            *iterator = spResult.Detach();

        Cleanup:

            RRETURN(hr);
        }

        // IVector<T>
            // Read portion of the interface
        IFACEMETHODIMP GetAt(_In_ UINT index, _Outptr_ T_abi *item) override
        {
            RRETURN(m_vector.GetAt(index, item));
        }

        // Get an unsafe reference to an item; it may be in a GC'd (but not finalized) state, and no ref-count is taken on it.
        _Check_return_ HRESULT GetAsReferenceTrackerUnsafe(_In_ UINT index, _Outptr_ xaml_hosting::IReferenceTrackerInternal** itemNoRef)
        {
            RRETURN(m_vector.GetAsReferenceTrackerUnsafe(index, itemNoRef));
        }

        IFACEMETHODIMP get_Size(_Out_ unsigned *value) override
        {
            *value = m_vector.Size();
            RRETURN(S_OK);
        }

        IFACEMETHOD(GetView)(_Outptr_result_maybenull_ wfc::IVectorView<T>** view) override
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IVectorView<T>> spResult;

            IFC(CheckThread());
            ARG_VALIDRETURNPOINTER(view);

            IFC(ctl::ComObject<TrackerView<T>>::CreateInstance(spResult.ReleaseAndGetAddressOf()));
            spResult.template Cast<TrackerView<T>>()->SetCollection(this);

            *view = spResult.Detach();

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHOD(IndexOf)(_In_opt_ T_abi value, _Out_ unsigned *index, _Out_ BOOLEAN *found) override
        {
            HRESULT hr = S_OK;
            auto current = m_vector.Begin();
            auto end = m_vector.End();
            bool areEqual = false;
            unsigned indexResult = 0;

            // Assume that we won't find it
            *found = FALSE;
            *index = 0;

            for (; current != end; current++, indexResult++)
            {
                IFC(PropertyValue::AreEqual(value, (*current).Get(), &areEqual));
                if (areEqual)
                {
                    // Found the item
                    *found = TRUE;
                    *index = indexResult;
                    break;
                }
            }

        Cleanup:

            RRETURN(hr);
        }

            // Write portion of the interface
        IFACEMETHODIMP SetAt(_In_ UINT index, _In_opt_ T_abi item) override
        {
            RRETURN(m_vector.SetAt(index, item));
        }

        IFACEMETHODIMP InsertAt(_In_ unsigned index, _In_ T_abi item) override
        {
            RRETURN(m_vector.InsertAt(index, item));
        }

        IFACEMETHODIMP RemoveAt(_In_ UINT index) override
        {
            RRETURN(m_vector.RemoveAt(index));
        }

        IFACEMETHODIMP Append(_In_opt_ T_abi item) override
        {
            m_vector.Append(item);
            return S_OK;
        }

        IFACEMETHODIMP UntypedAppend(_In_ IInspectable* pItem) override
        {
            HRESULT hr = S_OK;
            T_abi pTypedItem = NULL;

            IFC(ctl::do_query_interface(pTypedItem, pItem));
            IFC(Append(pTypedItem));

        Cleanup:
            ReleaseInterface(pTypedItem);
            RRETURN(hr);
        }

        IFACEMETHODIMP UntypedGetSize(_Out_ unsigned int* pSize) override
        {
            IFC_RETURN(get_Size(pSize));
            return S_OK;
        }

        IFACEMETHODIMP UntypedGetAt(_In_ uint32_t index, _COM_Outptr_ IInspectable** ppItem) override
        {
            wrl::ComPtr<typename std::remove_pointer<T_abi>::type> spTypedItem;

            IFC_RETURN(GetAt(index, &spTypedItem));
            IFC_RETURN(spTypedItem.CopyTo(ppItem));
            return S_OK;
        }

        IFACEMETHODIMP UntypedInsertAt(_In_ uint32_t index, _In_ IInspectable* pItem) override
        {
            wrl::ComPtr<IInspectable> spItem(pItem);
            wrl::ComPtr<typename std::remove_pointer<T_abi>::type> spTypedItem;

            IFC_RETURN(spItem.As(&spTypedItem));
            if (index == Size())
            {
                // Seems like a bug in the bounds check for TrackerCollection::InsertAt. It checks
                // that index < size when it should be index <= size.
                IFC_RETURN(Append(spTypedItem.Get()));
            }
            else
            {
                IFC_RETURN(SetAt(index, spTypedItem.Get()));
            }
            return S_OK;
        }

        IFACEMETHODIMP UntypedRemoveAt(_In_ uint32_t index) override
        {
            IFC_RETURN(RemoveAt(index));
            return S_OK;
        }

        IFACEMETHODIMP UntypedClear() override
        {
            IFC_RETURN(Clear());
            return S_OK;
        }

        IFACEMETHODIMP RemoveAtEnd() override
        {
            RRETURN(m_vector.RemoveAt(m_vector.Size() - 1));
        }


        IFACEMETHODIMP Clear() override
        {
            m_vector.Clear();
            RRETURN(S_OK);
        }

    protected:

        void OnReferenceTrackerWalk(INT walkType) override
        {
            m_vector.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));

            ctl::WeakReferenceSource::OnReferenceTrackerWalk(walkType);
        }

        UINT Size() const
        {
            return m_vector.Size();
        }

    private:

        TrackerPtrVector<T_type> m_vector;
    };

    template <typename T>
    class __declspec(novtable) ReadOnlyTrackerCollection : public TrackerCollection<T>
    {
    public:
        using T_abi = typename TrackerCollection<T>::T_abi;

        // Override the "writting" methods to treat this
        // as a read only array
        IFACEMETHODIMP SetAt(_In_ UINT index, _In_opt_ T_abi item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP InsertAt(_In_ UINT index, _In_opt_ T_abi item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP RemoveAt(_In_ UINT index) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP Append(_In_opt_ T_abi item) override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP RemoveAtEnd() override { RRETURN(E_NOTIMPL); }
        IFACEMETHODIMP Clear() override { RRETURN(E_NOTIMPL); }

        // Internal methods to modify the collection
        _Check_return_ HRESULT InternalSetAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            RRETURN(TrackerCollection<T>::SetAt(index, item));
        }

        _Check_return_ HRESULT InternalInsertAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            RRETURN(TrackerCollection<T>::InsertAt(index, item));
        }

        _Check_return_ HRESULT InternalRemoveAt(_In_ unsigned index)
        {
            RRETURN(TrackerCollection<T>::RemoveAt(index));
        }

        _Check_return_ HRESULT InternalAppend(_In_ T_abi item)
        {
            RRETURN(TrackerCollection<T>::Append(item));
        }

        _Check_return_ HRESULT InternalRemoveAtEnd()
        {
            RRETURN(TrackerCollection<T>::RemoveAtEnd());
        }

        _Check_return_ HRESULT InternalClear()
        {
            RRETURN(TrackerCollection<T>::Clear());
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        ReadOnlyTrackerCollection() = default;
    };

    template <typename THandler, typename TSource, typename TArgs>
    class TrackerEventSource
    {
    public:

        void AddHandler(_In_ THandler *pHandler, _In_ EventRegistrationToken *token)
        {
            m_handlers.Append(pHandler);
            token->value = reinterpret_cast<INT64>(pHandler);
        }

        _Check_return_ HRESULT RemoveHandler(_In_ EventRegistrationToken token)
        {
            HRESULT hr = S_OK;
            THandler* pHandler = reinterpret_cast<THandler *>(token.value);

            IFC(m_handlers.Remove(pHandler));

        Cleanup:

            RRETURN(hr);
        }

        _Check_return_ HRESULT Raise(_In_ TSource *pSource, _In_ TArgs* pArgs)
        {
            HRESULT hr = S_OK;

            switch (m_handlers.Size())
            {
            case 0:
                break;
            case 1:
                {
                    // One handler... No need to copy things to a temporary list.
                    auto itrDelegate = m_handlers.Begin();
                    ASSERT(itrDelegate != m_handlers.End());
                    ctl::ComPtr<THandler> spHandler = (*itrDelegate).Get();

                    IFC(spHandler->Invoke(pSource, pArgs));
                }
                break;
            default:
                {
                    std::vector<ctl::ComPtr<THandler>> handlers;

                    // Copy the handlers first to avoid re-entrancy issues
                    std::for_each(m_handlers.Begin(), m_handlers.End(),
                        [&handlers](const TrackerPtr<THandler> &tracker) {
                            handlers.push_back(tracker.Get());
                    });

                    // Raise the event from the copy
                    for (auto current = handlers.begin(); current != handlers.end(); current++)
                    {
                        IFC((*current)->Invoke(pSource, pArgs));
                    }
                }
                break;
            }

        Cleanup:

            RRETURN(hr);
        }

        bool Empty() const
        {
            return m_handlers.Empty();
        }

        bool ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ BOOLEAN fIsRoot = FALSE)
        {
            m_handlers.ReferenceTrackerWalk(walkType);
            return true;
        }

    private:

        TrackerPtrVector<THandler> m_handlers;
    };

    template <typename T>
    class __declspec(novtable) ObservableTrackerCollection:
        public wfc::IObservableVector<T>,
        public TrackerCollection<T>
    {
    protected:

        BEGIN_INTERFACE_MAP(ObservableTrackerCollection<T>, TrackerCollection<T>)
            INTERFACE_ENTRY(ObservableTrackerCollection<T>, wfc::IObservableVector<T>)
        END_INTERFACE_MAP(ObservableTrackerCollection<T>, TrackerCollection<T>)

        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override
        {
            if (InlineIsEqualGUID(iid, __uuidof(wfc::IObservableVector<T>)))
            {
                *ppObject = static_cast<wfc::IObservableVector<T>*>(this);
            }
            else
            {
                RRETURN(TrackerCollection<T>::QueryInterfaceImpl(iid, ppObject));
            }

            this->AddRefOuter();
            RRETURN(S_OK);
        }

        // This class is marked novtable, so must not be instantiated directly.
        ObservableTrackerCollection() = default;

    public:
        using T_abi = typename TrackerCollection<T>::T_abi;

        // IObservableVector<T>
        IFACEMETHODIMP add_VectorChanged(_In_ wfc::VectorChangedEventHandler<T> *handler, _Out_ EventRegistrationToken *token) override
        {
            m_handlers.AddHandler(handler, token);
            return S_OK;
        }

        IFACEMETHODIMP remove_VectorChanged(_In_ EventRegistrationToken token) override
        {
            RRETURN(m_handlers.RemoveHandler(token));
        }

        // Overrides of the write portion of IVector<T> since that is the
        // part that can cause changes
        IFACEMETHODIMP SetAt(_In_ UINT index, _In_opt_ T_abi item) override
        {
            HRESULT hr = S_OK;

            UINT size = this->Size();
            IFCEXPECTRC(index < size, E_BOUNDS);

            IFC(TrackerCollection<T>::SetAt(index, item));

            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemChanged, index));

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHODIMP InsertAt(_In_ unsigned index, _In_ T_abi item) override
        {
            HRESULT hr = S_OK;

            UINT size = this->Size();
            IFCEXPECTRC(index <= size, E_BOUNDS);

            IFC(TrackerCollection<T>::InsertAt(index, item));

            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, index));

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHODIMP RemoveAt(_In_ UINT index) override
        {
            HRESULT hr = S_OK;

            UINT size = this->Size();
            IFCEXPECTRC(index < size, E_BOUNDS);

            IFC(TrackerCollection<T>::RemoveAt(index));

            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemRemoved, index));

        Cleanup:

            RRETURN(hr);
        }

        IFACEMETHODIMP Append(_In_opt_ T_abi item) override
        {
            HRESULT hr = S_OK;

            IFC(TrackerCollection<T>::Append(item));

            IFC(RaiseVectorChanged(wfc::CollectionChange_ItemInserted, this->Size() - 1));

        Cleanup:

            RRETURN(hr);
        }


        IFACEMETHODIMP RemoveAtEnd() override
        {
            UINT size = this->Size();

            IFCEXPECTRC_RETURN(size > 0, E_BOUNDS);

            IFC_RETURN(RemoveAt(size - 1));

            return S_OK;
        }


        IFACEMETHODIMP Clear() override
        {
            HRESULT hr = S_OK;

            TrackerCollection<T>::Clear();

            IFC(RaiseVectorChanged(wfc::CollectionChange_Reset, 0));

        Cleanup:

            RRETURN(hr);
        }

    protected:

        void OnReferenceTrackerWalk(INT walkType) final
        {
            m_handlers.ReferenceTrackerWalk(static_cast<EReferenceTrackerWalkType>(walkType));

            TrackerCollection<T>::OnReferenceTrackerWalk(walkType);
        }

        virtual _Check_return_ HRESULT RaiseVectorChanged(_In_ wfc::CollectionChange action, UINT index)
        {
            HRESULT hr = S_OK;
            ctl::ComPtr<wfc::IVectorChangedEventArgs> spArgs;
            VectorChangedEventArgs *pArgs = NULL;

            if (m_handlers.Empty())
            {
                goto Cleanup;
            }

            // Create the args
            IFC(ctl::ComObject<VectorChangedEventArgs>::CreateInstance(spArgs.ReleaseAndGetAddressOf()));
            spArgs.CastTo(&pArgs);
            IFC(pArgs->put_CollectionChange(action));
            IFC(pArgs->put_Index(index));

            // Raise the event
            IFC(m_handlers.Raise(this, spArgs.Get()));

        Cleanup:

            RRETURN(hr);
        }

    private:

        TrackerEventSource<
            wfc::VectorChangedEventHandler<T>,
            wfc::IObservableVector<T>,
            wfc::IVectorChangedEventArgs> m_handlers;
    };

    template <typename T>
    class __declspec(novtable) ReadOnlyObservableTrackerCollection : public ObservableTrackerCollection<T>
    {
    public:
        using T_abi = typename ObservableTrackerCollection<T>::T_abi;

        // Override of the writting methods to fail
        IFACEMETHODIMP SetAt(_In_ unsigned index, _In_opt_ T_abi item) override
        { return E_NOTIMPL; }

        IFACEMETHODIMP InsertAt(_In_ unsigned index, _In_ T_abi item) override
        { return E_NOTIMPL; }

        IFACEMETHODIMP RemoveAt(_In_ unsigned index) override
        { return E_NOTIMPL; }

        IFACEMETHODIMP Append(_In_ T_abi item) override
        { return E_NOTIMPL; }

        IFACEMETHODIMP RemoveAtEnd() override
        { return E_NOTIMPL; }

        IFACEMETHODIMP Clear() override
        { return E_NOTIMPL; }

    public:

        // Internal methods to be able to write to the collection from inside
        _Check_return_ HRESULT InternalSetAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            RRETURN(ObservableTrackerCollection<T>::SetAt(index, item));
        }

        _Check_return_ HRESULT InternalInsertAt(_In_ unsigned index, _In_opt_ T_abi item)
        {
            RRETURN(ObservableTrackerCollection<T>::InsertAt(index, item));
        }

        _Check_return_ HRESULT InternalRemoveAt(_In_ unsigned index)
        {
            RRETURN(ObservableTrackerCollection<T>::RemoveAt(index));
        }

        _Check_return_ HRESULT InternalAppend(_In_ T_abi item)
        {
            RRETURN(ObservableTrackerCollection<T>::Append(item));
        }

        _Check_return_ HRESULT InternalRemoveAtEnd()
        {
            RRETURN(ObservableTrackerCollection<T>::RemoveAtEnd());
        }

        _Check_return_ HRESULT InternalClear()
        {
            RRETURN(ObservableTrackerCollection<T>::Clear());
        }

    protected:
        // This class is marked novtable, so must not be instantiated directly.
        ReadOnlyObservableTrackerCollection() = default;
    };
}

