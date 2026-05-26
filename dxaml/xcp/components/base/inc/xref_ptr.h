// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// xref_ptr is a smart pointer to manage ref count of pointers to core objects
// that inherit from IObject.

#include <functional>

template<typename Ty> class xref_ptr
{
public:
    template <typename U> friend class xref_ptr;
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> static_sp_cast(_In_ const xref_ptr<TRhs>& r);
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> static_sp_cast(_Inout_ xref_ptr<TRhs>&& r);
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> reinterpret_sp_cast(_In_ const xref_ptr<TRhs>& r);
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> reinterpret_sp_cast(_Inout_ xref_ptr<TRhs>&& r);
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> const_sp_cast(_In_ const xref_ptr<TRhs>& r);
    template<class TLhs, class TRhs> friend xref_ptr<TLhs> const_sp_cast(_Inout_ xref_ptr<TRhs>&& r);
    template<class TLhs, class TRhs> friend bool operator==(const xref_ptr<TLhs>& a, const xref_ptr<TRhs>& b);
    template<class TLhs, class TRhs> friend bool operator!=(const xref_ptr<TLhs>& a, const xref_ptr<TRhs>& b);

    typedef Ty element_type;

    xref_ptr()
        : m_ptr(nullptr)
    {}

    xref_ptr(std::nullptr_t)
        : m_ptr(nullptr)
    {}

    template<typename Other> explicit xref_ptr(_In_opt_ Other* ptr)
        : m_ptr(ptr)
    {
        IncrementRefCount();
    }

    template<typename Other> xref_ptr(const xref_ptr<Other>& rhs)
        : m_ptr(rhs.m_ptr)
    {
        IncrementRefCount();
    }

    template<typename Other> xref_ptr(xref_ptr<Other>&& rhs)
        : m_ptr(rhs.detach())
    {
    }

    xref_ptr(_In_ const xref_ptr<Ty>& rhs)
        : m_ptr(rhs.m_ptr)
    {
        IncrementRefCount();
    }

    xref_ptr(xref_ptr<Ty>&& rhs) noexcept
        : m_ptr(rhs.detach())
    {
    }

    ~xref_ptr()
    {
        DecrementRefCount();
    }

    xref_ptr<Ty>& operator=(_In_ const xref_ptr<Ty>& rhs)
    {
        if (static_cast<const void*>(this) != static_cast<const void*>(&rhs))
        {
            Ty* temp = this->m_ptr;
            this->m_ptr = rhs.m_ptr;
            if (temp != nullptr)
            {
                temp->Release();
            }
            this->IncrementRefCount();
        }

        return *this;
    }

    xref_ptr<Ty>& operator=(_In_opt_ Ty* ptr)
    {
        if (m_ptr != ptr)
        {
            Ty* temp = this->m_ptr;
            this->m_ptr = ptr;
            if (temp != nullptr)
            {
                temp->Release();
            }
            this->IncrementRefCount();
        }

        return *this;
    }

    xref_ptr<Ty>& operator=(xref_ptr<Ty>&& rhs) noexcept
    {
        if (this != &rhs)
        {
            this->attach(rhs.detach());
        }
        return *this;
    }

    template<class Other> xref_ptr& operator=(_In_ const xref_ptr<Other>& rhs)
    {
        if (static_cast<const void*>(this) != static_cast<const void*>(&rhs))
        {
            Ty* temp = this->m_ptr;
            this->m_ptr = rhs.m_ptr;
            if (temp != nullptr)
            {
                temp->Release();
            }
            this->IncrementRefCount();
        }

        return *this;
    }

    void reset()
    {
        this->DecrementRefCount();
        this->m_ptr = nullptr;
    }

    template<class Other> HRESULT reset(_In_opt_ Other* ptr)
    {
        Ty* temp = this->m_ptr;
        this->m_ptr = ptr;
        if (temp != nullptr)
        {
            temp->Release();
        }
        this->IncrementRefCount();

        RRETURN(S_OK);
    }

    // FUTURE: This function is completely pointless now (because failed 'new' operations
    // are going to result in a failfast), so it should be removed and all callsites replaced with
    // attach()
    template<class Other> HRESULT init(_In_opt_ Other* ptr)
    {
        // This is a special function not on the original interface
        // It's the same as attach, except won't take NULL.
        // If you pass NULL it will return E_OUTOFMEMORY
        //
        // This means we can avoid having a naked pointer in the
        // creator, and still keep the functionality of reset.
        //
        // It also doesn't addref on entry, since it is assumed you will
        // pass in a freshly new'd object that already has ref=1.
        Ty* temp = this->m_ptr;
        this->m_ptr = ptr;
        if (temp != nullptr)
        {
            temp->Release();
        }

        return S_OK;
    }

    // Attach to a pointer that already has a ref added to it. Useful for capturing
    // some of our awkward APIs that return a raw pointer with a ref added to it already.
    template<class Other> void attach(_In_ Other* ptr)
    {
        Ty* temp = this->m_ptr;
        this->m_ptr = ptr;
        if (temp != nullptr)
        {
            temp->Release();
        }
    }

    Ty** ReleaseAndGetAddressOf()
    {
        this->DecrementRefCount();
        return &(this->m_ptr);
    }

    Ty* detach()
    {
        Ty* ptr = this->m_ptr;
        this->m_ptr = nullptr;
        return ptr;
    }

    Ty* get() const
    {
        return m_ptr;
    }

    // Copy to pointer of same type as this - simple addref and copy
    template<typename T>
    typename std::enable_if<std::is_same<Ty, T>::value, void>::type
        CopyTo(_Outptr_ T** ptr)
    {
        this->IncrementRefCount();
        *ptr = this->m_ptr;
    }

    Ty& operator*() const               { ASSERT(this->m_ptr != nullptr); return *this->m_ptr;   }
    Ty* operator->() const              { return this->m_ptr;                                    }
    operator bool() const               { return this->m_ptr != nullptr;                         }
    operator Ty*() const                { return this->m_ptr;                                    }

private:
    void DecrementRefCount()
    {
        Ty* temp = this->m_ptr;
        if (temp != nullptr)
        {
            this->m_ptr = nullptr;
            temp->Release();
        }
    }

    void IncrementRefCount()
    {
        if (m_ptr != nullptr)
        {
            m_ptr->AddRef();
        }
    }

private:
    Ty* m_ptr;
};

