// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TrackerTargetReference.h"
#include "HStringUtil.h"

class CDependencyObject;

namespace DirectUI
{
    template <class T>
    class TrackerPtr
    {
    public:
        TrackerPtr() = default;

        TrackerPtr(const TrackerPtr& other) = delete;

        TrackerPtr(TrackerPtr&& other)
        {
            *this = std::move(other);
        }

        ~TrackerPtr()
        {
            m_trackerReference.Clear();
        }

        TrackerPtr& operator= (const TrackerPtr& other) = delete;

        TrackerPtr& operator= (TrackerPtr&& other)
        {
            if (*this != other)
            {
                m_trackerReference.Assign(std::move(other.m_trackerReference));
            }

            return *this;
        }

        typedef T InterfaceType;

    private:

        template <class U>
        _Check_return_ HRESULT SetWithQI(_In_ U* ptr)
        {
            HRESULT hr = S_OK;
            T* pCasted = NULL;

            IFC(ctl::do_query_interface(pCasted, ptr));
            Set(pCasted);

        Cleanup:
            ReleaseInterface(pCasted);

            RRETURN(hr);
        }

        template <>
        _Check_return_ HRESULT SetWithQI<T>(_In_ T* ptr)
        {
            Set(ptr);
            return S_OK;
        }

        template <class U>
        void SetWithQIOrNull(_In_ U* ptr)
        {
            T* const pCasted = ctl::query_interface<T>(ptr);

            Set(pCasted);

            ReleaseInterfaceNoNULL(pCasted);
        }

        template <typename U>
        void Set(_In_ U* ptr
            #if DBG
            , bool fStatic = false
            #endif
            )
        {
            static_assert(!ctl::IsEventPtrCompatible<T>::value, "TrackerPtr cannot be used to hold references to events, use EventPtr for that");
            static_assert(!ctl::IsWeakEventPtrCompatible<T>::value, "TrackerPtr cannot be used to hold references to weak events, use WeakEventPtr for that");

            if (ptr)
            {
                #if DBG
                m_trackerReference.Set(ctl::iunknown_cast(static_cast<T*>(ptr)), fStatic);
                #else
                m_trackerReference.Set(ctl::iunknown_cast(static_cast<T*>(ptr)));
                #endif
            }
            else
            {
                Clear();
            }
        }

        template <typename U>
        void Set(const ctl::ComPtr<U>& sp)
        {
            Set(sp.Get());
        }

    public:

        template <class U>
        U* Cast() const
        {
           return static_cast<U*>(Get());
        }


        T* Get() const
        {
            return ctl::down_cast<T>(m_trackerReference.Get());
        }

        xaml::IDependencyObject* GetAsDO() const
        {
            return m_trackerReference.GetAsDO();
        }

        CDependencyObject* GetAsCoreDO() const
        {
            return m_trackerReference.GetAsCoreDO();
        }

        // Get the reference tracker out of the value (if it's a reference tracker).
        // This was QI'd during the Set, but might be pointing to a GC'd object now, so it can't even be QI'd.
        // Note that this is raw, not AddRef'd
        xaml_hosting::IReferenceTrackerInternal* GetAsReferenceTrackerUnsafe() const
        {
            return m_trackerReference.GetAsReferenceTrackerUnsafe();
        }

        //
        // Call this method to peg the target of this TrackerPtr, if the target is still reachable.
        // If it's not reachable, the returned AutoPeg behaves as a nullptr.  When the returned
        // AutoPeg falls out of scope it will release the peg.
        //
        // This is primarily useful during destruction of an object, where it wants to clean its state
        // out of referenced objects, but needn't/shouldn't if those objects are not reachable
        // (i.e. they have been GC'd).
        //
        ctl::AutoPeg<xaml_hosting::IReferenceTrackerInternal, TRUE> TryMakeAutoPeg()
        {
            return m_trackerReference.TryMakeAutoPeg();
        }

        template <typename U>
        bool TryGetSafeReference(ctl::Internal::ComPtrRef<ctl::ComPtr<U>> ppSafeReference) const
        {
            return m_trackerReference.TryGetSafeReference(__uuidof(U), reinterpret_cast<void**>(ppSafeReference.ReleaseAndGetAddressOf()));
        }

        ctl::ComPtr<T> GetSafeReference() const
        {
            ctl::ComPtr<T> spResult;
            m_trackerReference.TryGetSafeReference(__uuidof(T), reinterpret_cast<void**>(spResult.ReleaseAndGetAddressOf()));
            return spResult;
        }

