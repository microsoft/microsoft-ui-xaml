// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LifetimeUnitTests.h"
#include "FakeDependencyObject.h"
#include "FakeDXamlCore.h"
#include "TestTrackers.h"

#include <ComBase.h>
#include <ComObject.h>
#include <ComPtr.h>
#include <CStaticLock.h>
#include <WeakReferenceSourceNoThreadId.h>
#include <ReferenceTrackerManager.h>

using namespace ctl;
using namespace DirectUI;

namespace
{
    struct CcwFindCallback
        : wrl::RuntimeClass<wrl::RuntimeClassFlags<wrl::ClassicCom>, IFindReferenceTargetsCallback>
    {
        STDMETHOD(FoundTrackerTarget)(IReferenceTrackerTarget *target) override { ++CallbacksCount; return S_OK; }
        unsigned CallbacksCount = 0;
    };

    // Pillar C test peer: a minimal peer whose thread affinity and owning core can be controlled directly, so the
    // off-thread final-release funnel (WeakReferenceSourceNoThreadId::OnFinalReleaseOffThread) can be exercised
    // deterministically without standing up the full peer-registration / core-search machinery.
    class ControllableThreadPeer
        : public ctl::WeakReferenceSourceNoThreadId
    {
    public:
        _Check_return_ HRESULT CheckThread() const override
        {
            return m_onOwningThread ? S_OK : RPC_E_WRONG_THREAD;
        }

        DirectUI::IDXamlCore* GetCoreForObject() override { return m_core; }

        void SetOnOwningThread(bool value) { m_onOwningThread = value; }
        void SetCore(DirectUI::IDXamlCore* core) { m_core = core; }

    private:
        bool m_onOwningThread = true;
        DirectUI::IDXamlCore* m_core = nullptr;
    };
}

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Lifetime {
    
    bool LifetimeUnitTests::ClassSetup()
    {
        THROW_IF_FAILED(StaticLockGlobalInit());
        THROW_IF_FAILED(ReferenceTrackerManager::EnsureInitialized());
        
        m_dxamlCore = new FakeDXamlCore();
        
        return true;
    }

    bool LifetimeUnitTests::ClassCleanup()
    {
        delete m_dxamlCore;
        
        StaticLockGlobalDeinit();
        return true;
    }

    void LifetimeUnitTests::CanInstantiateLifetimeObjects()
    {
        ctl::ComPtr<IInspectable> instance;
        THROW_IF_FAILED(ComObject<WeakReferenceSourceNoThreadId>::CreateInstance(&instance));
        VERIFY_IS_NOT_NULL(instance);
    }

    void LifetimeUnitTests::CanWalkReferenceTrackers()
    {
        auto manager = ReferenceTrackerManager::GetNoRef();
        //FakeDXamlCore currentCore;

        // Instantiates the WUX objects and 'managed' objects
        ctl::ComPtr<DirectUI::WuxTracker> wuxObj1;
        ctl::ComPtr<DirectUI::WuxTracker> wuxObj2;
        wrl::ComPtr<DirectUI::TrackerTarget> trackerTarget;
        wrl::ComPtr<DirectUI::ReferenceTrackerTarget> refTrackerTarget;

        THROW_IF_FAILED(ctl::ComObject<DirectUI::WuxTracker>::CreateInstance(wuxObj1.GetAddressOf()));
        THROW_IF_FAILED(ctl::ComObject<DirectUI::WuxTracker>::CreateInstance(wuxObj2.GetAddressOf()));
        THROW_IF_FAILED(wrl::MakeAndInitialize<DirectUI::TrackerTarget>(trackerTarget.GetAddressOf()));
        THROW_IF_FAILED(wrl::MakeAndInitialize<DirectUI::ReferenceTrackerTarget>(refTrackerTarget.GetAddressOf()));

        // We haven't set the references yet. Make sure TryGetSafeReference returns false.
        {
            ctl::ComPtr<IInspectable> expectedNull;
            VERIFY_IS_FALSE(wuxObj1->TryGetSafeReference(&expectedNull));
            VERIFY_IS_NULL(expectedNull.Get());
        }

        // Make the first WUX object point to the second one.
        // The latter points to the refTrackerTarget which in turns points to trackerTarget.
        THROW_IF_FAILED(wuxObj1->SetTrackerReference(wuxObj2.Get()));
        THROW_IF_FAILED(wuxObj2->SetTrackerReference(refTrackerTarget.Get()));
        THROW_IF_FAILED(refTrackerTarget->GetInner()->SetTrackerReference(trackerTarget.Get()));

        // References are now set. Let's test TryGetSafeReference's behavior and
        // make sure it returns true and the correct reference.
        {
            ctl::ComPtr<IInspectable> expectedObj2;
            VERIFY_IS_TRUE(wuxObj1->TryGetSafeReference(&expectedObj2));
            VERIFY_ARE_EQUAL(wuxObj2.Get(), expectedObj2.Get());
        }
        
        wrl::ComPtr<CcwFindCallback> callback;
        THROW_IF_FAILED(wrl::MakeAndInitialize<CcwFindCallback>(&callback));

        // Run GC
        THROW_IF_FAILED(manager->ReferenceTrackingStarted());
        THROW_IF_FAILED(refTrackerTarget->GetInner()->FindTrackerTargets(callback.Get()));
        THROW_IF_FAILED(manager->ReferenceTrackingCompleted());

        // Check that our trackers got walked in some way or another.
        VERIFY_IS_TRUE(wuxObj1->GotWalked());
        VERIFY_IS_TRUE(wuxObj2->GotWalked());

        VERIFY_ARE_EQUAL(1, callback->CallbacksCount);          // We expect to find trackerTarget.

        // 1 peg due to SetPtrValue, 1 for ComposingTrackerTargetWrapper::Ensure, the other due to the RTW_PEG walk.
        VERIFY_ARE_EQUAL(3, refTrackerTarget->GetPegCount());   

        VERIFY_ARE_EQUAL(1, refTrackerTarget->GetUnpegCount());
        VERIFY_ARE_EQUAL(1, trackerTarget->GetPegCount());
        VERIFY_ARE_EQUAL(1, trackerTarget->GetUnpegCount());
    }



    // Coordination between LockingTest and LockingTestGCThread
    wil::unique_event_nothrow m_lockingTestUIThreadEvent;
    wil::unique_event_nothrow m_lockingTestGCThreadEvent;
    boolean m_lockingTestSyncFlag;
    static DWORD WINAPI LockingTestGCThread(_In_  LPVOID );

    // Test the locking logic in AutoReentranceReferenceLock
    void LifetimeUnitTests::LockingTest()
    {
        auto manager = ReferenceTrackerManager::GetNoRef();

        // Synchronization events to coordinate between here and the GC thread
        // (Auto reset, not signaled).
        m_lockingTestUIThreadEvent.create();
        m_lockingTestGCThreadEvent.create();

        // Create some Xaml objects and some CCWs
        
        ctl::ComPtr<DirectUI::WuxTracker> tracker1;
        ctl::ComPtr<DirectUI::WuxTracker> tracker2;
        wrl::ComPtr<DirectUI::TrackerTarget> trackerTarget1;
        wrl::ComPtr<DirectUI::TrackerTarget> trackerTarget2;

        THROW_IF_FAILED(ctl::ComObject<DirectUI::WuxTracker>::CreateInstance(tracker1.GetAddressOf()));
        THROW_IF_FAILED(ctl::ComObject<DirectUI::WuxTracker>::CreateInstance(tracker2.GetAddressOf()));
        THROW_IF_FAILED(wrl::MakeAndInitialize<DirectUI::TrackerTarget>(trackerTarget1.GetAddressOf()));
        THROW_IF_FAILED(wrl::MakeAndInitialize<DirectUI::TrackerTarget>(trackerTarget2.GetAddressOf()));



        //Create a GC thread
        
        wil::unique_handle thread( CreateThread(NULL, 0, LockingTestGCThread, NULL, 0, NULL));
        if( thread == nullptr )
        {
            THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
        }
        if( !SetThreadPriority( thread.get(), THREAD_PRIORITY_BELOW_NORMAL ))
        {
            THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
        }


        // Allow the GC thread to *start* the GC, but then it will block until we're ready for it to finish.
        m_lockingTestGCThreadEvent.SetEvent();
        m_lockingTestUIThreadEvent.wait();


        // Now set the event to let it finish the GC.  It won't run exactly when we set the event, though, since it's lower 
        // priority thread.  We'll block on the tracker1->SetTrackerReference() call, which will then let the GC run.
        // The GC thread will verify that we're blocked by checking that m_lockingTestSyncFlag is false.
        // (All of this test the slim lock case in AutoReentrantReferenceLock.)
        
        m_lockingTestSyncFlag = false;
        m_lockingTestGCThreadEvent.SetEvent();
        THROW_IF_FAILED(tracker1->SetTrackerReference(trackerTarget1.Get())); // This will block until GC finishes
        m_lockingTestSyncFlag = true;


        // During that GC, since we hadn't created the reference to the trackerTarget1 at the *start* of GC,
        // it won't have been pegged by GC.  It will have been pegged once, though, during the
        // TrackerTarget.Set (all references are initially pegged).
        
        VERIFY_ARE_EQUAL(1, trackerTarget1->GetPegCount());

        
        // Now start a GC on this thread, but we'll let it finish on the GC thread.
        THROW_IF_FAILED(manager->ReferenceTrackingStarted());
        m_lockingTestGCThreadEvent.SetEvent(); // Let GC thread run

        // The GC thread will verify that this is still false when it runs, indicating
        // that we get blocked on the tracker2->SetTrackerReference call.
        m_lockingTestSyncFlag = false;
        
        // This will block until GC finishes
        // (This is testing the unique_event_nothrow case in AutoReentrantReferenceLock.)
        THROW_IF_FAILED(tracker2->SetTrackerReference(trackerTarget2.Get())); // This will block until GC finishes
        m_lockingTestSyncFlag = true;


        // The first tracker target should have been pegged again by GC.  The second tracker target
        // should have been pegged during TrackerTarget.Set().
        VERIFY_ARE_EQUAL(2, trackerTarget1->GetPegCount());
        VERIFY_ARE_EQUAL(1, trackerTarget2->GetPegCount());

        // Now run GC on this thread and both should be pegged
        THROW_IF_FAILED(manager->ReferenceTrackingStarted());
        THROW_IF_FAILED(manager->ReferenceTrackingCompleted());

        VERIFY_ARE_EQUAL(3, trackerTarget1->GetPegCount());
        VERIFY_ARE_EQUAL(2, trackerTarget2->GetPegCount());
        
    }

    // GC thread proc used during the LockingTest
    DWORD WINAPI LockingTestGCThread(_In_  LPVOID )
    {
        auto manager = ReferenceTrackerManager::GetNoRef();

        // Start a GC when the test (UI) thread permits it.
        m_lockingTestGCThreadEvent.wait();
        THROW_IF_FAILED(manager->ReferenceTrackingStarted());

        // Tell the UI thread that we've started, then wait.
        m_lockingTestUIThreadEvent.SetEvent();
        m_lockingTestGCThreadEvent.wait();

        // Make sure the UI thread is blocked, then complete the GC        
        VERIFY_IS_TRUE( !m_lockingTestSyncFlag );
        THROW_IF_FAILED(manager->ReferenceTrackingCompleted());

        // Let the UI thread run and wait for it to request GC again
        m_lockingTestGCThreadEvent.wait();

        // This time, the GC was started on the UI thread.  Again, make sure
        // the UI thread is blocked, then complete the GC
        VERIFY_IS_TRUE( !m_lockingTestSyncFlag );
        THROW_IF_FAILED(manager->ReferenceTrackingCompleted());

        return 0;
    }

    // Pillar C - thread-affine release funnel.
    // An off-thread final release must be marshaled to the owning core's release queue, never destroyed inline.
    void LifetimeUnitTests::OffThreadFinalReleaseRoutesToOwningThread()
    {
        ctl::ComPtr<ControllableThreadPeer> peer;
        THROW_IF_FAILED(ctl::ComObject<ControllableThreadPeer>::CreateInstance(peer.GetAddressOf()));

        peer->SetCore(m_dxamlCore);
        peer->SetOnOwningThread(false);

        VERIFY_IS_TRUE(m_dxamlCore->IsFinalReleaseQueueEmpty());

        // allowOffThreadDelete = false: we assert the object is routed to the queue, and guarantee it is never
        // deleted inline by this call (so the ComPtr below remains the sole owner).
        const bool routed = peer->OnFinalReleaseOffThread(false /* allowOffThreadDelete */);

        VERIFY_IS_TRUE(routed);
        VERIFY_ARE_EQUAL(static_cast<size_t>(1), m_dxamlCore->GetFinalReleaseQueueCount());

        // Cleanup: OnFinalReleaseOffThread set the ignore-releases guard (the object is "owned" by the queue).
        // Undo it and drop the fake queue entry so the ComPtr can release/destroy normally on this (owning) thread.
        peer->DisableIgnoreReleases();
        m_dxamlCore->ClearFinalReleaseQueue();
        peer->SetOnOwningThread(true);
    }

    // Pillar C - a final release already on the owning thread must proceed inline (not be routed).
    void LifetimeUnitTests::OnThreadFinalReleaseIsNotRouted()
    {
        ctl::ComPtr<ControllableThreadPeer> peer;
        THROW_IF_FAILED(ctl::ComObject<ControllableThreadPeer>::CreateInstance(peer.GetAddressOf()));

        peer->SetCore(m_dxamlCore);
        peer->SetOnOwningThread(true);

        const bool routed = peer->OnFinalReleaseOffThread(false /* allowOffThreadDelete */);

        VERIFY_IS_FALSE(routed);
        VERIFY_IS_TRUE(m_dxamlCore->IsFinalReleaseQueueEmpty());
    }

    // Pillar C ties the off-thread funnel into the Pillar A peer-lifetime state machine by announcing the move into
    // the Releasing state. Validate that this edge is legal from every state a peer can be in before release, and
    // that Releasing is terminal-bound (only -> TornDown).
    void LifetimeUnitTests::ReleasingIsALegalTransitionFromEveryPreReleaseState()
    {
        using PeerLifetimeState = ctl::WeakReferenceSourceNoThreadId::PeerLifetimeState;

        VERIFY_IS_TRUE(ctl::WeakReferenceSourceNoThreadId::IsLegalPeerStateTransition(PeerLifetimeState::Detached, PeerLifetimeState::Releasing));
        VERIFY_IS_TRUE(ctl::WeakReferenceSourceNoThreadId::IsLegalPeerStateTransition(PeerLifetimeState::Pegged, PeerLifetimeState::Releasing));
        VERIFY_IS_TRUE(ctl::WeakReferenceSourceNoThreadId::IsLegalPeerStateTransition(PeerLifetimeState::Tracked, PeerLifetimeState::Releasing));

        // Releasing only proceeds to teardown.
        VERIFY_IS_TRUE(ctl::WeakReferenceSourceNoThreadId::IsLegalPeerStateTransition(PeerLifetimeState::Releasing, PeerLifetimeState::TornDown));

        // A torn-down peer is terminal: it can never re-enter Releasing.
        VERIFY_IS_FALSE(ctl::WeakReferenceSourceNoThreadId::IsLegalPeerStateTransition(PeerLifetimeState::TornDown, PeerLifetimeState::Releasing));
    }

} } } } }



