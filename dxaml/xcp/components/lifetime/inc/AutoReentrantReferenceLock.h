// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ReferenceTrackerInterfaces.h"
#include "LifetimeExterns.h"
#include "ReferenceTrackerManager.h"

namespace DirectUI
{
    //
    //  This is an auto-release lock class, specifically for the DXamlCore::ReferenceSrwLock.
    //  We need special handling, because we need this behavior:
    //      *  When the DXamlCore thread is running app code (i.e. not running GC), it should take this
    //          lock.  The code uses this lock like a critical section, in that it may reenter it within a thread
    //          of execution.
    //      *  When GC starts, it should take this same lock, waiting if necessary, for the reentrant
    //          locks to be fully released.
    //
    //  We can't use a critical section, because the ReferenceTrackerManager needs to be able to
    //  hold this lock during both foreground GC and background GC.  If we were to use a critical
    //  section, we would take the critical section during foreground GC, and then the app thread could
    //  reenter during background GC (which is running on a separate thread).
    //
    //  A SRW lock is a performant alternative.
    //
    class AutoReentrantReferenceLock
    {
    public:
        //
        // Constructor that acquires the DXamlCore SRWLock.
        // The new object will automatically release the lock when it goes
        // out of scope.
        //
        explicit AutoReentrantReferenceLock(IPeerTableHost *pHost)
        {
            m_pHost = pHost;

            // Wait for the reference tracking to be unlocked.  This wait is non-alertable
            // and does no nested pumping.
            if (m_pHost)
            {
                
                ASSERT(m_pHost->GetReferenceLockEnterCount() >= 0);

                // See if GC has been started on this thread.
                if( m_pHost->GetThreadId() == ReferenceTrackerManager::GetStartThreadId())
                {
                    // GC was started on this thread, but it's running on another thread now.
                    // We can't use the SRW lock to wait for it to complete, because that lock was acquired by
                    // the ReferenceTrackerManager already on this thread, and the lock would just reenter.
                    // By the time the following call returns, the SRW lock weill have been released.

                    ReferenceTrackerManager::WaitForTrackingToComplete();
                }

                // Count the number of times we've entered the lock.  The first time we
                // enter, perform the actual exclusive acquisition to the SRW lock.  We don't
                // need this increment to be interlocked, because we're bound to a DXamlCore here,
                // which is single-threaded.

                if (m_pHost->IncrementReferenceLockEnterCount() == 1)
                {
                    AcquireSRWLockExclusive(&m_pHost->GetReferenceSrwLock());
                }
            }
        }

        ~AutoReentrantReferenceLock()
        {
            if (m_pHost)
            {
                ASSERT(m_pHost->GetReferenceLockEnterCount() > 0);

                // On the last reentrant exit, release the SRW lock.

                if (m_pHost->DecrementReferenceLockEnterCount() == 0)
                {
                    ReleaseSRWLockExclusive(&m_pHost->GetReferenceSrwLock());
                }
            }
        }

        static bool IsEntered()
        {
            IPeerTableHost *pHost = DXamlServices::GetPeerTableHost();
            return !(pHost == NULL || pHost->GetReferenceLockEnterCount() == 0);
        }

#if DBG
        static void AssertIfEntered()
        {
            ASSERT(!IsEntered());
        }
#endif

    private:
        // This shouldn't be copyable, so declare (but do not implement)
        // a private copy ctor and copy assignment operator.
        AutoReentrantReferenceLock(const AutoReentrantReferenceLock&);
        AutoReentrantReferenceLock& operator=(const AutoReentrantReferenceLock&);

        // This should not be created on the heap, so declare (but do not implement) private operator new/delete.
        static void* operator new(size_t);
        static void operator delete(void*);
        static void* operator new[](size_t);
        static void operator delete[](void*);

    private:
        IPeerTableHost *m_pHost;
    };
}