        typename ctl::Internal::RemoveIUnknown<T>::ReturnType* operator-> () const
        {
            return static_cast<typename ctl::Internal::RemoveIUnknown<T>::ReturnType*>(ctl::down_cast<T>(m_trackerReference.Get()));
        }

        bool operator== (const TrackerPtr& other) const
        {
            return GetNoMptStress() == other.GetNoMptStress();
        }

        bool operator== (const T* other) const
        {
            return GetNoMptStress() == other;
        }

        bool operator != (const TrackerPtr& other) const
        {
            return !(*this == other);
        }

        bool operator != (const T* other) const
        {
            return !(*this == other);
        }

        operator bool () const
        {
            return m_trackerReference.GetNoMptStress() != nullptr;
        }

        // See if a value is set (this doesn't assert IsValueSafeToUse)
        bool IsSet() const
        {
            return m_trackerReference.IsSet();
        }

        // Copy to pointer of same type as this - simple addref and copy
        template<typename U>
        _Check_return_ typename std::enable_if<std::is_same<T, U>::value, HRESULT>::type
            CopyTo(_Outptr_ U** ptr)
        {
            AddRefInterface(m_trackerReference.Get());
            *ptr = ctl::down_cast<T>(m_trackerReference.Get());
            RRETURN(S_OK);
        }

        // Copy to pointer of same type - simple addref and copy with a cast to IInspectable (to disambiguate diamond inheritance)
        template<typename U>
        _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && std::is_same<IInspectable, U>::value, HRESULT>::type
            CopyTo(_Outptr_ U** ptr)
        {
            AddRefInterface(m_trackerReference.Get());
            *ptr = ctl::iinspectable_cast(ctl::down_cast<T>(m_trackerReference.Get()));
            RRETURN(S_OK);
        }

        // Copy to pointer of parent (implicitly convertible) type - simple addref and copy
        template<typename U>
        _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && __is_convertible_to(T*, U*), HRESULT>::type
            CopyTo(_Outptr_ U** ptr)
        {
            AddRefInterface(m_trackerReference.Get());
            *ptr = ctl::down_cast<T>(m_trackerReference.Get());
            RRETURN(S_OK);
        }

        // Copy to other, possibly incompatible type - perform QI
        template<typename U>
        _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && !__is_convertible_to(T*, U*), HRESULT>::type
            CopyTo(_Outptr_ U** ptr) const
        {
            RRETURN(ctl::do_query_interface(*ptr, ctl::down_cast<T>(m_trackerReference.Get())));
        }

        // query for U interface
        template<typename U>
        _Check_return_ HRESULT As(ctl::Internal::ComPtrRef<ctl::ComPtr<U>> p) const
        {
            U **ppInterface = p.ReleaseAndGetAddressOf();

            RRETURN(ctl::do_query_interface<U>(*ppInterface, ctl::down_cast<T>(m_trackerReference.Get())));
        }

        template <class U>
        _Check_return_ HRESULT As(_Out_ ctl::ComPtr<U>* pOther) const
        {
            RRETURN(ctl::do_query_interface(*pOther->ReleaseAndGetAddressOf(), ctl::down_cast<T>(m_trackerReference.Get())));
        }

        template <class U>
        ctl::ComPtr<U> AsOrNull() const
        {
            ctl::ComPtr<U> spResult;

            IGNOREHR(As<U>(&spResult));
            return spResult;
        }

        void Clear()
        {
            m_trackerReference.Clear();
        }

    private:

        void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
        {
            m_trackerReference.ReferenceTrackerWalk(walkType);
        }

        T* GetNoMptStress() const
        {
            return ctl::down_cast<T>(m_trackerReference.GetNoMptStress());
        }

        TrackerTargetReference* GetTrackerReference()
        {
            return &m_trackerReference;
        }

    private:
        TrackerTargetReference m_trackerReference;

        friend class ctl::WeakReferenceSourceNoThreadId;
        friend class TrackerPtrWrapper;

        // List of exception classes, that keep doing the manual management of
        // Tracker pointers

        // This is the list of classes that one way or another are a collection
        // of tracker ptr, they will keep managing those with the old system.
        template <typename T>
        friend class TrackerPtrVector;

        template <typename T>
        friend class TrackerPropertySet;

        template <typename T>
        friend class TrackerUniquePtr;

        // The following (sub)classes are not deriving from WeakReferenceSourceNoThreadId yet
        friend class ItemContainerGenerator;
        friend class EffectiveValueEntry;
    };

    // A vector of TrackerPtr instances
    template <typename T>
    class TrackerPtrVector
    {
    public:

        TrackerPtrVector() { }

