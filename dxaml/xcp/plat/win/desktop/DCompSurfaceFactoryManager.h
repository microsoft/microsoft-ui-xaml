// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the DCompSurfaceFactoryManager object.
//      This object is mainly responsible for book-keeping SurfaceFactory object
//      across all UI threads.  It maintains a two-level map:
//      1) A mapping from thread ID -> SurfaceFactoryMap (known as the ThreadMap).
//         This allows different UI threads to create SurfaceFactories, even with
//         the same device, and they will be kept separate.
//         Note that only UI threads make requests for SurfaceFactories today, but this
//         class is not aware of which thread is making a request.
//      2) The SurfaceFactoryMap is the 2nd level map, mapping device -> SurfaceFactory.
//         This map allows us to quickly retrieve and reuse the same SurfaceFactory
//         for a given device as the app creates surfaces.

#pragma once

#include <dcompinternal.h>
#include <dcompprivate.h>

struct IDCompositionDesktopDevicePartner;
class DCompTreeHost;
class DCompSurfaceFactory;

class DCompSurfaceFactoryManager
{
public:
    static DCompSurfaceFactoryManager* Instance();

    static void EnsureInitialized();
    static void Deinitialize();

    void GetSurfaceFactoriesForCurrentThread(
        _Inout_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector
    );

    _Check_return_ HRESULT ObtainSurfaceFactory(
        _In_ DCompTreeHost *pDCompTreeHost,
        _In_ IUnknown *pIUnk,
        _In_ IDCompositionDesktopDevicePartner *pDCompDevice,
        _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
        );

    _Check_return_ HRESULT OnSurfaceFactoryDestroyed(
        _In_ DCompSurfaceFactory* pSurfaceFactory
        );

    _Check_return_ HRESULT CleanupSurfaceFactoryMapForCurrentThread();

    _Check_return_ HRESULT FlushAllSurfaceFactoriesWithDevice(_In_ IUnknown *pDevice);

private:
    DCompSurfaceFactoryManager();
    ~DCompSurfaceFactoryManager();

    // The second-level map is declared first, maps device -> SurfaceFactory
    // The device is AddRef'd while it is present in this map, this is to guard against
    // address recycling in case the app releases and creates a new device.
    // The DCompSurfaceFactory is NOT AddRef'd as that would create a circular reference (See OnSurfaceFactoryDestroyed).
    typedef std::pair<IUnknown*, DCompSurfaceFactory*> SurfaceFactoryMapEntry;
    typedef xvector<SurfaceFactoryMapEntry> SurfaceFactoryMap;

    // The first-level map maps thread ID -> SurfaceFactoryMap
    // The SurfaceFactoryMap is NOT AddRef'd as it's an internal object that only lives here.
    typedef std::pair<XUINT32, SurfaceFactoryMap*> ThreadMapEntry;
    typedef xvector<ThreadMapEntry> ThreadMap;

    _Check_return_ HRESULT EnsureSurfaceFactoryMapForCurrentThread(_Outptr_ SurfaceFactoryMap **ppSurfaceFactoryMapNoRef);

    DCompSurfaceFactoryManager::SurfaceFactoryMap* GetSurfaceFactoryMapForCurrentThread();

    wil::critical_section m_Lock;
    ThreadMap m_threadMap;

};
