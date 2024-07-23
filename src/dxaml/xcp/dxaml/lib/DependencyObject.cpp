// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyObject.h"
#include "BindingExpression.g.h"
#include "DependencyPropertyChangedEventArgs.g.h"
#include "ToolTip_Partial.h"
#include "CustomDependencyProperty.h"
#include "ToolTipService.g.h"
#include "ModernCollectionBasePanel_Partial.h"
#include "PropertyChangedParamsHelper.h"
#include <MUX-ETWEvents.h>
#include "ThemeResourceExpression.h"
#include "VisualTreeHelper.h"
#include <DiagnosticsInterop.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "StaticStore.h"
#include "EventMgr.h"
#include "Callback.h"
#include <VisualTree.h>
#include <FocusMgr.h>
#include "host.h"
#include <XcpAllocationDebug.h>

#include <DeferredElementStateChange.h>

#include "Control_Partial.h"

using namespace DirectUI;
using namespace DirectUISynonyms;
using namespace xaml_hosting;

class TearoffSourceInfoPrivate final :
    public xaml::ISourceInfoPrivate
{
private:
    TearoffSourceInfoPrivate(_In_ DependencyObject* owner) : m_owner(owner)
    {
    }

    ~TearoffSourceInfoPrivate() = default;

public:

    // Due to a bug in C# WinRT 1.2.1, tearoffs are not correctly lifetime managed. To maintain tearoff perf advantages
    // but avoid an over-release, tie the lifetime strictly to the lifetime of the parent object. This works because the parent 
    // will still have a C# WinRT tracker reference. This instance does not hold a reference on the parent, since that would 
    // create a cycle. So unlike a normal tear-off, this tear-off is cached for the lifetime of the containing object, but
    // is held in an external map.
    static TearoffSourceInfoPrivate* Get(_In_ DependencyObject* owner)
    {
        auto& instanceMap = GetInstanceMap();
        auto sharedLock = s_instanceMapLock.lock_shared();
        auto instance_ref_it = instanceMap.find(owner);
        sharedLock.reset();
        if (instance_ref_it == instanceMap.end())
        {
            auto exclusiveLock = s_instanceMapLock.lock_exclusive();
            instance_ref_it = instanceMap.find(owner);
            if (instance_ref_it == instanceMap.end())
            {
                instance_ref_it = instanceMap.emplace(owner, new TearoffSourceInfoPrivate(owner)).first;
            }
        }
        return instance_ref_it->second;
    }

    static void Discard(_In_ DependencyObject* owner)
    {
        auto& instanceMap = GetInstanceMap();
        auto sharedLock = s_instanceMapLock.lock_shared();

        if (instanceMap.find(owner) != instanceMap.end())
        {
            sharedLock.reset();
            auto exclusiveLock = s_instanceMapLock.lock_exclusive();
            delete(instanceMap[owner]);
            instanceMap.erase(owner);
        }
    }

    static std::map<DependencyObject*, TearoffSourceInfoPrivate*>& GetInstanceMap()
    {
#if XCP_MONITOR
        // http://task.ms/41561802 Clean up leaks the XAML leak detector is ignoring
        auto leakIgnorer = XcpDebugStartIgnoringLeaks();
#endif
        static std::map<DependencyObject*, TearoffSourceInfoPrivate*> instanceMap;
        return instanceMap;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppv) override
    {
        if (InlineIsEqualGUID(riid, __uuidof(xaml::ISourceInfoPrivate)))
        {
            *ppv = static_cast<xaml::ISourceInfoPrivate*>(this);
            AddRef();
            return S_OK;
        }
        else
        {
            return static_cast<xaml::IDependencyObject*>(m_owner)->QueryInterface(riid, ppv);
        }
    }

    IFACEMETHODIMP_(ULONG) AddRef() override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->AddRef();

    }

    IFACEMETHODIMP_(ULONG) Release() override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->Release();
    }

    // IInspectable
    IFACEMETHODIMP GetIids(_Out_ ULONG *iidCount, _Outptr_ IID **iids) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetIids(iidCount, iids);
    }

    IFACEMETHODIMP GetRuntimeClassName(_Out_ HSTRING *pClassName) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetRuntimeClassName(pClassName);
    }

    IFACEMETHODIMP GetTrustLevel(_Out_ TrustLevel *trustLvl) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetTrustLevel(trustLvl);
    }

    // ISourceInfoPrivate
    IFACEMETHODIMP get_Line(_Out_ INT* pValue) override
    {
        return m_owner->GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Line, pValue);
    }

    IFACEMETHODIMP put_Line(_In_ INT value) override
    {
        return m_owner->SetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Line, value);
    }
    IFACEMETHODIMP get_Column(_Out_ INT* pValue) override
    {
        return m_owner->GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Column, pValue);
    }

    IFACEMETHODIMP put_Column(_In_ INT value) override
    {
        return m_owner->SetValueByKnownIndex(KnownPropertyIndex::DependencyObject_Column, value);
    }

    IFACEMETHODIMP get_ParseUri(_Out_ HSTRING* pValue) override
    {
        return m_owner->GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_ParseUri, pValue);
    }

    IFACEMETHODIMP put_ParseUri(_In_ HSTRING value) override
    {
        return m_owner->SetValueByKnownIndex(KnownPropertyIndex::DependencyObject_ParseUri, value);
    }

    IFACEMETHODIMP get_XbfHash(_Out_ HSTRING* pValue) override
    {
        return m_owner->GetValueByKnownIndex(KnownPropertyIndex::DependencyObject_XbfHash, pValue);
    }

    IFACEMETHODIMP put_XbfHash(_In_ HSTRING value) override
    {
        return m_owner->SetValueByKnownIndex(KnownPropertyIndex::DependencyObject_XbfHash, value);
    }

private:
    static wil::srwlock s_instanceMapLock;
    DependencyObject* m_owner;
};

wil::srwlock TearoffSourceInfoPrivate::s_instanceMapLock;

class TearoffMemoryInfoPrivate final :
    public xaml::IMemoryInfoPrivate
{
private:
    TearoffMemoryInfoPrivate(_In_ DependencyObject* owner) : m_owner(owner)
    {
    }

    ~TearoffMemoryInfoPrivate() = default;

public:

    // Due to a bug in C# WinRT 1.2.1, tearoffs are not correctly lifetime managed. To maintain tearoff perf advantages
    // but avoid an over-release, tie the lifetime strictly to the lifetime of the parent object. This works because the parent 
    // will still have a C# WinRT tracker reference. This instance does not hold a reference on the parent, since that would 
    // create a cycle. So unlike a normal tear-off, this tear-off is cached for the lifetime of the containing object, but
    // is held in an external map.
    static TearoffMemoryInfoPrivate* Get(_In_ DependencyObject * owner)
    {
        auto& instanceMap = GetInstanceMap();
        auto sharedLock = s_instanceMapLock.lock_shared();
        auto instance_ref_it = instanceMap.find(owner);
        sharedLock.reset();
        if (instance_ref_it == instanceMap.end())
        {
            auto exclusiveLock = s_instanceMapLock.lock_exclusive();
            instance_ref_it = instanceMap.find(owner);
            if (instance_ref_it == instanceMap.end())
            {
                instance_ref_it = instanceMap.emplace(owner, new TearoffMemoryInfoPrivate(owner)).first;
            }
        }
        return instance_ref_it->second;
    }

    static void Discard(_In_ DependencyObject * owner)
    {
        auto& instanceMap = GetInstanceMap();
        auto sharedLock = s_instanceMapLock.lock_shared();

        if (instanceMap.find(owner) != instanceMap.end())
        {
            sharedLock.reset();
            auto exclusiveLock = s_instanceMapLock.lock_exclusive();
            delete(instanceMap[owner]);
            instanceMap.erase(owner);
        }
    }

    static std::map<DependencyObject*, TearoffMemoryInfoPrivate*>& GetInstanceMap()
    {
#if XCP_MONITOR
        // http://task.ms/41561802 Clean up leaks the XAML leak detector is ignoring
        auto leakIgnorer = XcpDebugStartIgnoringLeaks();
#endif
        static std::map<DependencyObject*, TearoffMemoryInfoPrivate*> instanceMap;
        return instanceMap;
    }

    IFACEMETHODIMP QueryInterface(_In_ REFIID riid, _Outptr_ void** ppv) override
    {
        if (InlineIsEqualGUID(riid, __uuidof(xaml::IMemoryInfoPrivate)))
        {
            *ppv = static_cast<xaml::IMemoryInfoPrivate*>(this);
            AddRef();
            return S_OK;
        }
        else
        {
            return static_cast<xaml::IDependencyObject*>(m_owner)->QueryInterface(riid, ppv);
        }
    }

    IFACEMETHODIMP_(ULONG) AddRef() override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->AddRef();

    }

    IFACEMETHODIMP_(ULONG) Release() override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->Release();
    }


    // IInspectable
    IFACEMETHODIMP GetIids(_Out_ ULONG *iidCount, _Outptr_ IID **iids) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetIids(iidCount, iids);
    }

    IFACEMETHODIMP GetRuntimeClassName(_Out_ HSTRING *pClassName) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetRuntimeClassName(pClassName);
    }

    IFACEMETHODIMP GetTrustLevel(_Out_ TrustLevel *trustLvl) override
    {
        return static_cast<xaml::IDependencyObject*>(m_owner)->GetTrustLevel(trustLvl);
    }

    IFACEMETHODIMP GetCountOfDescendantUIElements(_Out_ UINT64* count) override
    {
        TraceGetElementCountBegin();

        DXamlServices::GetDXamlCore()->SetRTWElementCount(0);

        // Walk the tree
        m_owner->ReferenceTrackerWalk(RTW_GetElementCount, true);

        *count = DXamlServices::GetDXamlCore()->GetRTWElementCount();

        TraceGetElementCountEnd();

        return S_OK;
    }

    IFACEMETHODIMP GetEstimatedSizeOfDescendantImages(_Out_ UINT64* size) override
    {
        TraceGetCompressedImageSizeBegin();

        DXamlServices::GetDXamlCore()->SetRTWTotalCompressedImageSize(0);

        // Walk the tree
        m_owner->ReferenceTrackerWalk(RTW_TotalCompressedImageSize, true);

        *size = DXamlServices::GetDXamlCore()->GetRTWTotalCompressedImageSize();

        TraceGetCompressedImageSizeEnd();

        return S_OK;
    }

private:
    static wil::srwlock s_instanceMapLock;
    DependencyObject* m_owner;
};

wil::srwlock TearoffMemoryInfoPrivate::s_instanceMapLock;

DependencyObject::DependencyObject()
    : m_pDPChangedEventSource(NULL)
    , m_pInheritanceContextChangedEventSource(NULL)
    , m_pM3Parents(NULL)
    , m_pDO(NULL)
#if DBG
    , m_ulPegRefCountShutDownException(0)
#endif
{
}

DependencyObject::~DependencyObject()
{
    // Release any tearoff objects associated with this DependencyObject. Order doesn't matter
    TearoffMemoryInfoPrivate::Discard(this);
    TearoffSourceInfoPrivate::Discard(this);

    // Release event listeners

    if (m_pEventMap != nullptr)
    {
        for (EventMapping::iterator iterEvent = m_pEventMap->begin();
             iterEvent != m_pEventMap->end();
             iterEvent++)
        {
            iterEvent->second->Release();
        }
    }

    // Release delegates for property change notifications
    if (m_pNotificationVector != nullptr)
    {
        for (auto iter : *m_pNotificationVector)
        {
            // Each item in the vector is std::pair<KnownPropertyIndex, EventSource *>
            // Call the Shutdown member to remove any remaining delegates
            iter.second->Shutdown();
        }
    }

    ctl::release_interface(m_pDPChangedEventSource);
    ctl::release_interface(m_pInheritanceContextChangedEventSource);
}

_Check_return_ HRESULT
DependencyObject::Initialize(CDependencyObject* pDO)
{
    HRESULT hr = S_OK;

    // Can only be called once.
    ASSERT(m_bIsDisconnectedFromCore);
    m_pDO = pDO;
    m_bIsDisconnectedFromCore = FALSE;

    // Call standard initializer.
    IFC(Initialize());

Cleanup:
    RRETURN(hr);
}

// Prepares object's state
_Check_return_
HRESULT
DependencyObject::Initialize()
{

    IFC_RETURN(ctl::WeakReferenceSourceNoThreadId::Initialize());

    if (m_bIsDisconnectedFromCore)
    {
        // If we're not connected to a core DO, then capture the thread ID ourselves.
        m_uThreadId = ::GetCurrentThreadId();

        if (DXamlCore::GetCurrent())
        {
            // Set the create-time peg on this object, so that it doesn't get garbage collected before
            // it's fully initialized.
            SetReferenceTrackerPeg();

            // Add to the reference tracking management
            AddToReferenceTrackingList();
        }
    }
    else
    {
        IFC_RETURN(CoreImports::DependencyObject_AddRef(GetHandle()));
    }

    return S_OK;
}

KnownTypeIndex DependencyObject::GetTypeIndex() const
{
    return KnownTypeIndex::DependencyObject;
}

BOOLEAN DependencyObject::AllowResurrection()
{
    CDependencyObject* pDO = GetHandle();
    return (pDO && pDO->AllowPeerResurrection());
}

// Verify that we're executing on the thread this DO belongs to.
_Check_return_ HRESULT DependencyObject::CheckThread() const
{
    XUINT32 threadId;
    if (m_bIsDisconnectedFromCore)
    {
        threadId = m_uThreadId;
    }
    else
    {
        threadId = GetHandle()->GetContext()->GetThreadID();
    }

    if (threadId == ::GetCurrentThreadId())
    {
        return S_OK;
    }
    else
    {
        return RPC_E_WRONG_THREAD;
    }
}

DWORD DependencyObject::GetThreadID() const
{
    if (m_bIsDisconnectedFromCore)
    {
        return m_uThreadId;
    }
    else
    {
        return GetHandle()->GetContext()->GetThreadID();
    }
}

IDXamlCore* DependencyObject::GetCoreForObject()
{
    IDXamlCore *pCore = DXamlCore::GetFromDependencyObject(this);

    if (!pCore)
    {
        // this could be a DO that is not in the PeerMap Table so lookup
        // using the ReferenceTrackingTable
        pCore = ReferenceTrackerManager::SearchCoresForObject(ctl::interface_cast<IReferenceTrackerInternal>(this));
    }

    return pCore;
}

// Verify that we're executing on the thread this DO belongs to.
void
DependencyObject::OnFinalRelease()
{
    if (OnFinalReleaseOffThread())
    {
        // the object will have post for
        // a release on the UI thread
        return;
    }

    // if DXamlCore has already been destroyed, don't re-create it just to remove our peer mapping
    // (the peer mapping will have already been removed when DXamlCore was deinitialized)
    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();

    if (pCore)
    {
        // Do not move the RemovePeer() call inside DisconnectFrameworkPeer().
        //
        // DisconnectFrameworkPeer() is also called from DXamlCore::ShutdownAllPeers().
        // That function iterates over the peer mappings and isn't written to expect a peer mapping
        // to be removed during a call to DisconnectFrameworkPeer().
        pCore->RemovePeer(this);

        // or it could be in the ReferenceTrackingTable
        pCore->RemoveFromReferenceTrackingList(ctl::interface_cast<IReferenceTrackerInternal>(this));
    }

    if (GetHandle())
    {
        // Release peer's reference to the core object.
        VERIFYHR(DisconnectFrameworkPeer(/* bFinalRelease */ TRUE));
    }

    ctl::WeakReferenceSourceNoThreadId::OnFinalRelease();
}

