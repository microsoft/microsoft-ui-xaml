// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <memory>
#include <type_traits>

// class TlsProviderHelper
// type-safe and RAII management of an object that resides in thread-local storage
// Typical usage is to help the TlsProvider class, which automatically determines
// a new Tls ID per type.
template<typename TlsType>
class TlsProviderHelper
{
    static_assert(::std::is_base_of<::std::enable_shared_from_this<TlsType>, TlsType>::value,
        "TlsType must inherit from std::enable_shared_from_this!");
public:
    static ::std::shared_ptr<TlsType> GetWrappedObject(DWORD tlsIndex)
    {
        auto pObject = GetRawObject(tlsIndex);
        if (pObject)
            return pObject->shared_from_this();
        else
            return nullptr;
    }

    template<typename... Types>
    static ::std::shared_ptr<TlsType> CreateWrappedObject(DWORD tlsIndex, Types&&... args)
    {
        auto pObject = GetRawObject(tlsIndex);

        // We don't support overwriting an existing object with a new one.
        // If you want that functionality, wrap it into your provided TlsType!
        FAIL_FAST_ASSERT(!pObject);

        if (!pObject)
        {
            return WrapRawObject(
                tlsIndex,
                CreateRawObject(tlsIndex, ::std::forward<Types>(args)...));
        }
        else
        {
            return GetWrappedObject(tlsIndex);
        }
    }

    template<typename Alloc, typename... Types>
    static ::std::shared_ptr<TlsType> CreateWrappedObjectWithAllocator(DWORD tlsIndex, const Alloc& alloc, Types&&... args)
    {
        auto pObject = GetRawObject(tlsIndex);

        // We don't support overwriting an existing object with a new one.
        // If you want that functionality, wrap it into your provided TlsType!
        FAIL_FAST_ASSERT(!pObject);

        if (!pObject)
        {
            return WrapRawObjectWithAllocator(
                tlsIndex,
                CreateRawObject(tlsIndex, ::std::forward<Types>(args)...),
                alloc);
        }
        else
        {
            return GetWrappedObject(tlsIndex);
        }
    }

private:

    struct TlsDeleter
    {
        explicit TlsDeleter(DWORD tlsIndex) : m_tlsIndex(tlsIndex) {}
        void operator()(TlsType* pObject) const
        {
            pObject->~TlsType();
            DeallocateStorage(m_tlsIndex, pObject);
        }
        static void DeallocateStorage(DWORD tlsIndex, TlsType* pStorage)
        {
            if (!::TlsSetValue(tlsIndex, nullptr)) {
                FAIL_FAST_ASSERT(false);
            }
            if (::LocalFree(pStorage)) {
                FAIL_FAST_ASSERT(false);
            }
        }

        DWORD m_tlsIndex;
    };

    static TlsType* GetRawObject(DWORD tlsIndex)
    {
        return static_cast<TlsType*>(::TlsGetValue(tlsIndex));
    }

    template <typename... Types>
    static TlsType* CreateRawObject(DWORD tlsIndex, Types&&... args)
    {
        void* pStorage = ::LocalAlloc(LPTR, sizeof(TlsType));
        FAIL_FAST_ASSERT(pStorage);
        if (!::TlsSetValue(tlsIndex, pStorage))
        {
            FAIL_FAST_ASSERT(false);
        }
        new(pStorage) TlsType(::std::forward<Types>(args)...);
        return static_cast<TlsType*>(pStorage);
    }

    static ::std::shared_ptr<TlsType> WrapRawObject(DWORD tlsIndex, TlsType* pObject)
    {
        if (pObject)
        {
            return ::std::shared_ptr<TlsType>(pObject, TlsDeleter(tlsIndex));
        }
        return nullptr;
    }

    template <typename Alloc>
    static ::std::shared_ptr<TlsType> WrapRawObjectWithAllocator(DWORD tlsIndex, TlsType* pObject, const Alloc& alloc)
    {
        if (pObject)
        {
            return ::std::shared_ptr<TlsType>(pObject, TlsDeleter(tlsIndex), alloc);
        }
        return nullptr;
    }
};

// class TlsProvider
// A simplified wrapper around TlsProviderWrapper that allocates a new tls index on a per-type basis
template <typename TlsType>
class TlsProvider
{
    static_assert(::std::is_base_of<::std::enable_shared_from_this<TlsType>, TlsType>::value,
        "TlsType must inherit from std::enable_shared_from_this!");
public:
    static ::std::shared_ptr<TlsType> GetWrappedObject()
    {
        return TlsProviderHelper<TlsType>::GetWrappedObject(GetTlsIndex());
    }

    template <typename... Types>
    static ::std::shared_ptr<TlsType> CreateWrappedObject(Types&&... args)
    {
        return TlsProviderHelper<TlsType>::CreateWrappedObject(GetTlsIndex(), ::std::forward<Types>(args)...);
    }

    // Overload to provide an allocator with which to allocate shared_ptr's ref count storage
    template <typename Alloc, typename... Types>
    static ::std::shared_ptr<TlsType> CreateWrappedObjectWithAllocator(const Alloc& alloc, Types&&... args)
    {
        return TlsProviderHelper2<TlsType>::CreateWrappedObjectWithAllocator(GetTlsIndex(), alloc, ::std::forward<Types>(args)...);
    }

private:
    static DWORD GetTlsIndex()
    {
        // Each TlsProvider of a given TlsType will get its own TlsIndex
        static const DWORD tlsIndex = ::TlsAlloc();
        FAIL_FAST_ASSERT(tlsIndex != TLS_OUT_OF_INDEXES);
        return tlsIndex;
    }
};
