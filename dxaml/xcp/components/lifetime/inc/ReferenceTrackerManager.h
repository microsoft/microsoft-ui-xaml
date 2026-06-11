// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#if DBG
#include "ReferenceTrackerLogging.h"
#include "XcpAllocation.h"
#endif

#include "vector_set.h"
#include "ReferenceTrackerInterfaces.h"

#include <wil\resource.h>

namespace DirectUI
{
    class DependencyObject;
    class TrackerTargetReference;

    struct IDXamlCore;

    //+---------------------------------------------------------------------------
    //
    //  Custom Iterator for PeerMapEntry in IDXamlCore
    //
    //+---------------------------------------------------------------------------

    class PeerMapEntriesHelper
    {
    private:
        IDXamlCore *m_pCore;

    public:
        PeerMapEntriesHelper(_In_ IDXamlCore *pCore) : m_pCore(pCore)
        {
            XCP_WEAK(&m_pCore);
        }

        class Iterator : public std::iterator<std::forward_iterator_tag, xaml_hosting::IReferenceTrackerInternal*>
        {
        private:
            IDXamlCore *m_pCore;
            ReferenceTrackerTable::iterator m_ReferenceTrackersIterator;
            PeerTable::iterator m_PeersIterator;

        public:
            Iterator(_In_ IDXamlCore *pCore, _In_ bool fEnd);

            Iterator& operator=(const Iterator& other)
            {
                m_pCore = other.m_pCore;
                m_PeersIterator = other.m_PeersIterator;
                m_ReferenceTrackersIterator = other.m_ReferenceTrackersIterator;
                return (*this);
            }

            bool operator==(const Iterator& other)
            {
                return (m_pCore == other.m_pCore && m_ReferenceTrackersIterator == other.m_ReferenceTrackersIterator && m_PeersIterator == other.m_PeersIterator);
            }

            bool operator!=(const Iterator& other)
            {
                return (m_pCore != other.m_pCore || m_ReferenceTrackersIterator != other.m_ReferenceTrackersIterator || m_PeersIterator != other.m_PeersIterator);
            }

            Iterator& operator++();

            Iterator operator++(int)
            {
                Iterator temp(*this);
                ++(*this);
                return (temp);
            }

            xaml_hosting::IReferenceTrackerInternal* operator*();
        };

        Iterator begin()
        {
            return(Iterator(m_pCore, FALSE));
        }

        Iterator end()
        {
            return(Iterator(m_pCore, TRUE));
        }
    };

    //+---------------------------------------------------------------------------
    //
    //  ReferenceTrackerManager
    //
    //+---------------------------------------------------------------------------


    class ReferenceTrackerManager final :
        public ::IReferenceTrackerManager
    {

    private:
        // Construction

        ReferenceTrackerManager();

        ~ReferenceTrackerManager();

    public:
        static _Check_return_ HRESULT EnsureInitialized(
            _Outptr_opt_ ReferenceTrackerManager** referenceTrackerManager = nullptr);


    public:
        // IUnknown

        IFACEMETHODIMP_(ULONG) AddRef() override
        {
            return InterlockedIncrement(&_refs);
        }

        IFACEMETHODIMP_(ULONG) Release() override
        {
            ULONG cRefs = InterlockedDecrement(&_refs);
            if (cRefs == 0)
                delete this;

            return cRefs;
        }

        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppvObject) override
        {
            *ppvObject = NULL;
            if (InlineIsEqualGUID(riid, __uuidof(IUnknown))
                || InlineIsEqualGUID(riid, __uuidof(IReferenceTrackerManager)))
            {
                *ppvObject = (IReferenceTrackerManager*) this;
                AddRef();
                return S_OK;
            }
            else
            {
                return E_NOINTERFACE;
            }
        }

    public:
        // IReferenceTrackerManager
        _Check_return_ HRESULT __stdcall ReferenceTrackingStarted() override;
        _Check_return_ HRESULT __stdcall FindTrackerTargetsCompleted(_In_ BOOLEAN findFailed) override;
        _Check_return_ HRESULT __stdcall ReferenceTrackingCompleted() override;
        _Check_return_ HRESULT __stdcall SetReferenceTrackerHost(_In_ IReferenceTrackerHost *value) override;


#if DBG
        _Check_return_ HRESULT RunValidation();
#endif

    public:

        // Registration of IDXamlCore instances
        static IDXamlCore* SearchCoresForObject(_In_ xaml_hosting::IReferenceTrackerInternal *pItem);
        static _Check_return_ HRESULT RegisterCore(_In_ IDXamlCore* pCore);
        static _Check_return_ bool IsCoreRegistered(_In_ IDXamlCore* pCore);
        static _Check_return_ HRESULT DisableCore(_In_ IDXamlCore* pCore);
        static _Check_return_ HRESULT UnregisterCore(_In_ IDXamlCore* pCore);
        static void OnShutdownAllPeers();
        static _Check_return_ HRESULT TriggerCollection();
        static _Check_return_ HRESULT TriggerFinalization();
        static _Check_return_ HRESULT TriggerCollectionForSuspend();

        // Methods for calling back to the caller
        static HRESULT AddRefFromReferenceTracker(IReferenceTrackerTarget *pTrackerTarget);
        static HRESULT ReleaseFromReferenceTracker(IReferenceTrackerTarget *pTrackerTarget);
        static void ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, IReferenceTrackerTarget *pTrackerTarget);

        // Keep track of the root context
        static void SetRootOfTrackerWalk(
            _In_opt_ xaml_hosting::IReferenceTrackerInternal *pTrackerRoot,
            _In_opt_ ::IFindReferenceTargetsCallback *callback,
            _In_ DirectUI::EReferenceTrackerWalkType walkType)
        {
            ASSERT( This->m_pTrackerWalkRoot == NULL || pTrackerRoot == NULL );
            ASSERT( This->_pFindReferenceTargetsCallback == NULL || callback == NULL );

            This->m_pTrackerWalkRoot = pTrackerRoot;
            This->_pFindReferenceTargetsCallback = callback;

            if (walkType == RTW_Find)
            {
                // The find walk is special: even if we've visited a node on a previous find walk,
                // we still need to keep walking so that the CLR can understand exactly which RCWs have
                // refs on exactly which CCWs.  But as we walk, we also need to remember which peers
                // we've visited for that particular walk so we don't get into an infinite loop
                // when we hit a cycle.  We *could* keep a boolean field on each peer to track if it's
                // been visited, and reset that field for every peer before each find walk, but that
                // would be expensive.
                //
                // So, we do this:
                // * In ReferenceTrackingStarted, all peers have m_lastFindWalkID set to 0.
                // * For each separate find walk, we increment m_currentFindWalkID to uniquely
                //   identify that walk.
                // * As we do a find walk, we mark each peer with the current walk by setting
                //   m_lastFindWalkID to m_currentFindWalkID.
                // * If we visit a peer whose m_lastFindWalkID is already equal to m_currentFindWalkID
                //   we know we already visited that peer fo the current find walk.  We found a cycle.
                //   We need to skip visiting that peer's references, to avoid an infinite loop.
                // * If m_currentFindWalkID ever wraps to zero, we need to go reset every peer's
                //   m_lastFindWalkID so we can start over again at "1".  This should be very rare,
                //   since for each GC in ReferenceTrackingStarted we set them all to zero.  It's
                //   expensive to go reset all the peers to zero so we want to avoid this.
                //
                // See also: WeakReferenceSourceNoThreadId::ReferenceTrackerWalkCore
                This->m_currentFindWalkID++;

                if (This->m_currentFindWalkID == 0)
                {
                    // This works fine until we wrap to zero.  There might still be peers around
                    // from find walk "1", so we can't do a find walk with ID "1" again.  Instead
                    // we reset all the peers' find walk mark to 0, so we can safely start over
                    // at 1 again.
                    This->ResetLastFindWalkIdForAllPeers();
                    This->m_currentFindWalkID = 1;
                }
            }
        }

        static unsigned short GetCurrentFindWalkID()
        {
            return This->m_currentFindWalkID;
        }

        static bool IsRootOfTrackerWalk(xaml_hosting::IReferenceTrackerInternal *pTracker)
        {
            return This->m_pTrackerWalkRoot == pTracker;
        }

        // If we're in between ReferenceTrackingStarted and ReferenceTrackingCompleted, return the thread
        // ID of the start.  (Complete might happen on a different thread.)  Will return zero if we're not
        // running.
        static DWORD GetStartThreadId()
        {
            if( This == nullptr )
            {
                return 0;
            }
            else
            {
                return This->m_startThreadId;
            }
        }


        // This is called by a UI thread that's waiting for a GC to complete.
        wil::unique_event_nothrow m_runningOnUIThreadEvent;
        static bool WaitForTrackingToComplete()
        {
            // Wait with no timeout, non-alertable, no nested pumping, for GC to complete.
            ASSERT( This != nullptr );
            return This->m_runningOnUIThreadEvent.wait();
        }


        // This indicates if a reference tracking is underway on this thread
        static void AssertActive(BOOLEAN assertActive)
        {
#if DBG

            // Removing this until it can be broken into foreground assert and background assert
            //BOOLEAN isActive = This != NULL && This->m_activeThreadID != 0 ;
            //ASSERT( isActive == assertActive );

#endif
        }

        static IInspectable* GetTrackerTarget(IUnknown* const punk);

        static IReferenceTrackerManager* GetNoRef()
        {
            IFCFAILFAST(ReferenceTrackerManager::EnsureInitialized());

            return static_cast<IReferenceTrackerManager*>(This);
        }