HRESULT DependencyObject::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(DependencyObject)))
    {
        *ppObject = static_cast<DependencyObject*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml::IDependencyObject)))
    {
        *ppObject = static_cast<xaml::IDependencyObject*>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(::ITrackerOwner)))
    {
        *ppObject = ctl::interface_cast<::ITrackerOwner>(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml::ISourceInfoPrivate)))
    {
        // Gets (or creates) the source info tearoff object for this DO.  The tearoff object's lifetime is directly tied to the DO, so we still
        // add to the DO's ref count instead of the tearoff object's (AddRef/Releases on the tearoff object will forward to the DO instead as well).
        *ppObject = TearoffSourceInfoPrivate::Get(this);
    }
    else if (InlineIsEqualGUID(riid, __uuidof(xaml::IMemoryInfoPrivate)))
    {
        // Gets (or creates) the source info tearoff object for this DO.  The tearoff object's lifetime is directly tied to the DO, so we still
        // add to the DO's ref count instead of the tearoff object's (AddRef/Releases on the tearoff object will also forward to the DO instead).
        *ppObject = TearoffMemoryInfoPrivate::Get(this);
    }
    else
    {
        return ctl::WeakReferenceSourceNoThreadId::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

CDependencyObject*
DependencyObject::GetHandle() const
{
    if (m_bIsDisconnectedFromCore)
    {
        return nullptr;
    }
    return m_pDO;
}

CDependencyObject*
DependencyObject::GetHandleAddRef()
{
    auto pDO = GetHandle();
    VERIFYHR(CoreImports::DependencyObject_AddRef(pDO));
    return pDO;
}

_Check_return_
HRESULT
DependencyObject::DisconnectFrameworkPeerCore()
{
    CDependencyObject* pDO = NULL;

    // Set a flag that we're shutting down
    m_bIsDisconnected = TRUE;

    // Disconnect event listeners. Disconnecting means we remove the core event listeners. We
    // will release the actual event sources (and their handlers) in our destructor.
    if (m_pEventMap != nullptr)
    {
        for (EventMapping::iterator iterEvent = m_pEventMap->begin();
             iterEvent != m_pEventMap->end();
             iterEvent++)
        {
            // Ignore failures. We don't want to leak the rest just because we weren't able to
            // remove a core event listener.
            VERIFYHR(iterEvent->second->Disconnect());
        }

    }

    ClearPeerReferences();

    // Release the core object
    pDO = GetHandle();
    if (pDO != nullptr)
    {
        XUINT32 uThreadId = pDO->GetContext()->GetThreadID();

        // In some cases (such as CType), the core object held a strong reference to the framework object. The
        // core will try to unpeg this framework object when we release the 2nd to last reference to the core
        // object *and* when the core object is still connected to this framework object. However, in Jupiter
        // it's possible we earlier or later, which means we never get the unpeg from the core. As such,
        // we just forcefully unpeg right here when we disconnect.
        // We suppress  clearing the m_bReferenceTrackerPeg, because we're in destruction anyway, and it
        // takes the GC lock.
        UnpegNoRef( /*suppressClearReferenceTrackerPeg*/ true);

        pDO->DisconnectManagedPeer();
        ReleaseInterface(pDO);
        m_pDO = NULL;

        // Store the thread ID the DO belonged to.
        m_uThreadId = uThreadId;
    }

    // Set a flag that we're disconnected from our core object.
    m_bIsDisconnectedFromCore = TRUE;

    return S_OK;
}

_Check_return_
HRESULT
DependencyObject::DisconnectFrameworkPeer(bool bFinalRelease)
{
    HRESULT hr = S_OK;

    // We shouldn't reenter
    ASSERT(!m_bIsDisconnected);

    DependencyObject* pThis = this;

    if (!bFinalRelease)
    {
        // Take a reference to keep this object alive during the shutdown path. There are
        // cases where re-entrancy happens (mainly because of our self-references in m_pM3Parents),
        // so we need to make sure that we can safely de-reference things all the time. We guarantee
        // safe access by doing this add-ref, and releasing at the very end of this method.
        // The only exception to this is when we're destructing the object. In that case re-entrancy
        // should not be possible. We don't want to do the add-ref in that case because we'll
        // increase the ref count from 0->1, and when we call release at the end of this method,
        // it'll go back from 1->0, invoking OnFinalRelease for a second time.
        ctl::addref_interface_inner(pThis);
    }

    // We don't need the AutoReentrantReferenceLock here; this object is already
    // out of scope to the ReferenceTrackerManager, and taking it could lead to deadlock.
    IFC(DisconnectFrameworkPeerCore());


Cleanup:
    if (!bFinalRelease)
    {
        // This call may end up releasing the last reference. There should be no re-entrances
        // left at this point.
        ctl::release_interface_inner(pThis);
    }

    RRETURN(hr);

}

void DependencyObject::UpdatePegWithPossibleShutdownException(bool peg, BOOLEAN fShutdownException)
{
    if(peg)
    {
        ctl::WeakReferenceSourceNoThreadId::UpdatePeg(true);

        #if DBG
        if (fShutdownException)
        {
            m_ulPegRefCountShutDownException++;
        }
        #endif
    }
    else
    {
        #if DBG
        if (m_ulPegRefCount > 0)
        {
            if (fShutdownException)
            {
                m_ulPegRefCountShutDownException--;
            }
        }
        #endif

        ctl::WeakReferenceSourceNoThreadId::UpdatePeg(false);
    }
}

//+-----------------------------------------------------------------------------------
//
//  BeginShutdown
//
//  Disconnect from the core object.
//
//+-----------------------------------------------------------------------------------

void
DependencyObject::BeginShutdown()
{
    DXamlCore* pCore = DXamlCore::GetCurrentNoCreate();

    // DisconnectManagedPeer could release the last reference to the core CDependencyObject before it's done with the CDO
    // (e.g. while iterating through sparse storage, it clears a parent pointer that was the last ref to the CDO). Keep
    // the CDependencyObject alive while we're cleaning up.
    xref_ptr<CDependencyObject> cDO(GetHandle());

    if (cDO != NULL && pCore != NULL)
    {
        // Disconnect from the core object, so that it won't think it has a peer that needs
        // to be updated during tree teardown.

        cDO->DisconnectManagedPeer();
    }
}


//+-----------------------------------------------------------------------------------
//
//  EndShutdown
//
//  Release all references held by this DependencyObject, including the reference to the
//  core DO.
//
//+-----------------------------------------------------------------------------------

_Check_return_ HRESULT
DependencyObject::EndShutdown()
{
    HRESULT hr = S_OK;

#if DBG
    // Keep alive until we return.
    DependencyObject* pThis = this;
    ctl::addref_interface_inner(pThis);
#endif

    IFC(DisconnectFrameworkPeer(/* bFinalRelease */ FALSE));

#if DBG
    if (m_ulPegRefCount > 0 && m_ulPegRefCount == m_ulPegRefCountShutDownException)
    {
        ctl::release_interface_nonull(pThis);
        m_ulPegRefCount = 0;
        m_ulPegRefCountShutDownException = 0;
    }
#endif

Cleanup:
#if DBG
    ctl::release_interface_inner(pThis);
#endif
    RRETURN(hr);
}

void
DependencyObject::OnReferenceTrackingProcessed(_In_ DirectUI::IDXamlCore* pCore)
{
    // Only queue our object for unreachable cleanup once.
    // If an object becomes unreachable, the following sequence of events may occur:
    // 1. GC takes core locks.
    // 2. GC finds an object is unreachable, and adds it to the unreachable-cleanup queue.
    // 3. GC releases core locks.
    // 4. UI tick. UIAffinityReleaseQueue tries to take a core lock. It succeeds, moves the object to the final release queue, and releases the core lock.
    // 6. A new GC kicks in and takes core locks.
    // 7. UIAffinityReleaseQueue releases the final reference to an object. In the destructor of the object (specifically, OnFinalRelease), we call
    //    DXamlCore::RemovePeer to remove us from the peer table. This requires a core lock, but because GC already took the core locks, we're blocking.
    // 8. GC finds the same object is unreachable (still), and adds it to the unreachable-cleanup queue (again).
    // 9. GC releases core locks.
    // 10. OnFinalRelease unblocks and gets the core lock. It removes itself from the peer table and deletes itself from memory.
    // 11. New UI tick. UIAffinityReleaseQueue tries to take a core lock. It succeeds, moves the object to the final release queue, and releases the core lock.
    // 12. UIAffinityReleaseQueue tries to release what it thinks is a final reference. However, the object was already deleted (in #10), and we AV.
    //
    // To prevent this problem, we only queue ourselves once. (In case of resurrectable objects, we only queue ourselves once per "life.")
    if (!IsReachable() && IsAlive() && !AllowResurrection())
    {
        // While the above checks prevent the AV mentioned in the steps above, it doesn't catch when the final release
        // occurs on the UI thread and it wasn't initiated by GC (as per step 2). In this case we still hit steps 6-12,
        // but the checks are passed. So now we add another check. In step 7, the item's IsInFinalRelease is set true right
        // after the ref count has hit zero. But since it couldn't get the core lock here, in step 8 the GC adds the item to
        // the unreachable-cleanup queue and we still end up with the AV in step 12.
        // This fix is a mitigation as there's still a small window in-between when the object's ref count drops to zero
        // and the IsInFinalRelease flag is set to true. Long term we should look to adjusting the locking on the UI thread.
        // See bugs 20395531, 13108159
        if (!IsInFinalRelease())
        {
            // This is the first time this non-resurrectable object becomes unreachable. Add it to the
            // unreachable-cleanup queue so that we'll break any reference cycles, allowing this object to be
            // removed from memory entirely.

            #if DBG
            ASSERT(pCore);
            ASSERT(!m_bQueuedForUnreachableCleanup);
            m_bQueuedForUnreachableCleanup = TRUE;
            #endif

            pCore->QueueObjectForUnreachableCleanup(this);
        }
    }

    WeakReferenceSourceNoThreadId::OnReferenceTrackingProcessed(pCore);
}

//+-----------------------------------------------------------------------------------
//
//  Deinitialize
//
// Break reference cycles between peers and parent/children.
//
//+-----------------------------------------------------------------------------------

void
DependencyObject::Deinitialize()
{
    // Only try to break cycles if this object is not resurrectable.
    if (!AllowResurrection() && !IsAlive())
    {
        ClearPeerReferences();
    }
}

void
DependencyObject::ClearPeerReferences()
{
    // Go through the effective storage cleanning up
    // Release local property values
    if (m_pMapValueTable != nullptr)
    {
        m_pMapValueTable->clear();
    }

    // Clear core peer references first, because they currently depend on protection in the framework.
    CDependencyObject* pDO = GetHandle();
    if (pDO != nullptr)
    {
        pDO->ClearPeerReferences();

        if (pDO->ShouldFrameworkClearCoreExpectedReference())
        {
            // Release expected reference from core object to peer, which was set to keep peer alive for GC.
            // Peer may have already been disconnected from its core object when DependencyObject::OnFinalRelease
            // called RemovePeer, so core object can't ask the peer to release the reference. Release the
            // expected reference here, and ask the core object to clear its flag.
            pDO->OnClearedExpectedReferenceOnPeer();
            ReleaseForPeerReferenceHelper();
        }
    }

    // TODO: Can we just delete in DependencyObject destructor, so we don't need to
    //       loop through it and manually call RemovePtrValue?
    // Release references to property value peers.
    if (m_pPropertyValueReferences != nullptr)
    {
        for (auto& entry : *m_pPropertyValueReferences)
        {
            // Remove the TrackerPtr from 'this'.
            RemovePtrValue(entry.second);
        }

        // Delete the list and its TrackerPtrs.
        m_pPropertyValueReferences = nullptr;
    }

    // Release references to item peers (core children)
    if (m_pItemReferences != nullptr)
    {
        for (auto& entry : *m_pItemReferences)
        {
            ReleaseForPeerReference(entry);
        }

        m_pItemReferences = nullptr;
    }

    // Remove temporary parent list (see bug 101268). This is where re-entrancy may occur.
    if (m_pM3Parents != nullptr)
    {
        // When re-entrancy happens, we want to make sure we don't do the same thing
        // again. Take a local reference, and NULL out the field immediately.
        std::list<CDependencyObject*>* pM3Parents = m_pM3Parents;
        m_pM3Parents = nullptr;

        for (size_t i = 0; i < pM3Parents->size(); i++)
        {
            ctl::release_interface_nonull(this);
        }

        delete pM3Parents;
        pM3Parents = nullptr;
    }
}

// Handle change of parent
_Check_return_ HRESULT
DependencyObject::OnParentUpdated(
    _In_opt_ CDependencyObject* pOldParentCore,
    _In_opt_ CDependencyObject* pNewParentCore,
    _In_ bool isNewParentAlive)
{
    HRESULT hr = S_OK;
    DependencyObject *pNewParentPeer = NULL;
    DependencyObject *pOldParentPeer = NULL;

    // Notify old & new parents of the child's parent change
    // Get a direct ref-count on the inner object (the controlling object could already be deleted)
    if (pNewParentCore)
    {
        IGNOREHR(DXamlCore::GetCurrent()->TryGetPeerWithInternalRef(
                    pNewParentCore,
                    &pNewParentPeer));
    }
    if (pNewParentPeer)
    {
        IFC(pNewParentPeer->OnChildUpdated(this));
    }
    if (pOldParentCore)
    {
        IGNOREHR(DXamlCore::GetCurrent()->TryGetPeerWithInternalRef(
                    pOldParentCore,
                    &pOldParentPeer));
    }
    if (pOldParentPeer)
    {
        IFC(pOldParentPeer->OnChildUpdated(this));
    }

    // Notify child of the parent change.
    // Note that the core parent is passed, to prevent creating
    // the parent peer unless required. If needed, the child
    // will create the parent peer to process the change.
    //$TODO: Need better name for OnTreeParentUpdated. Or unify OnParentUpdated & OnTreeParentUpdated.
    IFC(OnTreeParentUpdated(pNewParentCore, isNewParentAlive));

Cleanup:
    // We did a GetPeerWithInternalRef on the parents, so need to do a release-inner as well.
    ctl::release_interface_inner(pOldParentPeer);
    ctl::release_interface_inner(pNewParentPeer);

    return hr;
}

//+---------------------------------------------------------------------------------
//
//  StoreM3PeerReferenceToObject
//
//  This is a temporary workaround until we get all supported types into DirectUI (see bug 101268).
//  Instead of creating a reference from a parent to a child, since the parent (peer) can't be created,
//  simply addref the child, and keep track of the parent, so that we can release the ref on the
//  child in RemoveM3PeerReferenceToObject.  We need to keep a list of the parents on behalf
//  of which we did an AddRef, so that we know which cases to release.
//
//+---------------------------------------------------------------------------------
// TODO: Silverlight#101268: Remove this case after all types are added at the end of M3
_Check_return_
HRESULT
DependencyObject::StoreM3PeerReferenceToObject(CDependencyObject *parentElement)
{
    HRESULT hr = S_OK;

    if (m_pM3Parents == NULL)
    {
        m_pM3Parents = new std::list<CDependencyObject*>();
    }

    m_pM3Parents->push_back(parentElement);

    ctl::addref_interface(this);

    // This reference comes from the tree, so is expected.
    ctl::addref_expected(this, ctl::ExpectedRef_Tree);

    RRETURN(hr);//RRETURN_REMOVAL

}



//+---------------------------------------------------------------------------------
//
//  RemoveM3PeerReferenceToObject
//
//  See comment in StoreM3PeerReferenceToObject
//
//+---------------------------------------------------------------------------------
// TODO: Silverlight#101268: Remove this case after all types are added at the end of M3
_Check_return_
HRESULT
DependencyObject::RemoveM3PeerReferenceToObject(CDependencyObject *parentElement)
{
    HRESULT hr = S_OK;
    std::list<CDependencyObject *>::iterator iter;

    if (m_pM3Parents == NULL)
        return S_OK;

    for (iter = m_pM3Parents->begin();
        (*iter) != parentElement && iter != m_pM3Parents->end();
        ++iter)
    { }

    // Remove the item from the list.

    if (iter != m_pM3Parents->end())
    {
        m_pM3Parents->erase(iter);
        ctl::release_expected(this);
        ctl::release_interface_nonull(this);
    }

    RRETURN(hr);//RRETURN_REMOVAL

}

_Check_return_ HRESULT DependencyObject::GetValueByKnownIndex(_In_ KnownPropertyIndex ePropertyIndex,  CValue& value)
{
    TraceApiPropertyGetValueStart(reinterpret_cast<uint64_t>(this));
    auto scopeExit = wil::scope_exit([&] { TraceApiPropertyGetValueStop(reinterpret_cast<uint64_t>(this), static_cast<uint16_t>(ePropertyIndex)); });

    RRETURN(GetValueCore(MetadataAPI::GetDependencyPropertyByIndex(ePropertyIndex), value));
}

// Dependency property system getter which handles both core and custom property requests.
_Check_return_ HRESULT DependencyObject::GetValue(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue)
{
    IFC_RETURN(CheckThread());

    wrl::ComPtr<IInspectable> spValue;

    // TODO: Check for CDependencyProperty::IsInheritedProperty() instead of only doing this for DataContext.
    // Force an inherited value lookup for the DataContext property.
    // Bug 1202518: we need to respect object identity when returning inherited DataContext values.
    if (pDP->AllowsObjects() &&
        m_pPropertyValueReferences != nullptr &&
        // For data context values that are inherited (default), we skip reading the value from m_pPropertyValueReferences
        // because it might be out of date.
        !(pDP->GetIndex() == KnownPropertyIndex::FrameworkElement_DataContext && GetHandle()->IsPropertyDefault(pDP)))
    {
        // For object identity purposes, grab the value out of the property reference store instead.
        auto itrReference = m_pPropertyValueReferences->find(pDP->GetIndex());
        if (itrReference != m_pPropertyValueReferences->end())
        {
            CValueBoxer::UnwrapExternalObjectReferenceIfPresent(itrReference->second.Get(), &spValue);
            // Unwrap value from weak ref if necessary (e.g. Page_Frame).
            if (spValue != nullptr && CDependencyObject::IsDependencyPropertyWeakRef(pDP->GetIndex()))
            {
                // get_value_as will add ref so we attach.
                spValue.Attach(ValueWeakReference::get_value_as<IInspectable>(spValue.Get()));
            }
        }
    }

    if (!spValue)
    {
        CValue valueFromCore;
        IFC_RETURN(GetValueCore(pDP, valueFromCore));

        {
            IFC_RETURN(CValueBoxer::UnboxPropertyObjectValue(&valueFromCore, pDP, &spValue));
        }
    }

    *ppValue = spValue.Detach();

    return S_OK;
}

// Get core property value.
_Check_return_ HRESULT DependencyObject::GetValueCore(_In_ const CDependencyProperty* pDP,  CValue& pValue)
{
    HRESULT hr = S_OK;

    // Check that it really is an attached inherited storage group property.
    // then ensure that the property is actually a property on the passed in class.

    if (!pDP->IsInheritedAttachedPropertyInStorageGroup() &&
        !pDP->Is<CCustomDependencyProperty>() && // Skip validation for custom DPs, because getting the actual type index of this instance is expensive.
        !MetadataAPI::IsAssignableFrom(pDP->GetTargetType()->GetIndex(), GetTypeIndex()))
    {
        IFCEXPECT(false);
    }

    IFC(CoreImports::DependencyObject_GetValue(
        GetHandle(),
        pDP,
        &pValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex ePropertyIndex, _In_ const CValue& value)
{
    TraceApiPropertySetValueStart(reinterpret_cast<uint64_t>(this));
    auto scopeExit = wil::scope_exit([&] { TraceApiPropertySetValueStop(reinterpret_cast<uint64_t>(this), static_cast<uint16_t>(ePropertyIndex)); });

    RRETURN(SetValueCore(MetadataAPI::GetDependencyPropertyByIndex(ePropertyIndex), value));
}

// Calls the core SetValue to set the value on the dependency property
_Check_return_ HRESULT DependencyObject::SetValueCore(_In_ const SetValueParams& args, _In_ bool fSetEffectiveValueOnly)
{
    HRESULT hr = S_OK;

    if (!fSetEffectiveValueOnly)
    {
        // Check that it really is an attached inherited storage group property.
        // then ensure that the property is actually a property on the passed in class.

        if (!args.m_pDP->Is<CCustomDependencyProperty>() && // Skip validation for custom DPs, because getting the actual type index of this instance is expensive.
            !args.m_pDP->IsInheritedAttachedPropertyInStorageGroup() &&
            !MetadataAPI::IsAssignableFrom(args.m_pDP->GetTargetType()->GetIndex(), GetTypeIndex()))
        {
            IFCEXPECT(false);
        }

        IFC(ClearCorePropertyExpression(args.m_pDP->GetIndex()));
    }

    // If a framework property is being set, the peer has state.
    // (Otherwise CDependencyObject::ClearPeerReferences will clear sparse properties.)
    if (args.m_pDP->IsSparse())
    {
        IFC(MarkHasState());
    }

    IFC(CoreImports::DependencyObject_SetValue(
        GetHandle(),
        args));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_ REFIID iid, _Out_ void** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());

    CValue coreValue;
    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(GetValueCore(pProperty, coreValue));

    // Unwrap value from weak ref if necessary (e.g. Page_Frame).
    if (CDependencyObject::IsDependencyPropertyWeakRef(nPropertyIndex))
    {
        ctl::ComPtr<IInspectable> spInspectable;

        IFC_RETURN(CValueBoxer::UnboxObjectValue(&coreValue, pProperty->GetPropertyType(), __uuidof(IInspectable), reinterpret_cast<void**>(spInspectable.ReleaseAndGetAddressOf())));

        *ppValue = nullptr;

        if (spInspectable != nullptr)
        {
            spInspectable.Attach(ValueWeakReference::get_value_as<IInspectable>(spInspectable.Get()));
            if (spInspectable != nullptr)
            {
                IFC_RETURN(spInspectable.Get()->QueryInterface(iid, ppValue));
            }
        }
    }
    else
    {
        IFC_RETURN(CValueBoxer::UnboxObjectValue(&coreValue, pProperty->GetPropertyType(), iid, ppValue));
    }

    return S_OK;
}

// For IInspectable return values, avoid the QI for IInspectable. Every interface is an IInspectable already.
_Check_return_ HRESULT DependencyObject::GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ IInspectable** ppValue)
{
    ARG_VALIDRETURNPOINTER(ppValue);
    IFC_RETURN(CheckThread());

    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(GetValue(pProperty, ppValue));

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::GetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _Out_ HSTRING* pValue)
{
    ARG_VALIDRETURNPOINTER(pValue);
    IFC_RETURN(CheckThread());

    CValue boxedValue;
    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(GetValueCore(pProperty, boxedValue));
    IFC_RETURN(CValueBoxer::UnboxValue(&boxedValue, pValue));

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ IInspectable* pValue)
{
    IFC_RETURN(CheckThread());

    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(DependencyObject::SetValueCore(pProperty, pValue));

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::SetValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex, _In_opt_ HSTRING value)
{
    IFC_RETURN(CheckThread());

    CValue boxedValue;
    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(CValueBoxer::BoxValue(&boxedValue, value));
    IFC_RETURN(DependencyObject::SetValueCore(pProperty, boxedValue));

    return S_OK;
}

BOOLEAN DependencyObject::IsCollection(KnownTypeIndex index)
{
    return MetadataAPI::GetClassInfoByIndex(index)->IsCollection();
}

_Ret_maybenull_ EffectiveValueEntry* DependencyObject::TryGetEffectiveValueEntry(
    _In_ KnownPropertyIndex nPropertyIndex)
{
    if (m_pMapValueTable != nullptr)
    {
        EffectiveValueStore::iterator cur = m_pMapValueTable->find(nPropertyIndex);

        if (cur != m_pMapValueTable->end())
        {
            // return the EffectiveValueEntry from the map table
            return cur->second.get();
        }
    }

    return nullptr;
}

_Check_return_ HRESULT DependencyObject::ClearEffectiveValueEntryExpression(
    _In_ EffectiveValueEntry *pValueEntry)
{
    if (pValueEntry->IsExpression())
    {
        ctl::ComPtr<IInspectable> spExpressionInsp = pValueEntry->GetBaseValue();
        ctl::ComPtr<IExpressionBase> spExpressionBase;

        IFC_RETURN(spExpressionInsp.As(&spExpressionBase));

        if (spExpressionBase)
        {
            IFC_RETURN(spExpressionBase.Cast<BindingExpressionBase>()->OnDetach());
            pValueEntry->ClearExpressionValue();
        }
    }

    return S_OK;
}

// Gets the default value for a dependency property.
_Check_return_ HRESULT
DependencyObject::GetDefaultValueInternal(_In_  const CDependencyProperty* pDP, _Out_ IInspectable** ppValue)
{
    RRETURN(GetDefaultValueBase(pDP, ppValue));
}

_Check_return_ HRESULT
DependencyObject::GetDefaultValueBase(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;

    if (auto customDependencyProperty = pDP->AsOrNull<CCustomDependencyProperty>())
    {
        IFC(customDependencyProperty->GetDefaultValue(ppValue));

        if (*ppValue)
        {
            goto Cleanup;
        }
    }

    // Return default values by type.
    switch (pDP->GetPropertyTypeIndex())
    {
    case KnownTypeIndex::TimeSpan:
    case KnownTypeIndex::Duration:
    case KnownTypeIndex::Matrix:
    case KnownTypeIndex::Matrix3D:
    case KnownTypeIndex::String:
        IFC(StaticStore::GetDefaultValue(pDP->GetPropertyTypeIndex(), ppValue));
        break;
    default:
        CValue coreDefaultValue;
        IFC(CoreImports::Property_GetDefaultValue(
            DXamlCore::GetCurrent()->GetHandle(),
            GetHandle(),
            pDP->GetPropertyTypeIndex(),
            pDP,
            &coreDefaultValue));
        IFC(CValueBoxer::UnboxObjectValue(&coreDefaultValue, pDP->GetPropertyType(), ppValue));
        break;
    }

Cleanup:
    RRETURN(hr);
}

// Dependency property system setter which handles core and custom properties.
_Check_return_ HRESULT
DependencyObject::SetValue(_In_ const CDependencyProperty* pDP, _In_ IInspectable *pValue)
{
    HRESULT hr = S_OK;

    IFC(SetValueInternal(pDP, pValue, false /* fAllowReadOnly */));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::SetBindingCore(_In_ const CDependencyProperty* pProperty, _In_ Binding* pBinding)
{
    HRESULT hr = S_OK;
    BindingExpression *pExpression = NULL;
    BOOL fBindingDone = FALSE;

    // We should always be called with a DP.
    ASSERT(!pProperty->Is<CCustomProperty>());

    IFC(BindingExpression::CreateObject(pBinding, &pExpression));

    pExpression->BeginSetBinding();
    IFC(SetValueExpression(pProperty, pExpression, ::BaseValueSourceUnknown));
    pExpression->EndSetBinding();

    fBindingDone = TRUE;

Cleanup:
    if (!fBindingDone && pExpression)
    {
        pExpression->EndSetBinding();
    }
    ctl::release_interface(pExpression);
    RRETURN(hr);
}

// This method will wrap the incoming IInspectable into a MOR if needed
// or it will just let the values go.
// The wrapping into a MOR will be done for now based on whether the incoming IPV
// contains an IInspectable or not. We will be wrapping only those IInspectable(s) that do not
// QI for IDependencyObject.
_Check_return_ HRESULT
DependencyObject::SetValueCore(_In_ const CDependencyProperty* dp, _In_opt_ IInspectable* value, _In_ bool setEffectiveValueOnly, _In_ ::BaseValueSource baseValueSource)
{
    CValue boxedValue;
    BoxerBuffer buffer;
    ctl::ComPtr<DependencyObject> eor;
    ctl::ComPtr<IInspectable> trackerTarget;
    ctl::ComPtr<IInspectable> valueWeakRef;
    IInspectable* outerValueNoRef = value;

    DependencyObject* innerDONoRef = ctl::query_interface_cast<DependencyObject>(value).Get();

    if (innerDONoRef)
    {
        // Verify that we're executing on the thread this DO belongs to.
        IFC_RETURN(innerDONoRef->CheckThread());
    }

    if (dp->Is<CCustomDependencyProperty>() && value != nullptr)
    {
        if (!ctl::is<IDependencyObject>(value))
        {
            // Preserve the IInspectables for custom DPs, so we don't end up creating new IInspectable boxes
            // when 3rd party code calls GetValue() for these properties.
            boxedValue.WrapIInspectableNoRef(value);
        }
        else
        {
            // For compatibility with 8.1, just wrap the IDependencyObject if it isn't
            // an instance of the custom property's type.
            bool isInstanceOf = false;
            IFC_RETURN(MetadataAPI::IsInstanceOfType(value, dp->GetPropertyType(), &isInstanceOf));
            if (!isInstanceOf)
            {
                boxedValue.WrapIInspectableNoRef(value);
            }
        }
    }

    if (boxedValue.IsUnset())
    {
        IInspectable* valueToBoxNoRef = value;

        // Wrap value in weak ref if necessary (e.g. Page_Frame).
        if (value != nullptr && CDependencyObject::IsDependencyPropertyWeakRef(dp->GetIndex()))
        {
            IFC_RETURN(ValueWeakReference::Create(value, &valueWeakRef));
            valueToBoxNoRef = valueWeakRef.Get();
        }

        // Wrap the incoming IInspectable into a CValue for core consumption
        IFC_RETURN(CValueBoxer::BoxObjectValue(
            &boxedValue,
            dp->GetPropertyType(),
            valueToBoxNoRef,
            &buffer,
            &eor,
            dp->IsSparse() && dp->AllowsObjects() && dp->ShouldPreserveObjectIdentity() // bPreserveObjectIdentity
            ));

        // If we wrapped the value in an EOR, then that becomes the outer value that we need to protect.
        // We still support object identity in this case, because its NativeValue will point to value.
        if (eor != nullptr && eor->GetTypeIndex() == KnownTypeIndex::ExternalObjectReference)
        {
            outerValueNoRef = ctl::iinspectable_cast(eor.Get());
        }
    }

    IFC_RETURN(SetValueCore(SetValueParams(dp, boxedValue, baseValueSource, outerValueNoRef), setEffectiveValueOnly));

    return S_OK;
}

// This is called by SetValueCore so that if an expression
// is being overwritten it can be cleared before calling
// into ::SetValue
_Check_return_ HRESULT DependencyObject::ClearCorePropertyExpression(_In_ KnownPropertyIndex nPropertyIndex)
{
    EffectiveValueEntry* pValueEntry = TryGetEffectiveValueEntry(nPropertyIndex);

    if (pValueEntry && pValueEntry->IsExpression())
    {
        ctl::ComPtr<IInspectable> spExpressionInsp = pValueEntry->GetBaseValue();

        ctl::ComPtr<IExpressionBase> spIExpressionBase;
        IFC_RETURN(spExpressionInsp.As(&spIExpressionBase));

        bool fCanSetValue = false;
        IFC_RETURN(spIExpressionBase.Cast<BindingExpressionBase>()->GetCanSetValue(&fCanSetValue));

        if (!fCanSetValue)
        {
            IFC_RETURN(spIExpressionBase.Cast<BindingExpressionBase>()->OnDetach());
            RemoveEffectiveValueEntryIfExists(nPropertyIndex);
        }
    }

    return S_OK;
}

// This is called by SetValueCore so that if an expression
// is being overwritten it can be cleared before calling
// into ::SetValue
_Check_return_ HRESULT DependencyObject::ClearCorePropertyThemeResourceExpression(_In_ const CDependencyProperty* pDP)
{
    EffectiveValueEntry* pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());

    if (pValueEntry && pValueEntry->IsExpression())
    {
        ctl::ComPtr<IInspectable> spExpressionInsp = pValueEntry->GetBaseValue();
        ctl::ComPtr<IThemeResourceExpression> spThemeResourceExpression = spExpressionInsp.AsOrNull<IThemeResourceExpression>();

        if (spThemeResourceExpression)
        {
            IFC_RETURN(spThemeResourceExpression.Cast<ThemeResourceExpression>()->OnDetach());
            RemoveEffectiveValueEntryIfExists(pDP->GetIndex());
        }
    }

    return S_OK;
}

_Ret_notnull_ EffectiveValueEntry* DependencyObject::CreateEffectiveValueEntry(
    _In_ KnownPropertyIndex propertyIndex)
{
    if (m_pMapValueTable == nullptr)
    {
        // Create map if it does not exist.
        m_pMapValueTable.reset(new EffectiveValueStore());
    }

    std::unique_ptr<EffectiveValueEntry> spNewValueEntry(new EffectiveValueEntry());
    EffectiveValueEntry* result = spNewValueEntry.get();

    {
        // Don't update tracked references while the ReferenceTrackerManager is running
        AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());
        ASSERT(m_pMapValueTable->find(propertyIndex) == m_pMapValueTable->end());
        (*m_pMapValueTable)[propertyIndex] = std::move(spNewValueEntry);
    }

    return result;
}