        _Check_return_ HRESULT GetAt(_In_ UINT index, _Outptr_ T** item)
        {
            IFCCHECK_RETURN(index < m_items.size());

            *item = m_items[index].Get();
            AddRefInterface(*item);

            return S_OK;
        }

        // Get the reference tracker out of the value (if it's a reference tracker).
        // This was QI'd during the Set, but might be pointing to a GC'd object now, so it can't even be QI'd.
        // Note that this is raw, not AddRef'd
        _Check_return_ HRESULT GetAsReferenceTrackerUnsafe(_In_ UINT index, _Outptr_ xaml_hosting::IReferenceTrackerInternal** itemNoRef)
        {
            IFCCHECK_RETURN(index < m_items.size());

            *itemNoRef = m_items[index].GetAsReferenceTrackerUnsafe();

            return S_OK;
        }

        _Check_return_ HRESULT GetBack(_Outptr_ T** item)
        {
            IFCCHECK_RETURN( m_items.size() > 0 );

            *item = m_items.back().Get();
            AddRefInterface(*item);

            return S_OK;
        }
        _Check_return_ HRESULT SetAt(_In_ UINT index, _In_ T* item)
        {
            IFCCHECK_RETURN(index < m_items.size());

            // No need to take the lock since we're not really
            // modifying the collection, only the TrackerPtr stored in it
            m_items[index].Set(item);

            return S_OK;
        }

        _Check_return_ HRESULT InsertAt(_In_ UINT index, _In_ T* item)
        {
            TrackerPtr<T> tpItem;

            IFCEXPECTRC_RETURN(index <= m_items.size(), E_BOUNDS);

            tpItem.Set(item);

            // To modify the collection we need to take the lock
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                // We move the item into the vector, this SHOULD not require
                // callouts while the lock is taken
                m_items.insert(m_items.begin() + index, std::move(tpItem));
            }

            return S_OK;
        }