template <typename Ty,
    typename... Types > xref_ptr<Ty> make_xref(Types&&... Args)
{
    Ty* ptr = new Ty(std::forward<Types>(Args)...);
    xref_ptr<Ty> result;
    result.init(ptr);
    return result;
}

template<class TLhs, class TRhs>
bool operator==(const xref_ptr<TLhs>& a, const xref_ptr<TRhs>& b)
{
    return a.m_ptr == b.m_ptr;
}

// == operator that std collections will use
template<typename T>
bool operator ==(const xref_ptr<T>& lhs, const xref_ptr<T>& rhs)
{
    return (lhs.get() == rhs.get());
}

template<class TLhs, class TRhs>
bool operator!=(const xref_ptr<TLhs>& a, const xref_ptr<TRhs>& b)
{
    return a.m_ptr != b.m_ptr;
}

template<class TCastTo, class TCastFrom>
 xref_ptr<TCastTo> static_sp_cast(const xref_ptr<TCastFrom>& r)
{
    return xref_ptr<TCastTo>(static_cast<TCastTo*>(r.m_ptr));
}

template<class TCastTo, class TCastFrom>
xref_ptr<TCastTo> static_sp_cast(xref_ptr<TCastFrom>&& r)
{
    xref_ptr<TCastTo> result;
    result.attach(static_cast<TCastTo*>(r.detach()));
    return result;
}

template<class TCastTo, class TCastFrom>
xref_ptr<TCastTo> reinterpret_sp_cast(const xref_ptr<TCastFrom>& r)
{
    return xref_ptr<TCastTo>(reinterpret_cast<TCastTo*>(r.m_ptr));
}

template<class TCastTo, class TCastFrom>
xref_ptr<TCastTo> reinterpret_sp_cast(xref_ptr<TCastFrom>&& r)
{
    xref_ptr<TCastTo> result;
    result.attach(reinterpret_cast<TCastTo*>(r.detach()));
    return result;
}

template<class TCastTo, class TCastFrom>
xref_ptr<TCastTo> const_sp_cast(const xref_ptr<TCastFrom>& r)
{
    return xref_ptr<TCastTo>(const_cast<TCastTo*>(r.m_ptr));
}

template<class TCastTo, class TCastFrom>
xref_ptr<TCastTo> const_sp_cast(xref_ptr<TCastFrom>&& r)
{
    xref_ptr<TCastTo> result;
    result.attach(const_cast<TCastTo*>(r.detach()));
    return result;
}

// < operator allows xref_ptr's to be stored in std sorted associative collections (eg map)
template<typename T>
bool operator <(const xref_ptr<T>& lhs, const xref_ptr<T>& rhs)
{
    return (lhs.get() < rhs.get());
}

// hash implementation allows xref_ptr's to be stored in std hash collections (eg unordered_set)
namespace std {
    template <typename T>
    struct hash<xref_ptr<T>>
    {
        std::size_t operator()(const xref_ptr<T>& sp) const
        {
            return std::hash<T*>()(sp.get());
        }
    };
}

