// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A smart pointer to facilitate the usage of Modern COM objects.  The
// implementation was gently ported from the Windows Runtime Library ComPtr.
//
// Code in the ctl::Internal namespace is intended for internal usage within the
// ComPtr code.

#pragma once

#include "ComEventHandlerTraits.h"
#include "StaticAssertFalse.h"
#include <ComUtils.h>
#include <corerror.h>
#include <mindebug.h>
#define XNOTHROW __declspec(nothrow)

namespace ctl
{
    class WeakRefPtr;

    template<typename T>
    _Check_return_ HRESULT AsWeak(
        _In_ T* p,
        _Out_ WeakRefPtr* pWeak);

    namespace Internal
    {
        struct BoolStruct
        {
            int Member;
        };

        typedef int BoolStruct::* BoolType;

        template <bool b, typename T = void>
        struct EnableIf
        {
        };

        template <typename T>
        struct EnableIf<true, T>
        {
            typedef T type;
        };

        template <typename T1, typename T2>
        struct IsSame
        {
            static const bool value = false;
        };

        template <typename T1>
        struct IsSame<T1, T1>
        {
            static const bool value = true;
        };

        template<class T>
        struct RemoveReference
        {
            typedef T Type;
        };

        template<class T>
        struct RemoveReference<T&>
        {
            typedef T Type;
        };

        template<class T>
        struct RemoveReference<T&&>
        {
            typedef T Type;
        };

        template<class T>
        XNOTHROW inline typename RemoveReference<T>::Type&& Move(
            _Inout_ T&& arg)
        {
            return ((typename RemoveReference<T>::Type&&)arg);
        }

        template<class T>
        XNOTHROW inline void Swap(
            _Inout_ T& left,
            _Inout_ T& right)
        {
            T tmp = Move(left);
            left = Move(right);
            right = Move(tmp);
        }

        // Disables template argument deduction from Forward helper
        template <class T>
        struct Identity
        {
            // Map T to type unchanged
            typedef T Type;
        };

        template<class T>
        inline T&& Forward(
            typename Identity<T>::Type& arg)
        {
            // Forward arg, given explicitly specified Type parameter
            return (T&&)arg;
        }

        template <typename Base, typename Derived>
        struct IsBaseOfStrict
        {
            static const bool value = __is_base_of(Base, Derived);
        };

        template <typename Base>
        struct IsBaseOfStrict<Base, Base>
        {
            static const bool value = false;
        };

        // Helper object that makes IUnknown methods private
        template <typename T>
        class RemoveIUnknownBase : public T
        {
        private:
            // STDMETHOD macro implies virtual.
            // ComPtr can be used with any class that implements the 3 methods of IUnknown.
            // ComPtr does not require these methods to be virtual.
            // When ComPtr is used with a class without a virtual table, marking the functions
            // as virtual in this class adds unnecessary overhead.
            HRESULT __stdcall QueryInterface(REFIID riid, _COM_Outptr_ void **ppvObject) override;
            ULONG __stdcall AddRef() override;
            ULONG __stdcall Release() override;
        };

        template<typename T>
        struct RemoveIUnknown
        {
            typedef RemoveIUnknownBase<T> ReturnType;
        };

        template<typename T>
        struct RemoveIUnknown<const T>
        {
            typedef const RemoveIUnknownBase<T> ReturnType;
        };

        template <typename T>
        class WeakReferenceInterface
            : public T
        {
            private:
                _Check_return_ HRESULT __stdcall QueryInterface(
                    _In_ REFIID riid,
                    _Outptr_ void **ppvObject);
                ULONG __stdcall AddRef();
                ULONG __stdcall Release();
                _Check_return_ HRESULT __stdcall Resolve(
                    _In_ REFIID riid,
                    _Outptr_ void **ppvObject);
        };

        template <typename T>
        class ComPtrRef
        {
            T* ptr_;

        public:
            using InterfaceType = typename T::InterfaceType;

            ComPtrRef(
                _In_opt_ T* ptr)
            {
                ptr_ = ptr;
            }

