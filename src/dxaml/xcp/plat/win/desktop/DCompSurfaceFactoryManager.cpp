// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DCompTreeHost.h>
#include <OfferTracker.h>

DCompSurfaceFactoryManager* g_pDCompSurfaceFactoryManager = NULL;

/*static*/
DCompSurfaceFactoryManager* DCompSurfaceFactoryManager::Instance()
{
    return g_pDCompSurfaceFactoryManager;
}

DCompSurfaceFactoryManager::DCompSurfaceFactoryManager()
{
}

DCompSurfaceFactoryManager::~DCompSurfaceFactoryManager()
{
    for (ThreadMap::const_iterator it = m_threadMap.begin();
         it != m_threadMap.end();
         it++)
    {
        // By this point, all DCompSurfaces should be released, which should
        // release all corresponding DCompSurfaceFactory references, which should
        // cause all DCompSurfaceFactoryMap entries to be removed.
        // Hence it would be a bug if we were shutting down the DLL and still
        // had entries present in any of our SurfaceFactoryMap entries.
        ASSERT(it->second->empty());

        // Delete the empty SurfaceFactoryMap entry.
        delete it->second;
    }
    m_threadMap.clear();
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Initializer method, must be called when the XAML DLL is loading
//
//-------------------------------------------------------------------------
void DCompSurfaceFactoryManager::EnsureInitialized()
{
    if (g_pDCompSurfaceFactoryManager == nullptr)
    {
        g_pDCompSurfaceFactoryManager = new DCompSurfaceFactoryManager();
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Deinitializer method, must be called when the XAML DLL is unloading
//
//-------------------------------------------------------------------------
void DCompSurfaceFactoryManager::Deinitialize()
{
    delete g_pDCompSurfaceFactoryManager;
    g_pDCompSurfaceFactoryManager = NULL;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Find or create the SurfaceFactoryMap for the current thread ID,
//      creating one if necessary.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactoryManager::EnsureSurfaceFactoryMapForCurrentThread(
    _Outptr_ SurfaceFactoryMap **ppSurfaceFactoryMapNoRef
    )
{
    HRESULT hr = S_OK;

    SurfaceFactoryMap *pSurfaceFactoryMap = GetSurfaceFactoryMapForCurrentThread();

    if (pSurfaceFactoryMap == nullptr)
    {
        // We couldn't find one, create one now and store in the map
        pSurfaceFactoryMap = new SurfaceFactoryMap;
        IFC(m_threadMap.push_back(ThreadMapEntry(::GetCurrentThreadId(), pSurfaceFactoryMap)));
    }

    *ppSurfaceFactoryMapNoRef = pSurfaceFactoryMap;
    pSurfaceFactoryMap = nullptr;

Cleanup:
    delete pSurfaceFactoryMap;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Find the SurfaceFactoryMap for the current thread ID,
//      if we can not find one, then assign null to the input pointer
//
//-------------------------------------------------------------------------
DCompSurfaceFactoryManager::SurfaceFactoryMap* DCompSurfaceFactoryManager::GetSurfaceFactoryMapForCurrentThread()
{
    XUINT32 currentThreadId = ::GetCurrentThreadId();
    SurfaceFactoryMap *pSurfaceFactoryMap = nullptr;

    // Search the ThreadMap for the SurfaceFactoryMap for this thread
    for (ThreadMap::const_iterator it = m_threadMap.begin();
        it != m_threadMap.end();
        it++)
    {
        if (it->first == currentThreadId)
        {
            pSurfaceFactoryMap = it->second;
            break;
        }
    }

    return pSurfaceFactoryMap;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Find the SurfaceFactories for the current thread, this method
//      is called in DCompTreeHost to acquire all secondary SFs
//-------------------------------------------------------------------------
void DCompSurfaceFactoryManager::GetSurfaceFactoriesForCurrentThread(
    _Inout_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector
)
{
    auto guard = m_Lock.lock();

    SurfaceFactoryMap* pSurfaceFactoryMapNoRef = GetSurfaceFactoryMapForCurrentThread();

    if (pSurfaceFactoryMapNoRef != nullptr)
    {
        for (SurfaceFactoryMap::const_iterator it = pSurfaceFactoryMapNoRef->begin();
            it != pSurfaceFactoryMapNoRef->end();
            it++)
        {
            surfaceFactoryVector->push_back(it->second->GetSurfaceFactoryPartner());
        }
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      For the given device, obtains a SurfaceFactory, creating one if one
//      does not already exist, otherwise returning the SurfaceFactory
//      associated with this device, and associated with the current thread.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactoryManager::ObtainSurfaceFactory(
    _In_ DCompTreeHost *pDCompTreeHost,
    _In_ IUnknown *pIUnk,
    _In_ IDCompositionDesktopDevicePartner *pDCompDevice,
    _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
    )
{
    HRESULT hr = S_OK;

    auto guard = m_Lock.lock();

    DCompSurfaceFactory *pSurfaceFactory = NULL;
    SurfaceFactoryMap *pSurfaceFactoryMapNoRef = NULL;

    // Get the SurfaceFactoryMap for this thread.
    IFC(EnsureSurfaceFactoryMapForCurrentThread(&pSurfaceFactoryMapNoRef));

    // Search the SurfaceFactoryMap for an existing SurfaceFactory for this device
    for (SurfaceFactoryMap::const_iterator it = pSurfaceFactoryMapNoRef->begin();
         it != pSurfaceFactoryMapNoRef->end();
         it++)
    {
        if (it->first == pIUnk)
        {
            // We found one.  AddRef it and return it.
            it->second->AddRef();
            *ppSurfaceFactory = it->second;
            goto Cleanup;
        }
    }

    // If we get here, we could not find an existing SurfaceFactory,
    // create a new one and put it in the map.
    IFC(DCompSurfaceFactory::Create(
        pDCompTreeHost,
        pDCompDevice,
        pIUnk,
        &pSurfaceFactory
        ));

    IFC(pSurfaceFactoryMapNoRef->push_back(SurfaceFactoryMapEntry(pIUnk, pSurfaceFactory)));
    pIUnk->AddRef();

    IFC(pDCompTreeHost->OnSurfaceFactoryCreated(pSurfaceFactory));

    *ppSurfaceFactory = pSurfaceFactory;
    pSurfaceFactory = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactory);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Mechanism for SurfaceFactory to notify us when it is being destroyed.
//      Since we book-keep references to SurfaceFactories in the SurfaceFactoryMap
//      for this thread, this gives us a chance to remove this entry from the map.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactoryManager::OnSurfaceFactoryDestroyed(
    _In_ DCompSurfaceFactory* pSurfaceFactory
    )
{
    HRESULT hr = S_OK;

    auto guard = m_Lock.lock();

    SurfaceFactoryMap *pSurfaceFactoryMap = NULL;

    // Get the SurfaceFactoryMap for this thread.
    IFC(EnsureSurfaceFactoryMapForCurrentThread(&pSurfaceFactoryMap));

    // Search the SurfaceFactoryMap and remove if found.
    for (SurfaceFactoryMap::iterator it = pSurfaceFactoryMap->begin();
         it != pSurfaceFactoryMap->end();
         it++)
    {
        if (it->second == pSurfaceFactory)
        {
            ReleaseInterface(it->first);
            IFC(pSurfaceFactoryMap->erase(it));
            break;
        }
    }

    // Note:  If this is the last SurfaceFactory in the SurfaceFactoryMap,
    // we keep the empty SurfaceFactoryMap around, as we may have to create
    // another one for this thread if another SurfaceFactory is created on it.

Cleanup:
    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Removes all entries from the SurfaceFactory map associated with this thread.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactoryManager::CleanupSurfaceFactoryMapForCurrentThread()
{
    HRESULT hr = S_OK;

    auto guard = m_Lock.lock();

    SurfaceFactoryMap *pSurfaceFactoryMap = NULL;

    // Get the SurfaceFactoryMap for this thread.
    IFC(EnsureSurfaceFactoryMapForCurrentThread(&pSurfaceFactoryMap));
    ASSERT(pSurfaceFactoryMap != NULL);

    for (SurfaceFactoryMap::const_iterator it = pSurfaceFactoryMap->begin();
         it != pSurfaceFactoryMap->end();
         it++)
    {
        // We have taken a ref on the Device, release it.
        it->first->Release();
    }
    pSurfaceFactoryMap->clear();

    // Note:  We keep the empty SurfaceFactoryMap around, as we are very likely
    // to create another SurfaceFactory for this thread (eg when we recover from device lost).

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Flushes all GPU work for all SurfaceFactories created with the given device.
//      This includes SurfaceFactories for other UI threads as well.
//      This includes GPU work done by DComp on the app's behalf (gutters).
//
//-------------------------------------------------------------------------

_Check_return_ HRESULT
DCompSurfaceFactoryManager::FlushAllSurfaceFactoriesWithDevice(_In_ IUnknown *pDevice)
{
    HRESULT hr = S_OK;

    auto guard = m_Lock.lock();

    // Iterate over every SurfaceFactoryMap across all threads
    for (ThreadMap::const_iterator itOuter = m_threadMap.begin();
         itOuter != m_threadMap.end();
         itOuter++)
    {
        // Iterate over all SurfaceFactories
        for (SurfaceFactoryMap::const_iterator itInner = itOuter->second->begin();
             itInner != itOuter->second->end();
             itInner++)
        {
            if (itInner->first == pDevice)
            {
                // Flush!
                IFC(itInner->second->Flush());
            }
        }
    }

Cleanup:
    RRETURN(hr);
}