// Remove Effective Value Entry from Map Table.
void DependencyObject::RemoveEffectiveValueEntryIfExists(
    _In_ KnownPropertyIndex nPropertyIndex)
{
    if (m_pMapValueTable != nullptr)
    {
        EffectiveValueStore::iterator cur = m_pMapValueTable->find(nPropertyIndex);

        // Unique pointer will delete entry after leaving the lock, otherwise we could call up to CLR during destruction while
        // still holding the lock.
        std::unique_ptr<EffectiveValueEntry> entry;

        // Don't update tracked references while the ReferenceTrackerManager is running
        {
            AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

            ASSERT(cur != m_pMapValueTable->end());
            entry = std::move(cur->second);

            m_pMapValueTable->erase(cur);
        }
    }
}

// Sets the effective value of a custom dependency property.
// NOTE:
//   Currently, this is not an entry point for setting expressions.
_Check_return_ HRESULT
DependencyObject::SetValueInternal(_In_ const CDependencyProperty* pDP, _In_ IInspectable* pValue, _In_ bool fAllowReadOnly, _In_::BaseValueSource baseValueSource)
{
    HRESULT hr = S_OK;
    EffectiveValueEntry* pValueEntry = NULL;
    IExpressionBase* pTestAssociationExpressionBase = NULL;
    BOOLEAN fIsUnsetValue = FALSE;

    IFC(CheckThread());

    // validate dependency property is not a read only property.
    if (!fAllowReadOnly)
    {
        IFCEXPECT(!pDP->IsReadOnly());
    }

    IFC(DependencyPropertyFactory::IsUnsetValue(pValue, fIsUnsetValue));
    if (fIsUnsetValue)
    {
        IFC(ClearValue(pDP));
        goto Cleanup;
    }

    // If the new value is an expression then check that is is not already associated
    // We do not want to do anything in that case (similar to SL5)
    if ((pValue != NULL) &&
        SUCCEEDED(ctl::do_query_interface(pTestAssociationExpressionBase, pValue)) &&
        static_cast<BindingExpressionBase*>(pTestAssociationExpressionBase)->GetIsAssociated())
    {
        IFC(E_INVALIDARG);
    }

    //
    // TODO
    // 1) validate dependency property owner type is same as dependency object type
    // 2) if value is not expression, verify value is valid type for dependency property.
    //
    // An entry is added to the value table only for expressions, and TryGetEffectiveValueEntry
    // is used because the old expression needs to be obtained only to detach it.

    pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());

    if (pValueEntry)
    {
        if (pValueEntry->IsExpression())
        {
            ctl::ComPtr<IInspectable> spExpressionInsp = pValueEntry->GetBaseValue();
            ctl::ComPtr<IExpressionBase> spIExpressionBase;
            bool fCanSetValue = false;

            IFC(spExpressionInsp.As(&spIExpressionBase));

            IFC(spIExpressionBase.Cast<BindingExpressionBase>()->GetCanSetValue(&fCanSetValue));

            if (fCanSetValue)
            {
                ctl::ComPtr<IInspectable> spOldValue;
                IFC(GetValue(pDP, &spOldValue));

                // Ensure that the old value remains pegged for the rest of the call
                {
                    auto pegOldValue = ctl::make_autopeg(spOldValue.Get());

                    bool areEqual = false;
                    IFC(PropertyValue::AreEqual(spOldValue.Get(), pValue, &areEqual));
                    if (!areEqual)
                    {
                        IFC(pValueEntry->SetExpressionValue(pValue));
                        IFC(UpdateEffectiveValue(pDP, pValueEntry, ValueOperationDefault, baseValueSource));
                    }
                    goto Cleanup;
                }
            }

            // detach expression since it is no longer required
            IFC(DetachExpression(pDP, spIExpressionBase.Cast<BindingExpressionBase>()));

            // Set pValueEntry and NULL by calling DetachExpression() above.
            pValueEntry = NULL;
        }
        else
        {
            // no need to store for core properties as their base value is stored in core
            // if this was an expression it'll be handled in SetExpressionValue

            IFC(pValueEntry->SetBaseValue(pValue, BaseValueSourceLocal));
        }
    }

    IFC(UpdateEffectiveValue(pDP, pValueEntry, ValueOperationDefault, baseValueSource, pValueEntry ? NULL : pValue));