            // Conversion operators
            operator IInspectable**() const
            {
                static_assert(__is_base_of(IInspectable, InterfaceType), "Invalid cast: InterfaceType does not derive from IInspectable");
                return reinterpret_cast<IInspectable**>(ptr_->ReleaseAndGetAddressOf());
            }

            operator IUnknown**() const
            {
                static_assert(__is_base_of(IUnknown, InterfaceType), "Invalid cast: InterfaceType does not derive from IUnknown");
                return reinterpret_cast<IUnknown**>(ptr_->ReleaseAndGetAddressOf());
            }

            operator void**() const
            {
                return reinterpret_cast<void**>(ptr_->ReleaseAndGetAddressOf());
            }

            // This is our operator ComPtr<U> (or the latest derived class
            // from ComPtr (e.g. WeakRefPtr))
            operator T*()
            {
                *ptr_ = nullptr;
                return ptr_;
            }

            // We define operator InterfaceType**() here instead of on
            // ComPtrRefBase<T>, since if InterfaceType is IUnknown or
            // IInspectable, having it on the base will colide.
            operator InterfaceType**()
            {
                return ptr_->ReleaseAndGetAddressOf();
            }

            // This is used for IID_PPV_ARGS in order to do
            // __uuidof(**(ppType)).  It does not need to clear  ptr_ at
            // this point, it is done at IID_PPV_ARGS_Helper(ComPtrRef&)
            // later in this file.
            InterfaceType* operator *()
            {
                return ptr_->Get();
            }

            // Explicit functions
            InterfaceType* const * GetAddressOf() const
            {
                return ptr_->GetAddressOf();
            }

            InterfaceType** ReleaseAndGetAddressOf()
            {
                return ptr_->ReleaseAndGetAddressOf();
            }
        };
    }

    template <typename T>
    class ComPtr
    {
        public:
            typedef T InterfaceType;

        protected:
            InterfaceType *ptr_;
            template<class U> friend class ComPtr;

            XNOTHROW void InternalAddRef()
            {
                if (ptr_ != nullptr)
                {
                    iunknown_cast(ptr_)->AddRef();
                }
            }

            XNOTHROW void InternalRelease()
            {
                T* temp = ptr_;
                if (temp != nullptr)
                {
                    ptr_ = nullptr;
                    iunknown_cast(temp)->Release();
                }
            }

        public:
            XNOTHROW ComPtr()
                : ptr_(nullptr)
            {
                XCP_STRONG(&ptr_);
            }

            XNOTHROW ComPtr(
                decltype(__nullptr))
                : ptr_(nullptr)
            {
                XCP_STRONG(&ptr_);
            }

            template<class U>
            XNOTHROW ComPtr(
                _In_opt_ U *other)
                : ptr_(other)
            {
                XCP_STRONG(&ptr_);
                InternalAddRef();
            }

            XNOTHROW ComPtr(
                const ComPtr& other)
                : ptr_(other.ptr_)
            {
                XCP_STRONG(&ptr_);
                InternalAddRef();
            }

            // Copy ctor that allows to instanatiate class when U* is
            // convertible to T*
            template<class U>
            XNOTHROW ComPtr(
                const ComPtr<U> &other,
                typename Internal::EnableIf<__is_convertible_to(U*, T*), void *>::type * = 0)
                : ptr_(other.ptr_)
            {
                XCP_STRONG(&ptr_);
                InternalAddRef();
            }

            XNOTHROW ComPtr(
                _Inout_ ComPtr &&other)
                : ptr_(nullptr)
            {
                XCP_STRONG(&ptr_);
                if (this != reinterpret_cast<ComPtr*>(&reinterpret_cast<byte&>(other)))
                {
                    Swap(other);
                }
            }

            // Move ctor that allows instantiation of a class when U* is convertible to T*
            template<class U>
            XNOTHROW ComPtr(
                _Inout_ ComPtr<U>&& other,
                typename Internal::EnableIf<__is_convertible_to(U*, T*), void *>::type * = 0)
                : ptr_(other.ptr_)
            {
                XCP_STRONG(&ptr_);
                other.ptr_ = nullptr;
            }

            XNOTHROW ~ComPtr()
            {
                InternalRelease();
            }

