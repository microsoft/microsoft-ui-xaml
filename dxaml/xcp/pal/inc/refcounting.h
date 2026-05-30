// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Description:
//      Ref counting helpers

#ifndef _REF_COUNTING_H_
#define _REF_COUNTING_H_

#include <minpal.h>
#include "weakref_ptr.h"

//------------------------------------------------------------------------
//
//  Reference counting macros.
//
//  These are the same as in winpal.h, except that calls to interlocked
//  increment and decrement go though gps.
//
//------------------------------------------------------------------------

#include "ReferenceCount.h"

//------------------------------------------------------------------------
//
//  Class: CInterlockedReferenceCount
//
//      Provides a mixin class for objects requiring a thread-safe
//      reference count.
//
//      In debug builds includes the following checks:
//          o Magic word (8 bytes) has the correct value
//          o Reference count in range
//          o Deletion is by Release, not 'delete'
//
//------------------------------------------------------------------------

class CInterlockedReferenceCount : protected CReferenceCountBase
{
public:
    XUINT32 AddRef() const;

    XUINT32 Release() const;
};

//
// Yet another referenced counted object base class.
//
// Multiple inheritance is not supported well across the compilers we use
// so single inheritance is always preferable. However, the existing base
// class that exists is meant to be used as a mixin. This works fine if
// you don't need to implement an interface or want to derive from any
// other class but as soon as that's a requirement the class is not longer
// useful.
//
// The classes below allow deriving from a base interface or class while
// still using only single inheritance.
//
// Optionally depending on the threading requirements, you can use
// InterlockedIncrement/InterlockedDecrement by changing the class
// policy.
//
// As a minor optimization if you don't need a base class you can use
// "NoBaseClass" as the base interface and the class specialization will not
// include a base class.
//

struct NoBaseClass;

//
// Thread safe policy for object reference management.
//
class CXcpObjectThreadSafeAddRefPolicy
{
    public:
        static XINT32 AddRef(_Inout_ XINT32& RefCount);
        static XINT32 Release(_Inout_ XINT32& RefCount);
};

//
// Single threaded policy for object reference management.
//
class CXcpObjectAddRefPolicy
{
    public:
        static inline XINT32 AddRef(_Inout_ XINT32& RefCount)
        {
            return ++RefCount;
        }

        static inline XINT32 Release(_Inout_ XINT32& RefCount)
        {
            return--RefCount;
        }
};

//
// Object base with a base interface.
//
template< typename T, typename AccessPolicy >
class __declspec(novtable) CXcpObjectBaseInternal : public T
{
    protected:
        // This class is marked novtable, so must not be instantiated directly.
        CXcpObjectBaseInternal() : m_nRefCount(1)
        {
        }

    public:
        // Methods with NOLINT below in some cases are declared in base class as virtual or not declared at all.  It means that they could be virtual or override
        // here.  Leaving as is until it's fixed.

        virtual ~CXcpObjectBaseInternal() = default; // NOLINT(modernize-use-override)

        XINT32 InternalAddRef()
        {
            return AccessPolicy::AddRef(m_nRefCount);
        }

        XINT32 InternalRelease()
        {
            XINT32 cRefCount = AccessPolicy::Release(m_nRefCount);

            if (cRefCount == 0)
            {
                delete this;
            }

            return cRefCount;
        }

    private:
        XINT32 m_nRefCount;
};

//
// Object base specialized on no base interface.
//
template< typename AccessPolicy >
class CXcpObjectBaseInternal< NoBaseClass, AccessPolicy >
{
    public:
        CXcpObjectBaseInternal() : m_nRefCount(1)
        {
        }

        virtual ~CXcpObjectBaseInternal()
        {
        }

        XINT32 InternalAddRef()
        {
            return AccessPolicy::AddRef(m_nRefCount);
        }

        XINT32 InternalRelease()
        {
            XINT32 cRefCount = AccessPolicy::Release(m_nRefCount);

            if (cRefCount == 0)
            {
                delete this;
            }

            return cRefCount;
        }

    private:
        XINT32 m_nRefCount;
};

//
// Reference counted object base.
//
// Optionally accepts a base interface and a threading policy.
//
template< typename T = NoBaseClass, typename AccessPolicy = CXcpObjectThreadSafeAddRefPolicy >
class CXcpObjectBase : public CXcpObjectBaseInternal< T, AccessPolicy >
{
    public:
        // Methods with NOLINT below in some cases are declared in base class as virtual or not declared at all.  It means that they could be virtual or override
        // here.  Leaving as is until it's fixed.

        virtual XUINT32 AddRef()  // NOLINT(modernize-use-override)
        {
            return (XUINT32)CXcpObjectBaseInternal< T, AccessPolicy >::InternalAddRef();
        }

        virtual XUINT32 Release()  // NOLINT(modernize-use-override)
        {
            return (XUINT32)CXcpObjectBaseInternal< T, AccessPolicy >::InternalRelease();
        }
};

//
// Utility macros for forward the AddRef/Release calls to a specific base.
//
#define FORWARD_ADDREF_RELEASE(Class)   \
virtual XUINT32 AddRef()                \
{                                       \
    return Class::AddRef();             \
}                                       \
virtual XUINT32 Release()               \
{                                       \
    return Class::Release();            \
}                                       \

#endif

// Base class for a ref-counted type compatible with xref::weakref_ptr.
class CXcpWeakObjectBase
{
    template <typename Ty>
    friend xref::weakref_ptr<Ty> xref::get_weakref(_In_opt_ Ty* pTargetObject);

    public:
        CXcpWeakObjectBase()
        {
        }

        virtual ~CXcpWeakObjectBase()
        {
            m_ref_count.end_destroying();
        }

        XUINT32 AddRef()
        {
            return m_ref_count.AddRef();
        }

        XUINT32 Release()
        {
            auto refCount = m_ref_count.Release();
            if (0 == refCount)
            {
                m_ref_count.start_destroying();
                delete this;
            }
            return refCount;
        }

    private:
        _Ret_notnull_ xref::details::control_block* EnsureControlBlock()
        {
            return m_ref_count.ensure_control_block();
        }

    private:
        xref::details::optional_ref_count m_ref_count;
};