Cleanup:
    ReleaseInterface(pTestAssociationExpressionBase);
    RRETURN(hr);
}

// refresh expression
_Check_return_ HRESULT DependencyObject::RefreshExpression(_In_ const CDependencyProperty* pDP)
{
    EffectiveValueEntry* pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());

    IFCEXPECT_RETURN(pValueEntry);
    IFCEXPECT_RETURN(pValueEntry->IsExpression());

    return RefreshExpression_Helper(pDP, pValueEntry);
}

_Check_return_ HRESULT
DependencyObject::RefreshExpression_Helper(_In_ const CDependencyProperty* pDP, _In_ EffectiveValueEntry* pValueEntry)
{
    // Some properties like ContentPresenter.Content don't set the BaseValueSource correctly sometimes, so
    // consider "Default" as "Unknown" because the property may not have had its default bit cleared.
    ::BaseValueSource baseValueSource = GetHandle()->GetBaseValueSource(pDP);
    IFC_RETURN(UpdateEffectiveValue(pDP, pValueEntry, ValueOperationReevaluate, (baseValueSource == ::BaseValueSourceDefault ? ::BaseValueSourceUnknown : baseValueSource)));

    return S_OK;
}

// detach unused expression
_Check_return_ HRESULT
DependencyObject::DetachExpression(_In_ const CDependencyProperty* pDP, _In_ BindingExpressionBase *pExpression)
{
    IFC_RETURN(pExpression->OnDetach());
    RemoveEffectiveValueEntryIfExists(pDP->GetIndex());

    return S_OK;
}

// Set an expression for a dependency property.
_Check_return_ HRESULT
DependencyObject::SetValueExpression(
    _In_ const CDependencyProperty* dp,
    _In_ BindingExpressionBase* expression,
    _In_::BaseValueSource baseValueSource)
{
    auto mapValueTableCleanupGuard = wil::scope_exit([&]()
    {
        // If anything fails during execution, remove entry from map.
        RemoveEffectiveValueEntryIfExists(dp->GetIndex());
    });

    // expression should not be previously associated
    IFCEXPECT_RETURN(expression->GetIsAssociated() == FALSE);

    EffectiveValueEntry* valueEntry = TryGetEffectiveValueEntry(dp->GetIndex());

    if (valueEntry &&
        valueEntry->IsExpression())
    {
        // detach old expression
        ctl::ComPtr<IInspectable> spOldExpressionInsp = valueEntry->GetBaseValue();

        ctl::ComPtr<IExpressionBase> spOldExpressionIEB;
        IFC_RETURN(spOldExpressionInsp.As(&spOldExpressionIEB));

        IFC_RETURN(DetachExpression(dp, spOldExpressionIEB.Cast<BindingExpressionBase>()));

        valueEntry = TryGetEffectiveValueEntry(dp->GetIndex());
    }

    // Store entry.
    IFC_RETURN(MarkHasState());

    // Create and insert entry if it did not exist or was destroyed.
    if (valueEntry == nullptr)
    {
        valueEntry = CreateEffectiveValueEntry(dp->GetIndex());
    }

    // If we have an expression then attach it otherwise just set the value
    IFC_RETURN(expression->OnAttach(this, dp));

    auto expressionDetachCleanupGuard = wil::scope_exit([&]()
    {
        // If we failed for whatever reason after we attached the expression undo everything
        VERIFYHR(expression->OnDetach());
    });

    ctl::ComPtr<IInspectable> spValueToSet;

    IFC_RETURN(expression->GetValue(this, dp, &spValueToSet));

    ctl::ComPtr<IInspectable> spIBindingExpressionBase;
    IFC_RETURN(ctl::do_query_interface(spIBindingExpressionBase, expression));
    IFC_RETURN(valueEntry->SetBaseValue(spIBindingExpressionBase.Get(), BaseValueSourceLocal));
    IFC_RETURN(valueEntry->SetExpressionValue(spValueToSet.Get()));

    IFC_RETURN(UpdateEffectiveValue(dp, valueEntry, ValueOperationDefault, baseValueSource));

    // If everything went fine don't execute revert code.
    mapValueTableCleanupGuard.release();
    expressionDetachCleanupGuard.release();

    return S_OK;
}

// Update the effective value based on the layered property system.
// In case of setting core property to non-expression value we don't create
// EffectiveValueEntry but provide new value. In all other cases pValueEntry
// must be provided.
_Check_return_ HRESULT DependencyObject::UpdateEffectiveValue(
    _In_ const CDependencyProperty* pDP,
    _In_opt_ EffectiveValueEntry* pValueEntry,
    _In_ ValueOperation valueOperation,
    _In_ ::BaseValueSource baseValueSource,
    _In_opt_ IInspectable* pCorePropertyNewValue)
{
    ctl::ComPtr<IInspectable> spNewValue;

    ASSERT(!pValueEntry || !pCorePropertyNewValue);

    if (pValueEntry)
    {
        if (valueOperation & ValueOperationReevaluate)
        {
            IFC_RETURN(EvaluateEffectiveValue(pDP, pValueEntry, valueOperation));
        }

        spNewValue = pValueEntry->GetEffectiveValue();
    }
    else
    {
        // Re-evaluation requires pValueEntry.
        IFCEXPECT_ASSERT_RETURN(!(valueOperation & ValueOperationReevaluate));
        spNewValue = pCorePropertyNewValue;
    }

    // set the new effective value
    IFC_RETURN(SetValueCore(pDP, spNewValue.Get(), /* fSetEffectiveValueOnly */ true, baseValueSource));

    return S_OK;
}

// Helper to evaluate the effective value for a custom dependency property
// based on layered property system precedence rules.
_Check_return_ HRESULT
DependencyObject::EvaluateEffectiveValue(
    _In_ const CDependencyProperty* pDP,
    _Inout_ EffectiveValueEntry* pValueEntry,
    _In_ ValueOperation valueOperation)
{
    HRESULT hr = S_OK;

    // If this is a clearvalue cleanup the value store completely.
    if ((valueOperation & ValueOperationClearValue) == ValueOperationClearValue)
    {
        if (pValueEntry->HasModifiers())
        {
            pValueEntry->ClearExpressionValue();
        }
        pValueEntry->ClearBaseValue();
        ASSERT(!pValueEntry->HasModifiers());
    }

    // evaluate base value
    IFC(EvaluateBaseValue(pDP, pValueEntry, valueOperation));

Cleanup:
    RRETURN(hr);
}

//
// Evaluate base value using following precedence.
// 1. LocalValue
// 2. Style
// 3. Built-in style
// 4. Default value
//
_Check_return_ HRESULT
DependencyObject::EvaluateBaseValue(
    _In_ const CDependencyProperty* pDP,
    _Inout_ EffectiveValueEntry* pValueEntry,
    _In_ ValueOperation valueOperation)
{
    HRESULT hr = S_OK;
    IInspectable* pValue = NULL;
    IInspectable* pValueToSet = NULL;
    bool bGotValue = false;
    BaseValueSource baseValueSource = BaseValueSourceUnknown;
    bool processed = false;
    ctl::ComPtr<IFrameworkElement> spThisAsFE;

    // local value
    if ((valueOperation != ValueOperationClearValue) &&
        (pValueEntry->GetBaseValueSource() == BaseValueSourceLocal))
    {
        // reevaluate expression value
        if (pValueEntry->IsExpression())
        {
            ctl::ComPtr<IInspectable> spOldExpression = pValueEntry->GetBaseValue();

            ctl::ComPtr<IExpressionBase> spIExpressionBase;
            IFC(spOldExpression.As(&spIExpressionBase));

            IFC(spIExpressionBase.Cast<BindingExpressionBase>()->GetValue(this, pDP, &pValueToSet));
            IFC(pValueEntry->SetExpressionValue(pValueToSet));
        }

        goto Cleanup;
    }

    // style
    spThisAsFE = ctl::query_interface_cast<IFrameworkElement>(this);
    if(spThisAsFE)
    {
        IFC(spThisAsFE.Cast<FrameworkElement>()->GetValueFromStyle(pDP, &pValue, &bGotValue));
    }

    if (bGotValue)
    {
        baseValueSource = BaseValueSourceStyle;
    }
    else
    {

        Control* const pThisAsControl = Control::GetAsControlNoRef(this);
        if(pThisAsControl)
        {
            IFC(pThisAsControl->GetValueFromBuiltInStyle(pDP, &pValue, &bGotValue));
        }

        if (bGotValue)
        {
            baseValueSource = BaseValueSourceBuiltInStyle;
        }
        else
        {
            // default value
            IFC(GetDefaultValueInternal(pDP, &pValue));
            baseValueSource = BaseValueSourceDefault;
        }
    }

    // If value from style is a ThemeResource, bind to it
    IFC(TryProcessThemeResourceBaseValue(pDP, pValueEntry, pValue, baseValueSource, &processed));
    if (!processed)
    {
        //
        // detach old theme expression if present
        // this is required for the invalidateproperty() path for a framework property
        // that previously had a theme expression binding due to a non local value
        //
        IFC(ClearEffectiveValueEntryExpression(pValueEntry));
        IFC(pValueEntry->SetBaseValue(pValue, baseValueSource));
    }

Cleanup:
    ReleaseInterface(pValueToSet);
    ReleaseInterface(pValue);
    RRETURN(hr);
}