            XNOTHROW ComPtr& operator=(
                decltype(__nullptr))
            {
                InternalRelease();
                return *this;
            }

            XNOTHROW ComPtr& operator=(
                _In_opt_ T *other)
            {
                if (ptr_ != other)
                {
                    ComPtr(other).Swap(*this);
                }
                return *this;
            }

            template <typename U>
            XNOTHROW ComPtr& operator=(
                _In_opt_ U *other)
            {
                ComPtr(other).Swap(*this);
                return *this;
            }

            XNOTHROW ComPtr& operator=(
                const ComPtr &other)
            {
                if (ptr_ != other.ptr_)
                {
                    ComPtr(other).Swap(*this);
                }
                return *this;
            }

            template<class U>
            XNOTHROW ComPtr& operator=(
                const ComPtr<U>& other)
            {
                ComPtr(other).Swap(*this);
                return *this;
            }

            XNOTHROW ComPtr& operator=(
                _Inout_ ComPtr &&other)
            {
                ComPtr(static_cast<ComPtr&&>(other)).Swap(*this);
                return *this;
            }

            template<class U>
            XNOTHROW ComPtr& operator=(
                _Inout_ ComPtr<U>&& other)
            {
                ComPtr(static_cast<ComPtr<U>&&>(other)).Swap(*this);
                return *this;
            }

            XNOTHROW void Swap(
                _Inout_ ComPtr&& r)
            {
                T* tmp = ptr_;
                ptr_ = r.ptr_;
                r.ptr_ = tmp;
            }

            XNOTHROW void Swap(
                _Inout_ ComPtr& r)
            {
                T* tmp = ptr_;
                ptr_ = r.ptr_;
                r.ptr_ = tmp;
            }

            XNOTHROW operator Internal::BoolType() const
            {
                return Get() != nullptr ?
                    &Internal::BoolStruct::Member :
                    nullptr;
            }

            XNOTHROW T* Get() const
            {
                static_assert(!ctl::IsEventPtrCompatible<T>::value, "ComPtr cannot be used to keep references to event handlers, use EventPtr");
                static_assert(!ctl::IsWeakEventPtrCompatible<T>::value, "ComPtr cannot be used to keep references to weak event handlers, use WeakEventPtr");
                return ptr_;
            }

            template<typename U>
            XNOTHROW U* Cast() const
            {
                return static_cast<U*>(Get());
            }

            template <typename U>
            XNOTHROW void CastTo(U** casted) const
            {
                *casted = static_cast<U *>(Get());
            }

            XNOTHROW typename Internal::RemoveIUnknown<InterfaceType>::ReturnType* operator->() const
            {
                return static_cast<typename Internal::RemoveIUnknown<InterfaceType>::ReturnType*>(ptr_);
            }

#ifdef FOR_INTELLISENSE_ONLY
            // This is never defined and never compiled but helps IntelliSense work
            T* operator->() const;
#endif

            Internal::ComPtrRef<ComPtr<T>> operator&()
            {
                return Internal::ComPtrRef<ComPtr<T>>(this);
            }

            const Internal::ComPtrRef<const ComPtr<T>> operator&() const
            {
                return Internal::ComPtrRef<const ComPtr<T>>(this);
            }

            XNOTHROW T* const* GetAddressOf() const
            {
                return &ptr_;
            }

            XNOTHROW T** GetAddressOf()
            {
                return &ptr_;
            }

            XNOTHROW T** ReleaseAndGetAddressOf()
            {
                InternalRelease();
                return &ptr_;
            }

            XNOTHROW T* Detach()
            {
                T* ptr = ptr_;
                ptr_ = nullptr;
                return ptr;
            }

            XNOTHROW void Attach(
                _In_opt_ InterfaceType* other)
            {
                if (ptr_ != other)
                {
                    InternalRelease();
                    ptr_ = other;
                }
            }

            XNOTHROW void Reset()
            {
                InternalRelease();
            }

            // Copy to pointer of same type as this - simple addref and copy
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<std::is_same<T, U>::value, HRESULT>::type
                CopyTo(_Outptr_ U** ptr)
            {
                InternalAddRef();
                *ptr = ptr_;
                return S_OK;
            }