#if XCP_MONITOR
        static XCP_FORCEINLINE bool IsPeerStressEnabled()
        {
            // Quick check for the typical case (inlined)
            if (s_cMaxPeerStressIterations == 0)
            {
                return false;
            }

            // Non-trivial check for the stress case (not inlined)
            return CalculateIsPeerStressEnabled();
        }
#endif

#if XCP_MONITOR
        static bool CalculateIsPeerStressEnabled();
#endif

#if DBG
        static void UnregisterTrackerPtr(_In_ TrackerTargetReference *pTrackerPtr, IDXamlCore *pCore);
        static void RegisterTrackerPtr(_In_ TrackerTargetReference *pTrackerPtr, IDXamlCore *pCore);
        static void ValidateTrackerPtrs(IDXamlCore *pCore);

        static bool m_backgroundGCEnabled;
        static DWORD WINAPI BackgroundGCThread(_In_  LPVOID lpParameter);
        static HRESULT StartBackgroundGC();
#endif


        // Debug tracing for reference tracking
#if DBG
        // In Windbg ...
        //
        // To see the current position of the log index (the index that will be written to next)
        // dx microsoft_ui_xaml!DirectUI::ReferenceTrackerManager::_logPosition
        //
        // To see the 2,249 entries starting at entry 1,000:
        // dx -r3 (((microsoft_ui_xaml!DirectUI::ReferenceTrackerLogItem*) microsoft_ui_xaml!DirectUI::ReferenceTrackerManager::_logItems)+1000 ),2249x

        static ReferenceTrackerLogItem* _logItems;
        static const int _maxLogItemsEntries = 50000;
        static int _logPosition;

        static void Log(const ReferenceTrackerLogItem &item);
        static boolean IsLoggingEnabled() { return _logItems != nullptr; }
#endif


        // Tracing stats
        static int _peerCount;
        static int _targetCount;
        static int _unreachableCount;

    private:

        void ResetLastFindWalkIdForAllPeers();

        // The single instance of this class
        static ReferenceTrackerManager *This;
        static SRWLOCK s_lock;

        ULONG _refs;

        // State used during an active period (during reference walking phases)
        xaml_hosting::IReferenceTrackerInternal *m_pTrackerWalkRoot;
        IFindReferenceTargetsCallback *_pFindReferenceTargetsCallback;

        std::list<IDXamlCore*> m_allCores;
        std::list<IDXamlCore*> m_activeCores;

        IReferenceTrackerHost *_pReferenceTrackerHost;

        DWORD m_startThreadId;

#if DBG
        // Use the LeakIgnoringAllocator. If objects holding a TrackerPtr have been leaked, they won't unregister that pointer with the ReferenceTrackerManager
        // so we need to ignore these leaks. We also don't want a perfectly clean test getting dinged if this container has to reallocate
        // since a previous test dirtied it.
        using TrackerContainer = containers::vector_set<TrackerTargetReference*, ::std::less<TrackerTargetReference*>, XcpAllocation::LeakIgnoringAllocator<TrackerTargetReference*>>;
        TrackerContainer m_allTrackerPtrs;
#endif

#if XCP_MONITOR
        static int s_cPeerStressIteration;
        static int s_cMaxPeerStressIterations;
        bool m_bIsReferenceTrackingActive;
        static bool s_enableBackgroundGC;
#endif

        unsigned short m_currentFindWalkID;


    public:
        static bool HaveHost() { return This != nullptr && This->_pReferenceTrackerHost != nullptr; }

    };

}
