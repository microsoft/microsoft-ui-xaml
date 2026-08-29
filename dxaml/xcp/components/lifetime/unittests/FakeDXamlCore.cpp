// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FakeDXamlCore.h"

#include <LifetimeUtils.h>
#include <ReferenceTrackerManager.h>
#include <WeakReferenceSourceNoThreadId.h>

using namespace DirectUI;

IDXamlCore* FakeDXamlCore::s_currentDXamlCore = nullptr;

FakeDXamlCore::FakeDXamlCore()
{
    InitializeSRWLock(&m_peerReferenceLock);
    ASSERT(SUCCEEDED(ReferenceTrackerManager::RegisterCore(this)));
    ASSERT(s_currentDXamlCore == nullptr);
    s_currentDXamlCore = this;
    m_threadId = ::GetCurrentThreadId();
}

FakeDXamlCore::~FakeDXamlCore()
{
    ASSERT(SUCCEEDED(ShutdownAllPeers()));
    ASSERT(SUCCEEDED(ReferenceTrackerManager::UnregisterCore(this)));
    s_currentDXamlCore = nullptr;
}

void FakeDXamlCore::AddToReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* item) /*override*/
{
    if (item != nullptr)
    {
        // Don't update tracked references while the ReferenceTrackerManager is running
        AutoReentrantReferenceLock peerTableLock(this);
        m_referenceTrackers.insert(item);
        ctl::addref_expected(item, ctl::ExpectedRef_Tree);
    }
}

void FakeDXamlCore::RemoveFromReferenceTrackingList(_In_ xaml_hosting::IReferenceTrackerInternal* item) /*override*/
{
    if (item != nullptr)
    {
        auto it = m_referenceTrackers.find(item);

        if (it != m_referenceTrackers.end())
        {
            // Don't update tracked references while the ReferenceTrackerManager is running
            AutoReentrantReferenceLock peerTableLock(this);
            ctl::release_expected(item);
            // Ordinarily, just remove the entry from the map
            m_referenceTrackers.erase(item);
        }
    }
}

_Check_return_ HRESULT 
FakeDXamlCore::ShutdownAllPeers() /*override*/
{
    ReferenceTrackerManager::OnShutdownAllPeers();
    return S_OK;
}