            // Move to pointer of same type as this - simple copy and detach
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<std::is_same<T, U>::value, HRESULT>::type
                MoveTo(_Outptr_ U** ptr)
            {
                *ptr = Detach();
                return S_OK;
            }

            // Copy to pointer to IInspectable - simple addref and copy with a cast to IInspectable (to disambiguate diamond inheritance)
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && std::is_same<IInspectable, U>::value, HRESULT>::type
                CopyTo(_Outptr_ U** ptr)
            {
                InternalAddRef();
                *ptr = iinspectable_cast(ptr_);
                return S_OK;
            }

            // Move to pointer to IInspectable - simple copy and detach with a cast to IInspectable (to disambiguate diamond inheritance)
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && std::is_same<IInspectable, U>::value, HRESULT>::type
                MoveTo(_Outptr_ U** ptr)
            {
                *ptr = iinspectable_cast(Detach());
                return S_OK;
            }

            // Copy to pointer of parent (implicitly convertible) type - simple addref and copy
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && __is_convertible_to(T*, U*), HRESULT>::type
                CopyTo(_Outptr_ U** ptr)
            {
                InternalAddRef();
                *ptr = ptr_;
                return S_OK;
            }

            // Move to pointer of parent (implicitly convertible) type - simple copy and detach
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && __is_convertible_to(T*, U*), HRESULT>::type
                MoveTo(_Outptr_ U** ptr)
            {
                *ptr = Detach();
                return S_OK;
            }

            // Copy to other, possibly incompatible type - perform QI
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && !__is_convertible_to(T*, U*), HRESULT>::type
                CopyTo(_Outptr_ U** ptr) const
            {
                return ctl::do_query_interface(*ptr, iunknown_cast(ptr_));
            }

            // Move to other, possibly incompatible type - perform QI
            // DELIBERATELY UNSUPPORTED!
            template<typename U>
            XNOTHROW _Check_return_ typename std::enable_if<!std::is_same<T, U>::value && !std::is_same<IInspectable, U>::value && !__is_convertible_to(T*, U*), HRESULT>::type
                MoveTo(_Outptr_ U** ptr)
            {
                static_assert_false("ComPtr::MoveTo() is not supported when QueryInterface is required. Use CopyTo() instead.");
            }

            _Check_return_ HRESULT CopyTo(REFIID riid, _Outptr_result_nullonfailure_ void** ptr) const throw()
            {
                return iunknown_cast(ptr_)->QueryInterface(riid, ptr);
            }

            // query for U interface
            template<typename U>
            XNOTHROW _Check_return_ HRESULT As(
                Internal::ComPtrRef<ComPtr<U>> p)
                const
            {
                U **ppInterface = p.ReleaseAndGetAddressOf();

                return ctl::do_query_interface<U>(*ppInterface, iunknown_cast(ptr_));
            }

            // query for U interface
            template<typename U>
            XNOTHROW _Check_return_ HRESULT As(
                _Out_ ComPtr<U>* p)
                const
            {
                return ctl::do_query_interface<U>(*p->ReleaseAndGetAddressOf(), iunknown_cast(ptr_));
            }

            // query for U interface
            template<typename U>
            XNOTHROW ComPtr<U> AsOrNull()
                const
            {
                ComPtr<U> result = nullptr;
                IGNOREHR(As(&result));
                return result;
            }

            XNOTHROW _Check_return_ HRESULT AsWeak(
                _Out_ WeakRefPtr* pWeakRef)
            {
                return ctl::AsWeak(ptr_, pWeakRef);
            }
    };

