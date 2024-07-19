// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
/**************************************************************************\
*
* Abstract:
*
*   Ref counting helpers
*
\**************************************************************************/

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Reference counting macros.
//
//  These are the same as in winpal.h, except that calls to interlocked
//  increment and decrement go though gps.
//
//------------------------------------------------------------------------


//------------------------------------------------------------------------
//
//  Class: CReferenceCountBase
//
//      Provides base object for regular and interlocked reference counts.
//
//      In debug builds includes an 8 byte magic word and methods for
//      validatihg it and the refernce count value.
//
//------------------------------------------------------------------------

#if DBG
const XUINT64 XcpRefCount = 0x746e756f63666572ULL; // 'refcount'
const XUINT64 XcpDestroyd = 0x64796f7274736564ULL; // 'destroyd'
#endif

CReferenceCountBase::CReferenceCountBase()
{
    m_cReferences = 1;

    #if DBG
        m_magic = XcpRefCount;
    #endif
}

CReferenceCountBase::~CReferenceCountBase()
{
    #if DBG
        ASSERT(m_magic == XcpRefCount, L"Deleting corrupted (or deleted) refcount");
        ASSERT(m_cReferences == 0, L"Deleting refcount when references (%d) > 0", m_cReferences);
        m_magic = XcpDestroyd;
    #endif
}

#if DBG
void CReferenceCountBase::AddRefPreCheck() const
{
    ASSERT(m_magic == XcpRefCount, L"AddRef on corrupted (or deleted) refcount");
    ASSERT(m_cReferences > 0, L"AddRef on refcount with cReferences == 0");
    ASSERT(m_cReferences < 0xffffff, L"AddRef on refcount > 24 bits: m_cReferences = %x hex, %d decimal", m_cReferences, m_cReferences);
}

void CReferenceCountBase::AddRefPostCheck(XUINT32 cReferences) const
{
    ASSERT((XINT32)cReferences >= 0, L"Refcount overflowed 31 bits on Addref");
    ASSERT((XINT32)cReferences < 0xffffff, L"Refcount overflowed 24 bits on Addref");
}

void CReferenceCountBase::ReleasePreCheck() const
{
    ASSERT(m_magic == XcpRefCount, L"Release on corrupted (or deleted) refcount");
    ASSERT(m_cReferences > 0, L"Release on refcount with cReferences == 0");
    ASSERT(m_cReferences < 0xffffff, L"Release on refcount > 24 bits: m_cReferences = %x hex, %d decimal", m_cReferences, m_cReferences);
}

void CReferenceCountBase::ReleasePostCheck(XUINT32 cReferences) const
{
    ASSERT((XINT32)cReferences >= 0, L"Refcount (%d) dropped below zero on Release", cReferences);
}
#endif


//------------------------------------------------------------------------
//
//  Class: CReferenceCount
//
//      Provides a mixin class for objects requiring a non multi-threaded
//      reference count.
//
//      In debug builds includes the following checks:
//          o Magic word (8 bytes) has the correct value
//          o Reference count in range
//          o All AddRefs and Releases called from the same thread
//          o Deletion is by Release, not 'delete'
//
//------------------------------------------------------------------------

#if DBG && i386 && XCP_MONITOR

void CReferenceCount::ThreadCheck(const WCHAR *pszMessage) const
{
    IThreadMonitor *pThreadMonitor = GetPALDebuggingServices()->GetThreadMonitor();

    if (pThreadMonitor != nullptr)
    {
        pThreadMonitor->Resume();
        if (m_pThreadMonitor == nullptr)
        {
            m_pThreadMonitor = pThreadMonitor;
        }
        else
        {
            ASSERT(m_pThreadMonitor == pThreadMonitor, pszMessage);
        }
    }
}
#endif


CReferenceCount::CReferenceCount() : CReferenceCountBase()
{
#if DBG && i386 && XCP_MONITOR
    m_pThreadMonitor = NULL;
#endif
}


CReferenceCount::~CReferenceCount()
{
#if DBG && i386 && XCP_MONITOR
    ThreadCheck(L"Deletion called on different thread from first use");
#endif
}


XUINT32 CReferenceCount::AddRef() const
{
    XUINT32 cReferences;

#if DBG
    AddRefPreCheck();
#if i386 && XCP_MONITOR
    ThreadCheck(L"AddRef called on different thread from first use");
#endif
#endif

    cReferences = ++m_cReferences;

#if DBG
    AddRefPostCheck(cReferences);
#endif

    return cReferences;
}


XUINT32 CReferenceCount::Release() const
{
    XUINT32 cReferences;

#if DBG
    ReleasePreCheck();
#if i386 && XCP_MONITOR
    ThreadCheck(L"Release called on different thread from first use");
#endif
#endif

    cReferences = --m_cReferences;

#if DBG
    ReleasePostCheck(cReferences);
#endif

    if (cReferences == 0)
    {
        delete this;
    }

    return cReferences;
}


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

XUINT32 CInterlockedReferenceCount::AddRef() const
{
    XUINT32 cReferences;

#if DBG
    AddRefPreCheck();
#endif

    cReferences = (XUINT32) PAL_InterlockedIncrement((XINT32*) &m_cReferences);

#if DBG
    AddRefPostCheck(cReferences);
#endif

    return cReferences;
}

XUINT32 CInterlockedReferenceCount::Release() const
{
    XUINT32 cReferences;

#if DBG
    ReleasePreCheck();
#endif

    cReferences = (XUINT32) PAL_InterlockedDecrement((XINT32*) &m_cReferences);

#if DBG
    ReleasePostCheck(cReferences);
#endif

    if (cReferences == 0)
    {
        delete this;
    }

    return cReferences;
}

XINT32 CXcpObjectThreadSafeAddRefPolicy::AddRef(_Inout_ XINT32& RefCount)
{
    return PAL_InterlockedIncrement(&RefCount);
}

XINT32 CXcpObjectThreadSafeAddRefPolicy::Release(_Inout_ XINT32& RefCount)
{
    return PAL_InterlockedDecrement(&RefCount);
}