        void Append(_In_ T* item
            #if DBG
            , bool bStatic = false
            #endif
            )
        {
            TrackerPtr<T> tpItem;

            #if DBG
            tpItem.Set(item, bStatic);
            #else
            tpItem.Set(item);
            #endif

            // To modify the collection we need to take the lock
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                // We move the item into the vector, this SHOULD not require
                // callouts while the lock is taken
                m_items.push_back(std::move(tpItem));
            }
        }

        _Check_return_ HRESULT RemoveAt(_In_ UINT index)
        {
            TrackerPtr<T> tpItem;

            IFCCHECK_RETURN(index < m_items.size());

            // We first move the item OUT of the vector without
            // taking the lock, since we're not modifying the collection yet
            // The move operation will take the lock on its own but will
            // not do any call outs
            tpItem = std::move(m_items[index]);

            // To modify the collection we need to take the lock
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                m_items.erase(m_items.begin() + index);
            }

            return S_OK;
        }

        _Check_return_ HRESULT Remove(_In_ T* item)
        {
            HRESULT hr = S_OK;
            TrackerPtr<T> tpToRemove;
            auto handlerToRemove = std::find_if(m_items.begin(), m_items.end(),
                [item](TrackerPtr<T>& tracker) { return tracker.Get() == item; });

            if (handlerToRemove != m_items.end())
            {
                // Don't update while the ReferenceTrackerManager is running
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                // Move the TrackerPtr<T> out of the collection
                tpToRemove = std::move(*handlerToRemove);
                m_items.erase(handlerToRemove);
            }

            // Let the tpToRemove be destructed here, it will release the object
            // outside the lock

            RRETURN(hr);//RRETURN_REMOVAL
        }

        void Clear()
        {
            // First release each tracker within the collection without taking the lock
            // reach individual Clear operation will take it
            std::for_each(m_items.begin(), m_items.end(),
                [](TrackerPtr<T>& tracker) { tracker.Clear(); });

            // To modify the collection we need to take the lock
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

                m_items.clear();
            }
        }

        typename std::vector<TrackerPtr<T>>::const_iterator Begin() const
        {
            return m_items.cbegin();
        }

        typename std::vector<TrackerPtr<T>>::const_iterator End() const
        {
            return m_items.cend();
        }

        UINT Size() const
        {
            return static_cast<UINT>(m_items.size());
        }

        bool Empty() const
        {
            return m_items.empty();
        }

        void ReferenceTrackerWalk( _In_ EReferenceTrackerWalkType walkType)
        {
            std::for_each(m_items.begin(), m_items.end(),
                [walkType](TrackerPtr<T>& tracker) { tracker.ReferenceTrackerWalk(walkType); });
        }

    private:

        TrackerPtrVector(const TrackerPtrVector& other);

    private:

        std::vector<TrackerPtr<T>> m_items;
    };

    template <typename T>
    class TrackerPropertySet
    {
    public:

        ~TrackerPropertySet()
        {
            Clear();
        }

        _Check_return_ HRESULT HasKey(_In_ HSTRING key, _Out_ BOOLEAN *pfFound)
        {
            HRESULT hr = S_OK;
            LPCWSTR szKey = NULL;

            szKey = HStringUtil::GetRawBuffer(key, nullptr);

            {
                auto itr = m_items.find(szKey);
                *pfFound = (itr != m_items.end()) ? TRUE : FALSE;
            }

            RRETURN(hr);
        }

        _Check_return_ HRESULT Lookup(_In_ HSTRING key, _Outptr_ IInspectable **ppValue)
        {
            HRESULT hr = S_OK;
            LPCWSTR szKey = NULL;

            szKey = HStringUtil::GetRawBuffer(key, nullptr);

            *ppValue = NULL;

            {
                auto itr = m_items.find(szKey);
                if (itr != m_items.end())
                {
                    *ppValue = itr->second.Get();
                    AddRefInterface(*ppValue);
                }
            }

            RRETURN(hr);
        }

        _Check_return_ HRESULT Insert(_In_ HSTRING key, _In_ IInspectable *pValue, BOOLEAN *pfWasReplaced)
        {
            HRESULT hr = S_OK;
            LPCWSTR szKey = NULL;

            *pfWasReplaced = FALSE;

            szKey = HStringUtil::GetRawBuffer(key, nullptr);

            {
                auto itr = m_items.find(szKey);
                if (itr != m_items.end())
                {
                    *pfWasReplaced = TRUE;
                    itr->second.Set(pValue);
                }
                else
                {
                    std::pair<std::wstring, TrackerPtr<T>> entry;
                    entry.first = szKey;
                    entry.second.Set(pValue);

                    // Adding the entry into the map needs to be protected by the lock
                    {
                        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
                        m_items.insert(std::move(entry));
                    }
                }
            }

            RRETURN(hr); // RRETURN_REMOVAL
        }

        _Check_return_ HRESULT Remove(_In_ HSTRING key)
        {
            LPCWSTR szKey = NULL;

            szKey = HStringUtil::GetRawBuffer(key, nullptr);

            {
                auto itr = m_items.find(szKey);
                if (itr == m_items.end())
                {
                    return S_OK;
                }

                itr->second.Clear();

                // Removing the entry from the property set needs to be done under the protection of the lock
                {
                    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
                    m_items.erase(itr);
                }
            }

            return S_OK;
        }

        _Check_return_ HRESULT Size(_Out_ UINT *size)
        {
            *size = m_items.size();
            RRETURN(S_OK);
        }

        void Clear()
        {
            std::for_each(m_items.begin(), m_items.end(),
                [](std::pair<const std::wstring, TrackerPtr<T>>& entry) {
                entry.second.Clear();
            });

            // Actually clearing the property set needs to be done under the
            // protection of the lock
            {
                AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
                m_items.clear();
            }
        }

        void ReferenceTrackerWalk( _In_ EReferenceTrackerWalkType walkType)
        {
            std::for_each(m_items.begin(), m_items.end(),
                [walkType](std::pair<const std::wstring, TrackerPtr<T>>& entry) {
                    entry.second.ReferenceTrackerWalk(walkType);
            });
        }

    private:

        std::map<std::wstring, TrackerPtr<T>> m_items;
    };

    template <typename T>
    class TrackerUniquePtr
    {
    public:

        TrackerUniquePtr()
        { }

        TrackerUniquePtr(TrackerUniquePtr&& other)
        {
            *this = std::move(other);
        }

        T* operator-> () const
        {
            return m_ptr.get();
        }

        TrackerUniquePtr& operator= (TrackerUniquePtr&& other)
        {
            m_ptr = std::move(other.m_ptr);
            return *this;
        }

        void Reset(T* ptr)
        {
            // Change the pointer under the lock
            AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

            m_ptr.reset(ptr);
        }

        T* Get() const
        {
            return m_ptr.get();
        }

        operator bool () const
        {
            return !!m_ptr;
        }

        void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType)
        {
            if (m_ptr)
            {
                m_ptr->ReferenceTrackerWalk(walkType);
            }
        }

    private:

        // This object cannot be copied
        TrackerUniquePtr(const TrackerUniquePtr& other);
        TrackerUniquePtr& operator= (const TrackerUniquePtr& other);

    private:

        std::unique_ptr<T> m_ptr;
    };

}