    class WeakRefPtr
        : public ComPtr<IWeakReference>
    {
        protected:
            _Check_return_ HRESULT InternalResolve(
                _In_ REFIID riid,
                _Outptr_ IInspectable** inspectable)
            {
                HRESULT hr = S_OK;
                *inspectable = nullptr;

                if (ptr_ == nullptr)
                {
                    // Weak reference was already released
                    goto Cleanup;
                }

                // Resolve weak reference.
                // CLR WeakReference's Resolve currently fails with COR_E_INVALIDCOMOBJECT
                // if the weak reference target has been deleted, so handle that failure
                hr = ptr_->Resolve(riid, inspectable);
                ASSERT(SUCCEEDED(hr) || hr == COR_E_INVALIDCOMOBJECT);
                if (FAILED(hr) || *inspectable == nullptr)
                {
                    // Release weak reference because it will not succeed ever
                    InternalRelease();
                }

            Cleanup:
                return S_OK;
            }

        public:
            Internal::ComPtrRef<WeakRefPtr> operator&()
            {
                return Internal::ComPtrRef<WeakRefPtr>(this);
            }

            const Internal::ComPtrRef<const WeakRefPtr> operator&() const
            {
                return Internal::ComPtrRef<const WeakRefPtr>(this);
            }

            XNOTHROW WeakRefPtr()
                : ComPtr(nullptr)
            {
            }

            XNOTHROW WeakRefPtr(
                decltype(__nullptr))
                : ComPtr(nullptr)
            {
            }

            XNOTHROW WeakRefPtr(
                _In_opt_ IWeakReference* ptr)
                : ComPtr(ptr)
            {
            }

            XNOTHROW WeakRefPtr(
                const ComPtr<IWeakReference>& ptr)
                : ComPtr(ptr)
            {
            }

            XNOTHROW WeakRefPtr(
                const WeakRefPtr& ptr)
                : ComPtr(ptr)
            {
            }

            XNOTHROW WeakRefPtr(
                _Inout_ WeakRefPtr&& ptr)
                : ComPtr(static_cast<ComPtr<IWeakReference>&&>(ptr))
            {
            }

            XNOTHROW WeakRefPtr &operator=(const WeakRefPtr&) = default;

#if _MSC_VER >= 1900
            XNOTHROW WeakRefPtr &operator=(_Inout_ WeakRefPtr&&) = default;
#endif

            XNOTHROW ~WeakRefPtr()
            {
            }

            XNOTHROW Internal::WeakReferenceInterface<InterfaceType>* operator->() const
            {
                return reinterpret_cast<Internal::WeakReferenceInterface<InterfaceType>*>(ptr_);
            }

            // resolve U interface
            template<typename U>
            XNOTHROW _Check_return_ HRESULT As(
                Internal::ComPtrRef<ComPtr<U>> ptr)
            {
                static_assert(!Internal::IsSame<IWeakReference, U>::value, "IWeakReference cannot resolve IWeakReference object.");
                static_assert(__is_base_of(IInspectable, U), "WeakRefPtr::As() can only be used on types derived from IInspectable");

                return InternalResolve(__uuidof(U), ptr);
            }

            template<typename U>
            XNOTHROW _Check_return_ HRESULT As(
                _Out_ ComPtr<U>* ptr)
            {
                static_assert(!Internal::IsSame<IWeakReference, U>::value, "IWeakReference cannot resolve IWeakReference object.");
                static_assert(__is_base_of(IInspectable, U), "WeakRefPtr::As() can only be used on types derived from IInspectable");

                return InternalResolve(__uuidof(U), &(*ptr));
            }

            template<typename U>
            XNOTHROW ComPtr<U> AsOrNull()
            {
                ctl::ComPtr<U> result;
                IGNOREHR(As(&result));
                return result;
            }

            XNOTHROW _Check_return_ HRESULT AsIID(
                _In_ REFIID riid,
                _Out_ ComPtr<IInspectable>* ptr)
            {
                ASSERT(riid != __uuidof(IWeakReference));

                return InternalResolve(riid, ptr->ReleaseAndGetAddressOf());
            }

            XNOTHROW _Check_return_ HRESULT CopyTo(
                _In_ REFIID riid,
                _Outptr_ IInspectable** ptr)
            {
                ASSERT(riid != __uuidof(IWeakReference));

                return InternalResolve(riid, ptr);
            }

            template<typename U>
            XNOTHROW _Check_return_ HRESULT CopyTo(
                _Outptr_ U** ptr)
            {
                static_assert(__is_base_of(IInspectable, U), "WeakRefPtr::CopyTo() can only be used on types derived from IInspectable");
                return InternalResolve(__uuidof(U), reinterpret_cast<IInspectable**>(ptr));
            }

            XNOTHROW _Check_return_ HRESULT CopyTo(
                _Outptr_ IWeakReference** ptr)
            {
                InternalAddRef();
                *ptr = ptr_;
                return S_OK;
            }
    };