// If base value is a ThemeResourceExpression, bind to it and store it as base value
_Check_return_ HRESULT
DependencyObject::TryProcessThemeResourceBaseValue(
    _In_ const CDependencyProperty* pDP,
    _In_ EffectiveValueEntry* pValueEntry,
    _In_opt_ IInspectable* pBaseValue,
    BaseValueSource baseValueSource,
    _Out_ bool *pProcessed)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IThemeResourceExpression> spThemeResourceExpression;
    ctl::ComPtr<IExpressionBase> spNewExpressionBase;
    ctl::ComPtr<IInspectable> spNewExpressionValue;
    BindingExpressionBase* pExpression = NULL;

    *pProcessed = FALSE;

    spThemeResourceExpression.Attach(ctl::query_interface<IThemeResourceExpression>(pBaseValue));
    if (!spThemeResourceExpression)
    {
        goto Cleanup;
    }

    // Detach any old expression.
    if (pValueEntry->IsExpression())
    {
        ctl::ComPtr<IInspectable> spOldExpressionInsp = pValueEntry->GetBaseValue();

        ctl::ComPtr<IExpressionBase> spOldExpressionBase;
        IFC(spOldExpressionInsp.As(&spOldExpressionBase));

        IFC(spOldExpressionBase.Cast<BindingExpressionBase>()->OnDetach());
    }

    // Attach new expression
    IFC(ctl::do_query_interface(spNewExpressionBase, spThemeResourceExpression.Get()));
    pExpression = static_cast<BindingExpressionBase*>(spNewExpressionBase.Get());
    IFC(pExpression->OnAttach(this, pDP));
    IFC(pExpression->GetValue(this, pDP, &spNewExpressionValue));

    // Store new expression
    IFC(pValueEntry->SetBaseValue(spThemeResourceExpression.Get(), baseValueSource));
    IFC(pValueEntry->SetExpressionValue(spNewExpressionValue.Get()));

    *pProcessed = TRUE;

Cleanup:
    RRETURN(hr);
}