    template<typename T>
    _Check_return_ HRESULT AsWeak(
        _In_opt_ T* p,
        _Out_ WeakRefPtr* pWeak)
    {
        static_assert(!Internal::IsSame<IWeakReference,T>::value, "Cannot get IWeakReference object to IWeakReference.");

        ComPtr<IWeakReferenceSource> refSource;
        ComPtr<IWeakReference> weakref;

        if (p)
        {
            IFC_RETURN(iunknown_cast(p)->QueryInterface(__uuidof(IWeakReferenceSource), reinterpret_cast<void**>(refSource.GetAddressOf())));
            IFC_RETURN(refSource->GetWeakReference(weakref.GetAddressOf()));
        }

        *pWeak = WeakRefPtr(weakref);
        return S_OK;
    }

    template <typename T>
    _Check_return_ HRESULT AsWeakOrNull(
        _In_opt_ T* p,
        _Out_ WeakRefPtr* pWeak)
    {
        ComPtr<IWeakReferenceSource> spRefSource;
        ComPtr<IWeakReference> spWeakRef;

        spRefSource = query_interface_cast<IWeakReferenceSource>(p);
        if (spRefSource)
        {
            IFC_RETURN(spRefSource->GetWeakReference(&spWeakRef));
        }

        *pWeak = WeakRefPtr(spWeakRef);

        return S_OK;
    }

    // If pointer types are convertible, let the compiler calculate the correct offsets
    // This is really just a general case of identical types, but it's easier to read this way
    template<class T, class U>
    XNOTHROW typename std::enable_if<std::is_convertible<T*, U*>::value || std::is_convertible<U*, T*>::value, bool>::type
        operator==(
        const ComPtr<T>& a,
        const ComPtr<U>& b)
    {
        return a.Get() == b.Get();
    }

    // If they're not convertible, then either somebody is comparing interface pointers (use ctl::are_equal),
    // or comparing unrelated pointer types (let the compiler complain)
    template<class T, class U>
    XNOTHROW typename std::enable_if<!std::is_convertible<T*, U*>::value && !std::is_convertible<U*, T*>::value, bool>::type
        operator==(
        const ComPtr<T>& a,
        const ComPtr<U>& b)
    {
        bool result = false;
        IGNOREHR(ctl::are_equal(a.Get(), b.Get(), &result));
        return result;
    }

    template<class T>
    XNOTHROW bool operator==(
        const ComPtr<T>& a,
        decltype(__nullptr))
    {
        return a.Get() == nullptr;
    }

    template<class T>
    XNOTHROW bool operator==(
        decltype(__nullptr),
        const ComPtr<T>& a)
    {
        return a.Get() == nullptr;
    }

    // Just define this in terms of operator== and let this funnel into the correct operator== overload
    template<class T, class U>
    inline XNOTHROW bool operator!=(
        const ComPtr<T>& a,
        const ComPtr<U>& b)
    {
        return !(a == b);
    }

    template<class T>
    XNOTHROW bool operator!=(
        const ComPtr<T>& a,
        decltype(__nullptr))
    {
        return a.Get() != nullptr;
    }

    template<class T>
    XNOTHROW bool operator!=(
        decltype(__nullptr),
        const ComPtr<T>& a)
    {
        return a.Get() != nullptr;
    }

    // If pointer types are convertible, let the compiler calculate the correct offsets
    // This is really just a general case of identical types, but it's easier to read this way
    template<class T, class U>
    XNOTHROW bool operator<(
        const ComPtr<T>& a,
        const ComPtr<U>& b)
    {
        static_assert(std::is_convertible<T*, U*>::value || std::is_convertible<U*, T*>::value, "'T' and 'U' pointers must be comparable");
        return a.Get() < b.Get();
    }

    // ComPtrRef comparisons

    // Don't bother with QI-ing to IInspectable on these. Just assert on convertibility of pointer types
    template<class T, class U>
    XNOTHROW bool operator==(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        const Internal::ComPtrRef<ComPtr<U>>& b)
    {
        static_assert(std::is_convertible<T*, U*>::value || std::is_convertible<U*, T*>::value, "'T' and 'U' pointers must be comparable");
        return a.GetAddressOf() == b.GetAddressOf();
    }

    template<class T>
    XNOTHROW bool operator==(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        decltype(__nullptr))
    {
        return a.GetAddressOf() == nullptr;
    }

    template<class T>
    XNOTHROW bool operator==(
        decltype(__nullptr),
        const Internal::ComPtrRef<ComPtr<T>>& a)
    {
        return a.GetAddressOf() == nullptr;
    }

    template<class T>
    XNOTHROW bool operator==(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        void* b)
    {
        return a.GetAddressOf() == b;
    }

    template<class T>
    XNOTHROW bool operator==(
        void* b,
        const Internal::ComPtrRef<ComPtr<T>>& a)
    {
        return a.GetAddressOf() == b;
    }

    template<class T, class U>
    XNOTHROW bool operator!=(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        const Internal::ComPtrRef<ComPtr<U>>& b)
    {
        return !(a == b);
    }

    template<class T>
    XNOTHROW bool operator!=(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        decltype(__nullptr))
    {
        return a.GetAddressOf() != nullptr;
    }

    template<class T>
    XNOTHROW bool operator!=(
        decltype(__nullptr),
        const Internal::ComPtrRef<ComPtr<T>>& a)
    {
        return a.GetAddressOf() != nullptr;
    }

    template<class T>
    XNOTHROW bool operator!=(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        void* b)
    {
        return a.GetAddressOf() != b;
    }

    template<class T>
    XNOTHROW bool operator!=(
        void* b,
        const Internal::ComPtrRef<ComPtr<T>>& a)
    {
        return a.GetAddressOf() != b;
    }

    // Don't bother with QI-ing to IInspectable on these. Just assert on convertibility of pointer types
    template<class T, class U>
    XNOTHROW bool operator<(
        const Internal::ComPtrRef<ComPtr<T>>& a,
        const Internal::ComPtrRef<ComPtr<U>>& b)
    {
        static_assert(std::is_convertible<T*, U*>::value || std::is_convertible<U*, T*>::value, "'T' and 'U' pointers must be comparable");
        return a.GetAddressOf() < b.GetAddressOf();
    }

    template<typename T>
    inline _Check_return_ HRESULT ActivateInstance(
        _In_ HSTRING activatableClassId,
        _Out_ Internal::ComPtrRef<T> instance)
    {
        return ActivateInstance(activatableClassId, instance.ReleaseAndGetAddressOf());
    }

    template<typename T>
    inline _Check_return_ HRESULT GetActivationFactory(
        _In_  HSTRING activatableClassId,
              Internal::ComPtrRef<T> factory)
    {
        return wf::GetActivationFactory(activatableClassId, factory.ReleaseAndGetAddressOf());
    }

    // This is to match the definition of GetActivationFactory from roapi.h. This way we can always use
    // ctl::GetActivationFactory, regardless of whether we use our ComPtr or not.
    template<typename T>
    inline _Check_return_ HRESULT GetActivationFactory(
        _In_  HSTRING activatableClassId,
        _Out_ T** factory)
    {
        return wf::GetActivationFactory(activatableClassId, factory);
    }

    template <class T>
    HRESULT do_query_interface(_Out_ ComPtr<T>& spOut, _In_opt_ IUnknown *pIn)
    {
        HRESULT hr = S_OK;

        if (pIn)
        {
            hr = pIn->QueryInterface(__uuidof(T), (void **)&spOut);
        }

        return hr;
    }
}

// Overloaded global function to provide to IID_PPV_ARGS that support ComPtrRef
template<typename T>
void** IID_PPV_ARGS_Helper(
    _Inout_ ctl::Internal::ComPtrRef<T> pp)
{
    static_assert(__is_base_of(IUnknown, typename T::InterfaceType), "T has to derive from IUnknown");
    return pp;
}