/*static */ _Check_return_ HRESULT
DependencyObject::RefreshExpressionsOnThemeChange(_In_ CDependencyObject* pCoreDO, _In_ Theming::Theme theme, _In_ bool forceRefresh)
{
    auto peer = pCoreDO->GetDXamlPeer();
    if (peer && peer->m_pMapValueTable)
    {
        // MSFT:15687216: Refreshing expressions can cause the contents of m_pMapValueTable to change if
        // the property whose expression is being expressed causes another property which also has an associated
        // expression to update (ItemsControl.ItemsSource and Selector.SelectedIndex is an example of such a pair).
        // As a result, we'll copy the keys into a stack vector, and iterate over that. We'll also need to check
        // if the current key still exists in the real table, otherwise we might be playing with a zombie expression.
        Jupiter::stack_vector<EffectiveValueStore::key_type, 24> tmp;
        tmp.m_vector.reserve(peer->m_pMapValueTable->size()); // avoid reallocations
        for (const auto& tableEntry : *(peer->m_pMapValueTable))
        {
            tmp.m_vector.push_back(tableEntry.first);
        }

        for (const auto& key : tmp.m_vector)
        {
            // See comment above regarding MSFT:15687216
            const auto& tableEntry = peer->m_pMapValueTable->find(key);
            if (tableEntry != peer->m_pMapValueTable->end())
            {
                // Notify expressions (i.e. BindingExpressions) about the theme change so that they can update any relevant usage of ThemeResource, e.g.
                // on the FallbackValue or TargetNullValue properties.
                // We want to make sure we only refresh the expression (i.e. re-evaluate the EffectiveValueEntry) if the BindingExpression's value has
                // actually changed; overly aggressive refreshing is problematic because it seems to result in subtle timing changes; plus it's a lot of
                // extra unneeded work.
                if (tableEntry->second->IsExpression())
                {
                    auto property = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(tableEntry->first);

                    bool valueChanged = false;
                    IFC_RETURN(tableEntry->second->NotifyThemeChanged(theme, forceRefresh, valueChanged));

                    if (valueChanged)
                    {
                        IFC_RETURN(peer->RefreshExpression_Helper(property, tableEntry->second.get()));
                    }
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
DependencyObject::RegisterPropertyChangedCallback(_In_ const CDependencyProperty* pDP, _In_ xaml::IDependencyPropertyChangedCallback *handler, _Out_ INT64 *token)
{
    ctl::ComPtr<CorePropertyChangedCallbackEventSourceType> spSource;
    HRESULT     hr = E_INVALIDARG;
    bool        found = false;

    ASSERT((handler != nullptr) && (token != nullptr));

// Fail if no handler or token return is specified

    if ((handler == nullptr) || (token == nullptr))
    {
        IFC(E_INVALIDARG);
    }

    // See if we already have a map for the callbacks.
    if (m_pNotificationVector == nullptr)
    {
        m_pNotificationVector.reset(new std::vector<NotificationVectorEntry>());
        IFC(MarkHasState());
    }

    // See if this DP already has an event source. These are stored as unordered pairs
    // in a vector. The assumption is not many properties will have handlers. If this
    // becomes slow we could switch to either an ordered vector or even a map.
    for (auto iter : *m_pNotificationVector)
    {
        if (iter.first == pDP->GetIndex())
        {
            spSource = iter.second;
            found = true;
            break;
        }
    }

    if (!found)
    {
        IFC(ctl::make(&spSource));
        spSource->Initialize(KnownEventIndex::UnknownType_UnknownEvent, this, /* bUseEventManager */ false);
        m_pNotificationVector->push_back(std::make_pair(pDP->GetIndex(), spSource));
    }

// Now try to add the incoming delegate to the EventSource

    IFC(spSource->AddHandler(handler));

// To maintain consistency with other APIs we'll return the handler as a token.

   *token = reinterpret_cast<INT64>(handler);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::UnregisterPropertyChangedCallback(_In_ const CDependencyProperty* pDP, _In_ INT64 token)
{
    ctl::ComPtr<CorePropertyChangedCallbackEventSourceType> spSource;
    HRESULT         hr = S_OK;

    if (m_pNotificationVector != nullptr)
    {
        for (auto iter = m_pNotificationVector->begin(); iter != m_pNotificationVector->end(); ++iter)
        {
            if (iter->first == pDP->GetIndex())
            {
                spSource = iter->second;
                IGNOREHR(spSource->RemoveHandler(reinterpret_cast<xaml::IDependencyPropertyChangedCallback *>(token)));

                // If that was the last handler in the event source we can remove it
                // from our vector
                if (!spSource->HasHandlers())
                {
                    m_pNotificationVector->erase(iter);
                }

                // If that was the last event source in the notification vector we
                // can delete the vector itself
                if (m_pNotificationVector->empty())
                {
                    m_pNotificationVector = nullptr;
                }

                break;
            }
        }
    }

    RRETURN(hr);
}

// Clear dependency property value.
_Check_return_ HRESULT
DependencyObject::ClearValueByKnownIndex(_In_ KnownPropertyIndex nPropertyIndex)
{
    const CDependencyProperty* pProperty = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);
    IFC_RETURN(ClearValue(pProperty));

    return S_OK;
}

_Check_return_ HRESULT
DependencyObject::ClearValue(_In_ const CDependencyProperty* pDP)
{
    IFC_RETURN(CheckThread());

    // If this object is shutting down, assume the property value is cleared already.
    if (!m_bIsDisconnected)
    {
        EffectiveValueEntry* pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());

        if (pValueEntry && pValueEntry->IsExpression())
        {
            ctl::ComPtr<IInspectable> spExpressionInspectable = pValueEntry->GetBaseValue();

            ctl::ComPtr<BindingExpressionBase> spExpressionBase;
            IFC_RETURN(spExpressionInspectable.As(&spExpressionBase));

            IFC_RETURN(DetachExpression(pDP, spExpressionBase.Get()));
        }
    }

    IFC_RETURN(CoreImports::DependencyObject_ClearValue(GetHandle(), pDP));

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::ReadLocalValue(_In_ const CDependencyProperty* pDP, _Out_ IInspectable** ppValue)
{
    *ppValue = nullptr;

    IFC_RETURN(CheckThread());

    EffectiveValueEntry *pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());
    ctl::ComPtr<IInspectable> spValue;
    BOOLEAN isUnsetValue = TRUE;

    if (pValueEntry)
    {
        IFC_RETURN(pValueEntry->GetLocalValue(&spValue));
        IFC_RETURN(DependencyPropertyFactory::IsUnsetValue(spValue.Get(), isUnsetValue));
    }

    if (isUnsetValue)
    {
        CValue value;
        bool hasLocalValue = false;
        bool isTemplateBound = false;

        IFC_RETURN(GetHandle()->ReadLocalValue(
            pDP,
            &value,
            &hasLocalValue,
            &isTemplateBound));

        if (!hasLocalValue)
        {
            IFC_RETURN(DependencyPropertyFactory::GetUnsetValue(ppValue));
        }
        else if (isTemplateBound)
        {
            // TODO: Return an empty TemplateBindingExpression.
        }
        else
        {
            IFC_RETURN(CValueBoxer::UnboxObjectValue(&value, pDP->GetPropertyType(), ppValue));
        }
    }
    else
    {
        IFC_RETURN(spValue.MoveTo(ppValue));
    }

    return S_OK;
}

_Check_return_ HRESULT
DependencyObject::GetAnimationBaseValue(_In_ const CDependencyProperty* pDP, _Out_ IInspectable **ppValue)
{
    HRESULT hr = S_OK;
    CValue value;

    IFC(CheckThread());

    IFC(GetHandle()->GetAnimationBaseValue(pDP, &value));

    IFC(CValueBoxer::UnboxObjectValue(&value, pDP->GetPropertyType(), ppValue));

Cleanup:
    RRETURN(hr);
}


bool DependencyObject::IsInLiveTree() const
{
    return GetHandle()->IsActive();
}

_Check_return_ HRESULT DependencyObject::IsPropertyLocal(_In_ const CDependencyProperty* pDP, _Out_ BOOLEAN *pfIsLocal)
{
    EffectiveValueEntry *pValueEntry = TryGetEffectiveValueEntry(pDP->GetIndex());
    bool bIsLocalValue = false;

    if (pValueEntry)
    {
        bIsLocalValue = pValueEntry->IsPropertyLocal();
    }

    if (!bIsLocalValue)
    {
        bool bIsDefaultValue = false;

        IFC_RETURN(CoreImports::DependencyObject_IsPropertyDefault(
            GetHandle(),
            pDP,
            &bIsDefaultValue));

        bIsLocalValue = (bIsDefaultValue == FALSE);
    }

    *pfIsLocal = !!bIsLocalValue;

    return S_OK;
}

// This is the NEW property changed method.
_Check_return_ HRESULT
DependencyObject::OnPropertyChanged2(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;

    // Handle attached property changed notifications.
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::ToolTipService_ToolTip:
    case KnownPropertyIndex::ToolTipService_KeyboardAcceleratorToolTip:
        IFC(ToolTipService::OnToolTipChanged(this, args));
        break;
    }

    // We raise the event here instead of DependencyObject::NotifyPropertyChanged to preserve the order in Windows 8.1. apps.
    // CONSIDER: Ideally, moving forward we only raise it here for compatibility reasons, and raise it in NotifyPropertyChanged to
    // make the behavior more predictable (i.e. not dependent on whether our internal control authors decide to call the base
    // OnPropertyChanged function before or after their custom code).
    if (m_pDPChangedEventSource)
    {
        // Don't raise events on the application once we are shutting down as objects
        // may be getting torn down.
        if (!DXamlServices::IsDXamlCoreShutdown())
        {
            IFC(DependencyObject::RaiseDPChanged(args.m_pDP));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::NotifyPropertyChanged(_In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyPropertyChangedEventArgs> spArgs;
    IPropertyChangedCallback* pCallbackNoRef = nullptr;
    ctl::ComPtr<IInspectable> spOldValue, spNewValue;
    ctl::ComPtr<CorePropertyChangedCallbackEventSourceType> spSource;
    ctl::ComPtr<xaml::IDependencyProperty> spDP;

    if (auto customDependencyProperty = args.m_pDP->AsOrNull<CCustomDependencyProperty>())
    {
        pCallbackNoRef = customDependencyProperty->GetPropertyChangedCallbackNoRef();
    }

    if (pCallbackNoRef != nullptr)
    {
        // Don't raise events on the application once we are shutting down as objects
        // may be getting torn down.
        if (!DXamlServices::IsDXamlCoreShutdown())
        {
            IFC(PropertyChangedParamsHelper::GetObjects(args, &spOldValue, &spNewValue));

            // Double check that the value really changed. Eventually we want to make sure that CDependencyObject::NotifyPropertyChanged
            // properly checks whether two EORs/IInspectables are equal.
            bool areEqual = FALSE;
            IFC(PropertyValue::AreEqual(spOldValue.Get(), spNewValue.Get(), &areEqual));
            if (!areEqual)
            {
                IFC(DependencyPropertyChangedEventArgs::Create(args.m_pDP->GetIndex(), spOldValue.Get(), spNewValue.Get(), &spArgs));

                // SYNC_CALL_TO_APP DIRECT - This next line may directly call out to app code.
                IFC(pCallbackNoRef->Invoke(this, spArgs.Get()));
            }
        }
    }
    else
    {
        IFC(OnPropertyChanged2(args));
    }

    // Don't raise events on the application once we are shutting down as objects
    // may be getting torn down.
    if (!DXamlServices::IsDXamlCoreShutdown())
    {
        // CONSIDER: Add a quirk and raise the DP changed event here for *all* properties.
        if (args.m_pDP->Is<CCustomDependencyProperty>())
        {
            if (m_pDPChangedEventSource)
            {
                IFC(DependencyObject::RaiseDPChanged(args.m_pDP));
            }
        }

        // Try to get the event source for the specified DP. Note that because of re-entrancy
        // it's conceivable the mapping no longer exist, gracefully exit when that happens.
        if (m_pNotificationVector != nullptr)
        {
            for (auto iter : *m_pNotificationVector)
            {
                if (iter.first == args.m_pDP->GetIndex())
                {
                    spSource = iter.second;

                    // Convert the core DP into one usable in the callbacks
                    IFC(MetadataAPI::GetIDependencyProperty(args.m_pDP->GetIndex(), &spDP));

                    // Now invoke the list of delegates in the event source
                    IFC(spSource->Raise(this, spDP.Get()));
                    break;
                }
            }
        }
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT DependencyObject::RaiseDPChanged(_In_ const CDependencyProperty* pDP)
{
    HRESULT hr = S_OK;

    if (m_pDPChangedEventSource)
    {
        IFC(m_pDPChangedEventSource->Raise(this, pDP));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DependencyObject::EnsureReferenceStore()
{
    HRESULT hr = S_OK;

    if (m_pPropertyValueReferences)
    {
        goto Cleanup;
    }

    m_pPropertyValueReferences.reset(new std::unordered_map<KnownPropertyIndex, TrackerPtr<IInspectable> >());

    IFC(MarkHasState());

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------
//
//  EnsureItemReferenceStore
//
//  Create m_pitemReferences
//
//+-----------------------------------------------------------------

_Check_return_
HRESULT
DependencyObject::EnsureItemReferenceStore()
{
    HRESULT hr = S_OK;

    if (m_pItemReferences)
    {
        goto Cleanup;
    }

    m_pItemReferences.reset(new std::list<DependencyObject*>());

    IFC(MarkHasState());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::MarkHasState()
{
    HRESULT hr = S_OK;

    // This is the first bit of state that we're adding to
    // the object, now mark the object as participating in the tree
    if (!m_bHasState)
    {
        CDependencyObject *pCoreDO = GetHandle();
        m_bHasState = true;

        if (pCoreDO) //May be disconnected from Core object
        {
            IFC(pCoreDO->SetParticipatesInManagedTreeDefault());

            // Since this DependencyObject became stateful, check if
            // this is being parsed, in which case, we should add the
            // PegNoRef that wasn't added when GetPeerPrivate() on this
            // framework object was initially called. Note that GetPeerPrivate's
            // call into CoreImports::DependencyObject_ShouldCreatePeerWithStrongRef
            // causes PegNoRef to be called only if the object participates.
            if (pCoreDO->IsParsing() || pCoreDO->ParserOwnsParent())
            {
                PegNoRef();
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------
//
//  StorePeerReferenceToObject
//
//  Store a reference to a peer object, keyed by a property ID
//
//+-----------------------------------------------------------------
_Check_return_ HRESULT
DependencyObject::StorePeerPropertyReferenceToObject(
    _In_ const CDependencyProperty* pProperty,
    _In_ IInspectable* pObject,
    _In_ bool bPreservePegNoRef,
    _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter)
{
    IFC_RETURN(EnsureReferenceStore());

    // Get this property out of the dictionary if it's already there.
    auto itrReference = m_pPropertyValueReferences->find(pProperty->GetIndex());

    // We could be setting to a value or null
    if (pObject)
    {
        // If this property isn't already in the dictionary, insert it.
        if (itrReference == m_pPropertyValueReferences->end())
        {
            TrackerPtr<IInspectable>* ptpValue = nullptr;

            // Grab a slot from the map.
            ptpValue = &(*m_pPropertyValueReferences)[pProperty->GetIndex()];

            // Store the value in the TrackerPtr.
            SetPtrValue(*ptpValue, pObject);
        }

        // If this property is already in the dictionary, swap its value.
        else
        {
            if (ppOldValueOuter != nullptr)
            {
                IFC_RETURN(itrReference->second.CopyTo(ppOldValueOuter));
            }
            SetPtrValue(itrReference->second, pObject);
        }

        // Unpeg the value.  The peg was necessary when the target was held by the core (typically
        // by the parser).  But now its lifetime is the responsibility of the source.
        // Even in failure we need to release this peg so as not to cause a leak.
        if (!bPreservePegNoRef)
        {
            ctl::ComPtr<DependencyObject> spDO;
            if (SUCCEEDED(ctl::do_query_interface(spDO, pObject)))
            {
                spDO->UnpegNoRef();
            }
        }
    }
    else
    {
        // We're setting to null which means that we want to to remove the reference
        // from the references collection
        if (itrReference != m_pPropertyValueReferences->end())
        {
            if (ppOldValueOuter != nullptr)
            {
                IFC_RETURN(itrReference->second.CopyTo(ppOldValueOuter));
            }

            // Clear the TrackerPtr.
            RemovePtrValue(itrReference->second);

            // Remove the entry from our list of property value references.
            m_pPropertyValueReferences->erase(itrReference);
        }
    }

    return S_OK;
}

_Check_return_
HRESULT
DependencyObject::GetReference(_In_ const CDependencyProperty* pProperty, _Outptr_ DependencyObject** ppObject)
{
    // TODO: This should be dead code... Remove this and callers asap.
    return E_NOTIMPL;
}

//+--------------------------------------------------------------------------------
//
// AddRef a peer object.  This requires special processing, because it might be composed
// by a ReferenceTrackerTarget.
//
//+--------------------------------------------------------------------------------

void
DependencyObject::AddRefForPeerReferenceHelper( )
{
    // Is the object composed by an IReferenceTrackerTarget?
    if (SUCCEEDED(ComposingTrackerTargetWrapper::Ensure(this)))
    {
        if (ComposingTrackerTargetWrapper::GetTrackerTarget(this) != nullptr)
        {
            ASSERT(IsComposed());

            // Keep a ref count on the inner object (DependencyObject)
            ctl::addref_interface_inner(this);
            ctl::addref_expected(this, ctl::ExpectedRef_Tree);

            // Keep a special ReferenceTracker ref-count on the composing object.
            ComposingTrackerTargetWrapper::AddRefTarget(this);
        }

        // This isn't composed by a tracking target, so just do a normal AddRef
        else
        {
            ctl::addref_interface(this);

            // If this object is composed, the above addref went to the controlling object, and we don't have to update
            // the expected ref count. But if it isn't composed, we need to account for it.
            if (!ComposingTrackerExtensionWrapper::IsComposed(this))
            {
                ctl::addref_expected(this, ctl::ExpectedRef_Tree);
            }
        }
    }
}



//+--------------------------------------------------------------------------------
//
// Release a peer object.  This requires special processing, because it might be composed
// by a ReferenceTrackerTarget.
//
//+--------------------------------------------------------------------------------

void
DependencyObject::ReleaseForPeerReferenceHelper(  )
{
    DependencyObject *pTempDO = NULL;

    // Is the object composed by an IReferenceTrackerTarget?
    if (ComposingTrackerTargetWrapper::GetTrackerTarget(this) != nullptr)
    {
        ASSERT(IsComposed());

        // Release the special ReferenceTracker ref-count on the composing object.
        ComposingTrackerTargetWrapper::ReleaseTarget(this);

        // Release the ref count on the inner object (DependencyObject)
        pTempDO = this;
        ctl::release_expected(pTempDO);
        ctl::release_interface_inner(pTempDO);
    }

    // This isn't a composed object, so just do a normal Release
    else
    {
        pTempDO = this;

        // If this object is composed, the below release will go to the controlling object, and we don't have to update
        // the expected ref count. But if it isn't composed, we need to account for it.
        if (!ComposingTrackerExtensionWrapper::IsComposed(this))
        {
            ctl::release_expected(pTempDO);
        }

        ctl::release_interface(pTempDO);
    }
}


//+------------------------------------------------------------------------
//
//  StorePeerReferenceToObject
//
// Store a reference from this peer to the specified peer (in m_pItemReferences).
// (For lifetime management when working with a GC system.)
//
//+------------------------------------------------------------------------

_Check_return_
HRESULT
DependencyObject::StorePeerItemReferenceToObject(_In_ DependencyObject *pObject)
{
    HRESULT hr = S_OK;

    ASSERT(pObject != NULL);

    // Allocate m_pItemResources
    IFC(EnsureItemReferenceStore());

    // Add to the list
    // Don't update tracked references while the ReferenceTrackerManager is running
    {
        AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

        m_pItemReferences->push_back(pObject);
    }

    // AddRef the object.  We have to do a special AddRef if it's compsed by an IReferenceTrackerTarget.
    AddRefForPeerReference(pObject);

    // Unpeg the value.  The peg was necessary when the target was held by the core (typically
    // by the parser).  But now its lifetime is the responsibility of the source.
    // Even in failure we need to release this peg so as not to cause a leak.
    pObject->UnpegNoRef();

Cleanup:

    RRETURN(hr);
}




//+------------------------------------------------------------------------
//
// RemovePeerReferenceToObject
//
// Remove a reference that this peer keeps on the specified peer
// (For lifetime management when working with a GC system.)
//
//+------------------------------------------------------------------------

_Check_return_
HRESULT
DependencyObject::ClearPeerItemReferenceToObject(_In_ DependencyObject *pObject)
{
    HRESULT hr = S_OK;
    std::list<DependencyObject*>::iterator iter;

    ASSERT(pObject != NULL);

    // Check for noop
    if (m_pItemReferences == nullptr)
    {
        goto Cleanup;
    }

    // We'll sometimes get called even when the object isn't in the list, so look for it first.

    for (iter = m_pItemReferences->begin();
        (*iter) != pObject && iter != m_pItemReferences->end();
        ++iter)
    { }

    // Remove the item from the list.

    if (iter != m_pItemReferences->end())
    {
        // Don't update tracked references while the ReferenceTrackerManager is running
        {
            AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

            m_pItemReferences->erase(iter);
        }

        // Release the object.  We have to do a special Release if it's composed by an IReferenceTrackerTarget.
        ReleaseForPeerReference(pObject);
    }



Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  NotifyPropertyChanged
//
//  This is called from core to notify the peer that a property has changed.
//
//+------------------------------------------------------------------------
_Check_return_ HRESULT
DependencyObject::NotifyPropertyChanged(
    _In_ CDependencyObject* pDO,
    _In_ const PropertyChangedParams& args)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spDO;

    // Only invoke NotifyPropertyChanged if we already have a peer.
    IFC(DXamlCore::GetCurrent()->TryGetPeer(pDO, &spDO));
    if (spDO != nullptr)
    {
        IFC(spDO->NotifyPropertyChanged(args));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::GetDefaultValueCallback(
    _In_ CDependencyObject* pReferenceObject,
    _In_ const CDependencyProperty* pDP,
    _Out_ CValue* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spReferenceObjectDO;

    IFC(DXamlCore::GetCurrent()->GetPeer(pReferenceObject, &spReferenceObjectDO));
    IFCFAILFAST(spReferenceObjectDO->GetDefaultValue2(pDP, pValue));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DependencyObject::SetBindingCallback(
    _In_ CDependencyObject* pTarget,
    _In_ KnownPropertyIndex propertyId,
    _In_ CDependencyObject* pBinding)
{

    ctl::ComPtr<DependencyObject> spTargetDO;
    ctl::ComPtr<ExternalObjectReference> spTargetEOR;
    ctl::ComPtr<IInspectable> spTarget;
    ctl::ComPtr<DependencyObject> spBindingAsDO;

    // Get the peers.
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC_RETURN(pCore->GetPeer(pTarget, &spTargetDO));
    IFC_RETURN(pCore->GetPeer(pBinding, &spBindingAsDO));

    // PegNoRef the target. We assume that this callback only ever gets called during parse.
    spTargetDO->PegNoRef();

    // Resolve the property.
    const CDependencyProperty* pProperty = MetadataAPI::GetPropertyByIndex(propertyId);

    // Check if this is a non-DO (wrapped in an EOR).
    if (SUCCEEDED(spTargetDO.As(&spTargetEOR)))
    {
        auto customProperty = pProperty->AsOrNull<CCustomProperty>();

        // It is. Assign the binding to the property like a regular value.
        spTargetEOR->get_Target(&spTarget);

        ASSERT(customProperty != nullptr);

        IFC_RETURN(customProperty->GetXamlPropertyNoRef()->SetValue(
            spTarget.Get(),
            ctl::iinspectable_cast(spBindingAsDO.Get())));
    }
    else
    {
        const CDependencyProperty* pDP = nullptr;

        // Try to get the underlying DP so we can attach a binding.
        IFC_RETURN(MetadataAPI::TryGetUnderlyingDependencyProperty(pProperty, &pDP));

        if (pDP != nullptr)
        {
            // We've got a DO and a DP.

            // Set the binding.
            IFC_RETURN(spTargetDO->SetBindingCore(pDP, spBindingAsDO.Cast<Binding>()));
        }
        else
        {
            auto customProperty = pProperty->AsOrNull<CCustomProperty>();

            ASSERT(customProperty != nullptr);

            // We've got a DO, but no DP. Assign the binding to the property like a regular value.
            IFC_RETURN(customProperty->GetXamlPropertyNoRef()->SetValue(
                ctl::iinspectable_cast(spTargetDO.Get()),
                ctl::iinspectable_cast(spBindingAsDO.Get())));
        }
    }

    return S_OK;
}

//+------------------------------------------------------------------------
//
//  AddPeerReferenceToItemCallback
//
//  This is called from core to add a reference from one peer to another (e.g. for the collection scenario).
//  This is like SetPeerReferenceToProperty, except for the collection case instead of the property case.
//
//+------------------------------------------------------------------------

_Check_return_ HRESULT
DependencyObject::AddPeerReferenceToItemCallback(
    _In_ CDependencyObject* nativeSource,
    _In_ CDependencyObject* nativeTarget)
{
    HRESULT hr = S_OK;

    DependencyObject *pSource = NULL;
    DependencyObject *pTarget = NULL;

    // Get the target's peer
    DXamlCore* pCore = DXamlCore::GetCurrent();
    IFC(pCore->GetPeer(nativeTarget, &pTarget));

    // Get the source's peer
    hr = pCore->GetPeer(nativeSource, &pSource);
    if (hr == E_FAIL)
    {
        // TODO: Silverlight#101268: Remove this case after all types are added at the end of M3:
        IFC(pTarget->StoreM3PeerReferenceToObject(nativeSource));
        pSource = NULL;
    }
    else
    {
        // Store the reference to the target in the source
        IFC(pSource->StorePeerItemReferenceToObject(pTarget));
    }

Cleanup:

    // Unpeg the target.  The peg was necessary when the target was held by the core (typically
    // by the parser).  But now its lifetime is the responsibility of the source.
    // Even in failure we need to release this peg so as not to cause a leak.
    pTarget->UnpegNoRef();

    ctl::release_interface(pSource);
    ctl::release_interface(pTarget);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//  RemovePeerReferenceToItemCallback
//
//  This is called from core to remove a reference from one peer to another (e.g. for the collection scenario).
//  (For lifetime management when working with a GC system.)
//  This is like SetPeerReferenceToProperty (to null), except for the collection case instead of the property case.
//
//+------------------------------------------------------------------------

_Check_return_ HRESULT
DependencyObject::RemovePeerReferenceToItemCallback(
    _In_ CDependencyObject* nativeSource,
    _In_ CDependencyObject* nativeTarget)
{
    HRESULT hr = S_OK;

    DependencyObject *pSource = NULL;
    DependencyObject *pTarget = NULL;

    DXamlCore* pCore = DXamlCore::GetCurrent();

    // Get the target's peer
    // Get a direct ref-count on the inner object (the controlling object already be deleted).
    IFC(pCore->TryGetPeerWithInternalRef(nativeTarget, &pTarget));
    if (pTarget)
    {
        // Similarly get the source's peer
        hr = pCore->TryGetPeerWithInternalRef(nativeSource, &pSource);
        VERIFYHR(hr);
        if (!pSource)
        {
            // TODO: Silverlight#101268: Remove this case after all types are added at the end of M3:
            IFC(pTarget->RemoveM3PeerReferenceToObject(nativeSource));
            pSource = NULL;
        }
        else
        {
            // Remove the source's reference on the target.
            IFC(pSource->ClearPeerItemReferenceToObject(pTarget));
        }
    }

Cleanup:

    ctl::release_interface_inner(pSource);
    ctl::release_interface_inner(pTarget);

    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::SetPeerReferenceToPropertyCallback(
    _In_ CDependencyObject* nativeTarget,
    _In_ const CDependencyProperty* pDP,
    _In_ const CValue& value,
    _In_ bool bPreservePegNoRef,
    _In_opt_ IInspectable* pNewValueOuter,
    _Outptr_opt_result_maybenull_ IInspectable** ppOldValueOuter)
{
    HRESULT hr = S_OK;
    DXamlCore* pCore = DXamlCore::GetCurrent();
    ctl::ComPtr<DependencyObject> spTarget;
    ctl::ComPtr<IInspectable> spValue;
    ctl::ComPtr<DependencyObject> spValueDO;

    if (ppOldValueOuter != nullptr)
    {
        *ppOldValueOuter = nullptr;
    }

    if (pNewValueOuter != nullptr)
    {
        spValue = pNewValueOuter;
    }
    else if (!value.IsNullOrUnset())
    {
        switch (value.GetType())
        {
        case valueObject:
            // There are still some objects that we do not create in the DirectUI
            // TODO: Ensure that we can create everything
            // For now ignore those objects for which we can't create peers
            if (FAILED(pCore->GetPeer(value.AsObject(), &spValueDO)))
            {
                goto Cleanup;
            }

            spValue.Attach(ctl::iinspectable_cast(spValueDO.Detach()));
            break;
        case valueIInspectable:
            spValue = value.AsIInspectable();
            break;
        }
    }

    // If we can't create a peer for the target then we ignore this call
    // TODO: Ensure that we can create peers for everything
    if (FAILED(pCore->GetPeer(nativeTarget, &spTarget)))
    {
        goto Cleanup;
    }

    if (ppOldValueOuter != nullptr)
    {
        IFC(spTarget->StorePeerPropertyReferenceToObject(pDP, spValue.Get(), bPreservePegNoRef, ppOldValueOuter));
    }
    else
    {
        IFC(spTarget->StorePeerPropertyReferenceToObject(pDP, spValue.Get(), bPreservePegNoRef));
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DependencyObject::GetDPChangedEventSource(_Out_ IDPChangedEventSource **ppEventSource)
{
    HRESULT hr = S_OK;

    if (m_pDPChangedEventSource == NULL)
    {
        IFC(ctl::ComObject<DPChangedEventSource>::CreateInstance(&m_pDPChangedEventSource));
    }

    *ppEventSource = m_pDPChangedEventSource;
    ctl::addref_interface(m_pDPChangedEventSource);

Cleanup:

    RRETURN(hr);
}

_Check_return_
HRESULT
DependencyObject::TryGetDPChangedEventSource(_Out_ IDPChangedEventSource** ppEventSource)
{
    HRESULT hr = S_OK;

    // Do not attempt to create the event source

    *ppEventSource = m_pDPChangedEventSource;
    ctl::addref_interface(m_pDPChangedEventSource);

    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::GetInheritanceContextChangedEventSource(_Outptr_ IInheritanceContextChangedEventSource **ppEventSource)
{
    HRESULT hr = S_OK;

    if (m_pInheritanceContextChangedEventSource == NULL)
    {
        IFC(ctl::ComObject<InheritanceContextChangedEventSource>::CreateInstance(&m_pInheritanceContextChangedEventSource));
        m_pInheritanceContextChangedEventSource->Initialize(this);

        GetHandle()->SetWantsInheritanceContextChanged(true);
    }

    *ppEventSource = m_pInheritanceContextChangedEventSource;
    ctl::addref_interface(m_pInheritanceContextChangedEventSource);

Cleanup:

    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::GetMentor(_Outptr_ FrameworkElement **ppMentor)
{
    auto pCoreMentor = GetHandle()->GetMentor(); // The core doesn't addref, no need to release

    // The mentor is, by definition a FrameworkElement so we must always
    // be able to get the mentor for it
    ctl::ComPtr<DependencyObject> spMentor;
    if (pCoreMentor)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pCoreMentor, &spMentor));
    }

    // The mentor is by definition a FrameworkElement
    // convert and handover the reference to the caller
    // TODO: Debug only check to verify this assumption
    *ppMentor = static_cast<FrameworkElement *>(spMentor.Detach());

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::NotifyInheritanceContextChanged(_In_ InheritanceContextChangeKind kind)
{
    return GetHandle()->NotifyInheritanceContextChanged(kind);
}

_Check_return_ HRESULT
DependencyObject::OnInheritanceContextChanged()
{
    HRESULT hr = S_OK;

    // This will notify the local bindings of the new inheritance
    // context.
    if (m_pInheritanceContextChangedEventSource)
    {
        IFC(m_pInheritanceContextChangedEventSource->Raise(this, NULL));
    }

Cleanup:
    RRETURN(hr);
}

IFACEMETHODIMP
DependencyObject::SetValue(
    _In_ xaml::IDependencyProperty* pDP,
    _In_ IInspectable* pValue)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(SetValue(pActualDP, pValue));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::GetValue(
    _In_ xaml::IDependencyProperty* pDP,
    _Out_ IInspectable** ppValue)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(GetValue(pActualDP, ppValue));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::ClearValue(
    _In_ xaml::IDependencyProperty* pDP)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(ClearValue(pActualDP));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::RegisterPropertyChangedCallback(
    _In_ xaml::IDependencyProperty* pDP,
    _In_ xaml::IDependencyPropertyChangedCallback *handler,
    _Out_ INT64 *token)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(RegisterPropertyChangedCallback(pActualDP, handler, token));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::UnregisterPropertyChangedCallback(
    _In_ xaml::IDependencyProperty* pDP,
    _In_ INT64 token)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(UnregisterPropertyChangedCallback(pActualDP, token));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::ReadLocalValue(
    _In_ xaml::IDependencyProperty* pDP,
    _Out_ IInspectable** ppValue)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(ReadLocalValue(pActualDP, ppValue));
    return S_OK;
}

IFACEMETHODIMP
DependencyObject::GetAnimationBaseValue(
    _In_ xaml::IDependencyProperty* pDP,
    _Out_ IInspectable** ppValue)
{
    ARG_NOTNULL_RETURN(pDP, "dp");
    const CDependencyProperty* pActualDP = static_cast<DependencyPropertyHandle*>(pDP)->GetDP();
    IFC_RETURN(GetAnimationBaseValue(pActualDP, ppValue));
    return S_OK;
}

_Check_return_ HRESULT
DependencyObjectFactory::QueryInterfaceImpl(
    _In_ REFIID riid,
    _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(IDependencyObjectFactory)))
    {
        *ppObject = static_cast<IDependencyObjectFactory*>(this);
    }
    else
    {
        return __super::QueryInterfaceImpl(riid, ppObject);
    }

    AddRefOuter();
    RRETURN(S_OK);
}

IFACEMETHODIMP
DependencyObjectFactory::CreateInstance(
    _In_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml::IDependencyObject** ppInstance)
{
    HRESULT hr = S_OK;
    xaml::IDependencyObject* pInstance = NULL;
    IInspectable* pInner = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}

KnownTypeIndex DependencyObjectFactory::GetTypeIndex() const
{
    return KnownTypeIndex::DependencyObject;
}

//+---------------------------------------------------------------------------
//
//  ReferenceTrackerWalk
//
//+--------------------------------------------------------------------------

bool DependencyObject::ReferenceTrackerWalk(EReferenceTrackerWalkType walkType, _In_ bool fIsRoot)
{
    std::list<DependencyObject *>::iterator childReference;
    bool walked = true;

#if DBG
    // This is a debug aid, to enable location-based breakpoints
    TouchDebugMemory();
#endif

    // If we already cleared our weak reference, that means we're no longer reachable, and we should
    // stop walking. When we get resurrected, the weak-reference will be re-connected as well.
    if (!IsAlive()
#if DBG
        && walkType != RTW_TrackerPtrTest
#endif
        )
    {
        return false;
    }

#if DBG
    if( walkType != RTW_TrackerPtrTest && walkType != RTW_Unpeg )
    {
        ReferenceTrackerLogWalk::Log( this, walkType, fIsRoot, walked, true /*start*/ );
    }
#endif

#if DBG
    if (DXamlCore::NotifyEndOfReferenceTrackingOnThread())
    {
        // ReferenceTrackerManager shouldn't be seeing a disconnected object.
        ASSERT(!m_bIsDisconnected);
    }
    else
    {
        // because we dont call NotifyEndOfReferenceTrackingOnThread()
        // it is OK for some RCWs to be connected still.
    }
#endif

    if (fIsRoot)
    {
        ReferenceTrackerManager::SetRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this), nullptr /*callback*/, walkType);
    }

    walked = ReferenceTrackerWalkCore(walkType, fIsRoot);
    if (walked)
    {

        // Delegate ReferenceTrackerWalk to any subclasses or extension.
        ComposingTrackerExtensionWrapper::OnReferenceTrackerWalk(this, walkType);

        // For a composed object, during a Peg or Find walk, only process the outer/controlling object
        // if this isn't the root of the walk.  I.e., we don't need to track references from the outer object
        // to itself.

        if (IsComposed()
            &&
            (!ReferenceTrackerManager::IsRootOfTrackerWalk(ctl::interface_cast<IReferenceTrackerInternal>(this))
            || walkType != RTW_Find))
        {
            // We used to assert IsEnsured for the non-unpeg case, but there's a race where we're running
            // reference-tracking on one thread, while the UI thread is actually in the process but not quite
            // done yet doing an Ensure.
            ComposingTrackerTargetWrapper::ReferenceTrackerWalk(this, walkType);
        }


        // Look for tracker targets in local property values

        if (m_pMapValueTable != nullptr)
        {
            for (EffectiveValueStore::iterator iterValueEntry = m_pMapValueTable->begin();
                 iterValueEntry != m_pMapValueTable->end();
                 iterValueEntry++)
            {
                iterValueEntry->second->ReferenceTrackerWalk(walkType);
            }
        }

        // Look for a tracker target in referenced items

        if (m_pItemReferences != NULL)
        {
            for (childReference = m_pItemReferences->begin(); childReference != m_pItemReferences->end(); childReference++)
            {
                (*childReference)->ReferenceTrackerWalk(walkType);
            }
        }

        // Look for a tracker target in event listeners

        if (m_pEventMap != nullptr)
        {
            for (EventMapping::iterator iterEvent = m_pEventMap->begin();
                 iterEvent != m_pEventMap->end();
                 iterEvent++)
            {
                // Note: CEventSourceBase/CRoutedEventSourceBase do not derive from
                // WeakReferenceSourceNoThreadID to reduce memory cost. Instead they separately define a
                // RefrenceTrackerWalk() method that forwards the walk to source's list of event handlers
                // (which are still WeakReferenceSourceNoThreadID and properly participate).
                iterEvent->second->ReferenceTrackerWalk(walkType);
            }
        }

        // Use core tree to walk references
        CDependencyObject *pDO = GetHandle();
        if (pDO)
        {
            pDO->ReferenceTrackerWalk(
                walkType,
                fIsRoot,
                false);     // shouldWalkPeer = false because peer is being walked, so doesn't need to be walked again
        }
    }

    if (fIsRoot)
    {
        ReferenceTrackerManager::SetRootOfTrackerWalk(nullptr, nullptr, RTW_None);
    }

#if DBG
    if( walkType != RTW_TrackerPtrTest && walkType != RTW_Unpeg )
    {
        ReferenceTrackerLogWalk::Log( this, walkType, fIsRoot, walked, false /*start*/ );
    }
#endif

    return walked;
}


//
// IMPORTANT: Don't use CoreDispatcher in XAML framework code.
//
// CoreDispatcher is exposed through our API for apps to use,
// but we need to avoid taking any internal dependencies on it,
// because it's not available in all environments we support.
//
// Instead, use DirectUI::IDispatcher. This is available through:
//     DXamlCore::GetXamlDispatcher()
//     DependencyObject::GetXamlDispatcher()
//
IFACEMETHODIMP DependencyObject::get_Dispatcher(__RPC__deref_out_opt wuc::ICoreDispatcher** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;

    DXamlCore* pCore = DXamlCore::GetFromDependencyObject(this);
    if (!pCore)
    {
        return S_OK;
    }

    *ppValue = pCore->GetCoreDispatcherNoRef();
    AddRefInterface(*ppValue);

    return S_OK;
}

IFACEMETHODIMP DependencyObject::get_DispatcherQueue(__RPC__deref_out_opt msy::IDispatcherQueue** ppValue)
{
    IFCPTR_RETURN(ppValue);
    *ppValue = nullptr;

    auto cDO = GetHandle();
    if(cDO)
    {
        auto coreServices = cDO->GetContext();
        if (coreServices)
        {
            IXcpHostSite* hostSite = coreServices->GetHostSite();
            if (hostSite)
            {
                auto xcpDispatcher = hostSite->GetXcpDispatcher();
                if (xcpDispatcher)
                {
                    *ppValue = xcpDispatcher->GetDispatcherQueueNoRef();
                    AddRefInterface(*ppValue);
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::EnterImpl(
    _In_ CDependencyObject* nativeDO,
    _In_ CDependencyObject* nativeNamescopeOwner,
    _In_ bool bLive,
    _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bUseLayoutRounding)
{
    HRESULT hr = S_OK;

    DependencyObject* pTargetDO = NULL;

    // We're trying to get rid of the live-enter-walk.
    // BUG 252678: However, due to ordering issues with Loaded and Unloaded events, we're reinstating it for now.
    IFC(DXamlCore::GetCurrent()->TryGetPeer(nativeDO, &pTargetDO));

    if (pTargetDO)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

        IFC(pTargetDO->EnterImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bUseLayoutRounding));

        // Signals the debug tool, if available, that the tree has mutated.
        if (bLive && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
        {
            const auto diagInterop = ::Diagnostics::GetDiagnosticsInterop(true);
            if (diagInterop && diagInterop->IsEnabledForThread(DXamlCore::GetCurrent()->GetThreadId()))
            {
                diagInterop->SignalMutation(pTargetDO, VisualMutationType::Add);
            }
        }
    }

Cleanup:
    ctl::release_interface(pTargetDO);
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::LeaveImpl(
    _In_ CDependencyObject* nativeDO,
    _In_ CDependencyObject* nativeNamescopeOwner,
    _In_ bool bLive, _In_ bool bSkipNameRegistration,
    _In_ bool bCoercedIsEnabled,
    _In_ bool bVisualTreeBeingReset)
{
    HRESULT hr = S_OK;

    DependencyObject* pTargetDO = NULL;

    // NOTE: It is possible that we have reference to an object that is
    // garbage collected but not finalized, so we use TryGetPeerWithInternalRef rather
    // than TryGetPeer

    IFC(DXamlCore::GetCurrent()->TryGetPeerWithInternalRef(nativeDO, &pTargetDO));
    if (pTargetDO)
    {
        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

        // We're trying to get rid of the live-enter-walk.
        // BUG 252678: However, due to ordering issues with Loaded and Unloaded events, we're reinstating it for now.
        IFC(pTargetDO->LeaveImpl(bLive, bSkipNameRegistration, bCoercedIsEnabled, bVisualTreeBeingReset));

        // Signals the debug tool, if available, that the tree has mutated.
        if (bLive && runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::XamlDiagnostics))
        {
            const auto diagInterop = ::Diagnostics::GetDiagnosticsInterop(false);
            if(diagInterop && diagInterop->IsEnabledForThread(DXamlCore::GetCurrent()->GetThreadId()))
            {
                diagInterop->SignalMutation(pTargetDO, VisualMutationType::Remove);
            }
        }
    }

Cleanup:
    ctl::release_interface_inner(pTargetDO);
    RRETURN(hr);
}

_Check_return_ HRESULT DependencyObject::FireEvent(KnownEventIndex nEventId, IInspectable* pSender, IInspectable* pArgs)
{
    HRESULT hr = S_OK;

    IUntypedEventSource* const pEventSource = GetEventSourceNoRef(nEventId);
    if (pEventSource)
    {
        IFC(pEventSource->UntypedRaise(pSender, pArgs));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the specified event should be raised.
//
//------------------------------------------------------------------------
bool
DependencyObject::ShouldRaiseEvent(KnownEventIndex eventID)
{
    return (m_pEventMap && (m_pEventMap->find(eventID) != m_pEventMap->end()));
}

_Check_return_ HRESULT DependencyObject::EventAddPreValidation(_In_ void* const pValue, EventRegistrationToken* const ptToken) const
{
    HRESULT hr = S_OK;

    ARG_VALIDRETURNPOINTER(ptToken);
    ARG_NOTNULL(pValue, "value");
    IFC(CheckThread());

Cleanup:
    return S_OK;
}

_Check_return_ HRESULT DependencyObject::GetEventSourceNoRefWithArgumentValidation(KnownEventIndex nEventIndex, _Outptr_ IUntypedEventSource** ppEventSource)
{
    ARG_VALIDRETURNPOINTER(ppEventSource);

    *ppEventSource = GetEventSourceNoRef(nEventIndex);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the EventSource for the given event ID if it exists in
//      this DO's event map. Returns NULL otherwise.
//
//------------------------------------------------------------------------
_Ret_maybenull_ IUntypedEventSource* DependencyObject::GetEventSourceNoRef(KnownEventIndex nEventIndex)
{
    IUntypedEventSource* pEventSource = nullptr;

    if (m_pEventMap != nullptr)
    {
        EventMapping::iterator iterEvent = m_pEventMap->find(nEventIndex);

        if (iterEvent != m_pEventMap->end())
        {
            pEventSource = iterEvent->second;
            ASSERT(pEventSource != nullptr);
        }
    }

    return pEventSource;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stores the EventSource for the given event ID in this DO's event map.
//      Takes the peer reference lock to prevent modification when the
//      ReferenceTrackerManager is running.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject::StoreEventSource(
    KnownEventIndex nEventIndex,
    _In_ IUntypedEventSource* pEventSource)
{
    // Peer has state because EventSource is being is being set.
    //
    // Do this outside the AutoReentrantReferenceLock because MarkHasState can
    // call ComposingTrackerTargetWrapper::GetTrackerTarget when the core DO sets an
    // expected reference on the peer, which will assert if the lock has been taken.
    IFC_RETURN(MarkHasState());

    {
        AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

        if (m_pEventMap == nullptr)
        {
            m_pEventMap.reset(new EventMapping());
        }

        ASSERT(m_pEventMap->find(nEventIndex) == m_pEventMap->end());
        (*m_pEventMap)[nEventIndex] = pEventSource;
        pEventSource->AddRef();
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes the EventSource for the given event ID from this DO's event map.
//      Takes the peer reference lock to prevent modification when the
//      ReferenceTrackerManager is running.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject::RemoveEventSource(
    KnownEventIndex nEventIndex)
{
    AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

    ASSERT(m_pEventMap);

    EventMapping::iterator iterEvent = m_pEventMap->find(nEventIndex);

    ASSERT(iterEvent != m_pEventMap->end());

    if (iterEvent != m_pEventMap->end())
    {
        IUntypedEventSource* pEventSource = iterEvent->second;
        m_pEventMap->erase(iterEvent);
        ReleaseInterface(pEventSource);

#if XCP_MONITOR
        // Global singletons that are DependencyObjects with events, e.g. Application.DebugSettings,
        // can cause the leak detector to raise a false positive during tests as they will never be 
        // destroyed prior to the leak detector running. To avoid this we'll explicitly free the
        // event map if it is now empty
        if (m_pEventMap->empty())
        {
            m_pEventMap.reset(nullptr);
        }
#endif
    }

    return S_OK;
}

//
// Moves all registered event handlers in <this> to <pTarget>
//
_Check_return_ HRESULT DependencyObject::MoveEventSourcesImpl(_In_ DependencyObject* pTarget)
{
    AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

    if (m_pEventMap == nullptr)
    {
        return S_OK;
    }

    std::vector<KnownEventIndex> events;

    for (EventMapping::iterator iterEvent = m_pEventMap->begin();
        iterEvent != m_pEventMap->end();
        iterEvent++)
    {
        events.push_back(iterEvent->first);
        IFC_RETURN(pTarget->StoreEventSource(iterEvent->first, iterEvent->second));
    }

    for(auto eventIndex: events)
    {
        IFC_RETURN(this->RemoveEventSource(eventIndex));
    }

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::MoveEventSources(_In_ DependencyObject* pTarget)
{
    AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

    IFC_RETURN(this->MoveEventSourcesImpl(pTarget));

    if (this->GetHandle()->OfTypeByIndex<KnownTypeIndex::UIElement>())
    {
        CUIElement* pCoreThis = static_cast<CUIElement*>(this->GetHandle());
        CEventManager* pEventManager = pCoreThis->GetContext()->GetEventManager();
        if (pEventManager)
        {
            CDependencyObject* pCoreTarget = static_cast<CDependencyObject*>(pTarget->GetHandle());
            if (pCoreThis->m_pEventList!=nullptr)
            {
                IFC_RETURN(pEventManager->AddRequestsInOrder(pCoreTarget, pCoreThis->m_pEventList));
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::RestoreEventSources(_In_ DependencyObject* pSource)
{
    AutoReentrantReferenceLock lock(DXamlCore::GetCurrent());

    IFC_RETURN(pSource->MoveEventSourcesImpl(this));

    CDependencyObject* pCoreSource = static_cast<CDependencyObject*>(pSource->GetHandle());
    CEventManager* pEventManager = pCoreSource->GetContext()->GetEventManager();
    if (pEventManager)
    {
        IFC_RETURN(pEventManager->RemoveObject(pCoreSource));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  OnReferenceTrackerWalk override
//
//------------------------------------------------------------------------

void
DependencyObject::OnReferenceTrackerWalk(INT walkType) // override
{
    EReferenceTrackerWalkType walkTypeAsEnum = static_cast<EReferenceTrackerWalkType>(walkType);
    if (m_pDPChangedEventSource)
        m_pDPChangedEventSource->ReferenceTrackerWalk(walkTypeAsEnum);

    if (m_pInheritanceContextChangedEventSource)
        m_pInheritanceContextChangedEventSource->ReferenceTrackerWalk(walkTypeAsEnum);

    if (m_pNotificationVector != nullptr)
    {
        for (auto iter : *m_pNotificationVector)
        {
            iter.second->ReferenceTrackerWalk(walkTypeAsEnum);
        }
    }

    WeakReferenceSourceNoThreadId::OnReferenceTrackerWalk(walkType);
}


//-----------------------------------------------------------------------------
//
// Returns the XAML dispatcher associated with this DO. Can be called from any
// thread.
//
//-----------------------------------------------------------------------------
DirectUI::IDispatcher* DependencyObject::GetXamlDispatcherNoRef()
{
    DXamlCore* pCore = DXamlCore::GetFromDependencyObject(this);
    if (pCore)
    {
        return pCore->GetXamlDispatcherNoRef();
    }
    return NULL;
}

//-----------------------------------------------------------------------------
//
// Returns the XAML dispatcher associated with this DO. Can be called from any
// thread.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT DependencyObject::GetXamlDispatcher(_Out_ ctl::ComPtr<IDispatcher>* pspDispatcher)
{
    HRESULT hr = S_OK;

    DXamlCore* pCore = DXamlCore::GetFromDependencyObject(this);
    IFCCHECK(pCore);

    IFC(pCore->GetXamlDispatcher(pspDispatcher));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyObject::OnCollectionChangedCallback(
    _In_ CDependencyObject* nativeObject,
    _In_ XUINT32 nCollectionChangeType,
    _In_ XUINT32 nIndex)
{
    HRESULT hr = S_OK;

    DependencyObject *pPeer = NULL;

    // Get the native object's peer
    DXamlCore* pCore = DXamlCore::GetCurrent();

    if (nCollectionChangeType == wfc::CollectionChange_Reset)
    {
        // when the collection is being reset it is possible
        // the peer has been finalized
        IFC(pCore->TryGetPeer(nativeObject, &pPeer));
    }
    else
    {
        IFC(pCore->GetPeer(nativeObject, &pPeer));
    }

    if (pPeer)
    {
        IFC(pPeer->OnCollectionChanged(nCollectionChangeType, nIndex));
    }

Cleanup:
    ctl::release_interface(pPeer);

    RRETURN(hr);
}

// Indicates if the given dependency object is a child of this object.
_Check_return_ HRESULT
DependencyObject::IsAncestorOf(
    _In_ DependencyObject* pElement,
    _Out_ BOOLEAN* pIsAncestor)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<IDependencyObject> spAncestor = this;
    ctl::ComPtr<IDependencyObject> spCurrent = pElement;
    ctl::ComPtr<IDependencyObject> spNext = nullptr;

    IFCPTR(pIsAncestor);

    while (spCurrent && spCurrent != spAncestor)
    {
        IFC(VisualTreeHelper::GetParentStatic(spCurrent.Get(), &spNext));
        spCurrent = spNext;
    }

    *pIsAncestor = (spAncestor == spCurrent);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// Set focus on specified element
//
//-----------------------------------------------------------------------------

// static
_Check_return_ HRESULT
DependencyObject::SetFocusedElement(
    _In_ DependencyObject* pElementToFocus,
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_ BOOLEAN* pFocusUpdated,
    _In_ bool isProcessingTab,
    _In_ bool isShiftPressed,
    _In_ bool forceBringIntoView)
{
    xaml_input::FocusNavigationDirection focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_None;
    if (isProcessingTab)
    {
        if (isShiftPressed)
        {
            focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Previous;
        }
        else
        {
            focusNavigationDirection = xaml_input::FocusNavigationDirection::FocusNavigationDirection_Next;
        }
    }

    IFC_RETURN(SetFocusedElementWithDirection(
        pElementToFocus,
        focusState,
        animateIfBringIntoView,
        pFocusUpdated,
        focusNavigationDirection,
        forceBringIntoView));

    return S_OK;
}


_Check_return_ HRESULT
DependencyObject::SetFocusedElementWithDirection(
    _In_ DependencyObject* pFocusedElement,
    _In_ xaml::FocusState focusState,
    _In_ BOOLEAN animateIfBringIntoView,
    _Out_ BOOLEAN* pFocusUpdated,
    _In_ xaml_input::FocusNavigationDirection focusNavigationDirection,
    _In_ bool forceBringIntoView,
    InputActivationBehavior inputActivationBehavior)
{
    ctl::ComPtr<xaml_input::IFocusManagerStaticsPrivate> spFocusManager;

    *pFocusUpdated = FALSE;
    IFC_RETURN(ctl::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Input_FocusManager).Get(),
        &spFocusManager));

    if (spFocusManager)
    {
        IFC_RETURN(spFocusManager->SetFocusedElementWithDirection(
            pFocusedElement,
            focusState,
            animateIfBringIntoView,
            forceBringIntoView,
            focusNavigationDirection,
            (inputActivationBehavior == InputActivationBehavior::RequestActivation),
            pFocusUpdated));
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Get focused element
//
//-----------------------------------------------------------------------------

_Check_return_ HRESULT
DependencyObject::GetFocusedElement(
    _Outptr_result_maybenull_ DependencyObject** ppFocusedElement)
{
    ctl::ComPtr<DependencyObject> spFocusedElement;

    *ppFocusedElement = NULL;

    auto focusManager = VisualTree::GetFocusManagerForElement(GetHandle());
    CDependencyObject* pDO = focusManager->GetFocusedElementNoRef();

    if (pDO != nullptr)
    {
        IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(pDO, &spFocusedElement));
        *ppFocusedElement = spFocusedElement.Detach();
    }

    return S_OK;
}

//static
_Check_return_ HRESULT DependencyObject::OnParentUpdated(
    _In_ CDependencyObject* pChildCore,
    _In_opt_ CDependencyObject* pOldParentCore,
    _In_opt_ CDependencyObject* pNewParentCore,
    _In_ bool isNewParentAlive)
{
    HRESULT hr = S_OK;

    ctl::ComPtr<DependencyObject> spPeer;

    IFC(DXamlCore::GetCurrent()->TryGetPeer(pChildCore, &spPeer));

    if (spPeer)
    {
        IFC(spPeer->OnParentUpdated(pOldParentCore, pNewParentCore, isNewParentAlive));
    }

Cleanup:
    RRETURN(hr);
}

//static
_Check_return_ HRESULT DependencyObject::ReferenceTrackerWalk(
    _In_ CDependencyObject* pCoreDO,
    _In_ DirectUI::EReferenceTrackerWalkType walkType,
    _In_ bool isRoot,
    _Out_ bool *pIsPeerAlive,
    _Out_ bool *pWalked
    )
{
    *pWalked = false;
    *pIsPeerAlive = false;

    // Directly get the peer, because this is called during reference tracker walk.
    // ReferenceTrackerManager::ReferenceTrackingStarted's
    // lock isn't recursive, so TryGetPeer's AutoReentrantReferenceLock will cause a
    // deadlock, because it isn't aware that ReferenceTrackingStarted has taken the lock.
    //$TODO: Investigate this more

    DependencyObject *pPeer = pCoreDO->GetDXamlPeer();
    if (pPeer)
    {
        // During the peg walks, if we're pegged because of m_PegNoRefCoreObjectsWithoutPeers,
        // set a bit to let the ReferenceTrackerWalkCore know about it.
        if (walkType == RTW_Peg && isRoot)
        {
            pPeer->m_referenceTrackerBitFields.peggedByCoreTable = true;
        }

        *pIsPeerAlive = pPeer->IsAlive();
        *pWalked = pPeer->ReferenceTrackerWalk(walkType, isRoot);

    }

    return S_OK;
}

//static
_Check_return_ HRESULT DependencyObject::SetExpectedReferenceOnPeer(
    _In_ CDependencyObject* pCoreDO)
{
    DependencyObject* pDO = nullptr;

    // AddRefForPeerReferenceHelper calls addref_interface_inner.
    // Get an internal ref so that this won't fail for unreachable (GC'd) peers.
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeerWithInternalRef(
                    pCoreDO,
                    &pDO));
    if (pDO)
    {
        pDO->AddRefForPeerReferenceHelper();
    }

    // Match TryGetPeerWithInternalRef with release_interface_inner
    ctl::release_interface_inner(pDO);

    return S_OK;
}

//static
_Check_return_ HRESULT DependencyObject::ClearExpectedReferenceOnPeer(
    _In_ CDependencyObject* pCoreDO)
{
    DependencyObject* pDO = nullptr;

    // ReleaseForPeerReferenceHelper calls release_interface_inner, so
    // ensure that inner unknown is kept alive until peer is released in
    // this method.
    IFC_RETURN(DXamlCore::GetCurrent()->TryGetPeerWithInternalRef(
                    pCoreDO,
                    &pDO));
    if (pDO)
    {
        pDO->ReleaseForPeerReferenceHelper();
    }

    // Match TryGetPeerWithInternalRef with release_interface_inner
    ctl::release_interface_inner(pDO);

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::NotifyDeferredElementStateChangedStatic(
    _In_ CDependencyObject* parent,
    _In_ KnownPropertyIndex propertyIndex,
    _In_ DeferredElementStateChange state,
    _In_ UINT32 collectionIndex,
    _In_ CDependencyObject* realizedElement)
{
    ctl::ComPtr<DependencyObject> parentDO;

    IFC_RETURN(DXamlCore::GetCurrent()->GetPeer(parent, &parentDO));

    IFC_RETURN(parentDO->NotifyDeferredElementStateChanged(
        propertyIndex,
        state,
        collectionIndex,
        realizedElement));

    return S_OK;
}

_Check_return_ HRESULT DependencyObject::NotifyDeferredElementStateChanged(
    _In_ KnownPropertyIndex propertyIndex,
    _In_ DeferredElementStateChange state,
    _In_ UINT32 collectionIndex,
    _In_ CDependencyObject* realizedElement)
{
    // Only one class should handle insertion, so don't call base class.
    return S_OK;
}

namespace DirectUI
{
    // Activation wrappers for type table
    _Check_return_ IActivationFactory* CreateActivationFactory_DependencyObject()
    {
        RRETURN(ctl::ActivationFactoryCreator<DirectUI::DependencyObjectFactory>::CreateActivationFactory());
    }

    // Another function to avoid the pain of folding the two parts together. Some
    // initialization values require deep knowledge of the platform and the core
    // is still somewhat platform agnostic. So helper function time!
    double GetModernDoubleValue(_In_ const int nValue)
    {
        switch (nValue)
        {
        case 0:
            return ModernCollectionBasePanel::GetDefaultCacheLength();

        default:
            return 0.0;
        }
    }
}
