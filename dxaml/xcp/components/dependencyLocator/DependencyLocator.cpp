// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <DependencyLocator.h>
#include <ExternalDependency.h>

namespace DependencyLocator {
    namespace Internal {

        LocalDependencyStorage& LocalDependencyStorage::Instance()
        {
            auto storage = GetDependencyLocatorStorage();
            return storage.Get();
        }

        ProtectedResource<DependencyMap> LocalDependencyStorage::GetMap()
        {
            return ProtectedResource<DependencyMap>(m_mapMutex, m_map);
        }

        ProtectedResource<DependencyActivatorMap> LocalDependencyStorage::GetActivatorMap()
        {
            return ProtectedResource<DependencyActivatorMap>(m_activatorMutex, m_activatorMap);
        }

        bool LocalDependencyStorage::IsInitialized() const
        {
            return m_initialized;
        }

        void LocalDependencyStorage::MarkInitialized()
        {
            m_initialized = true;
        }

        void LocalDependencyStorage::InitializeTls()
        {
            if (m_tlsIndex == TLS_OUT_OF_INDEXES)
            {
                m_tlsIndex = TlsAlloc();
            }

            if (m_tlsIndex == TLS_OUT_OF_INDEXES)
            {
                XAML_FAIL_FAST();
            }
        }

        DependencyMap* LocalDependencyStorage::EnsureThreadInitialized()
        {
            ASSERT(m_tlsIndex != TLS_OUT_OF_INDEXES);
            DependencyMap* pStorageNoRef = static_cast<DependencyMap*>(TlsGetValue(m_tlsIndex));

            if (pStorageNoRef == nullptr)
            {
                pStorageNoRef = new DependencyMap();
                TlsSetValue(m_tlsIndex, pStorageNoRef);
#ifdef DBG
                m_initializedThreads++;
#endif
            }
            return pStorageNoRef;
        }

        void LocalDependencyStorage::Reset()
        {
            {
                auto map = GetMap();
                map.Get().clear();
            }

#ifdef DBG
            // In Jupiter it's not obvious what all the different threading contexts are. We think
            // we understand it pretty well, but we'll ASSERT if we missed uninitializing one to be
            // sure we're not leaking. 
            if (m_initializedThreads != 0)
            {
                LOG(L"--------------------------------------------------------------------------------------------");
                LOG(L"WARNING: Poorly written test code likely called a UI-threaded method from a non-UI thread.");
                LOG(L"If this warning is seen while not executing a legacy DRT it should be properly investigated.");
                LOG(L"--------------------------------------------------------------------------------------------");
            }

            //ASSERT(m_initializedThreads == 0);
#endif

            auto activatorMap = GetActivatorMap();
            activatorMap.Get().clear();
            m_initialized = false;

            if (m_tlsIndex != TLS_OUT_OF_INDEXES)
            {
                TlsFree(m_tlsIndex);
            }

            m_tlsIndex = TLS_OUT_OF_INDEXES;
        }

        void LocalDependencyStorage::UninitializeThread()
        {
            if (m_tlsIndex != TLS_OUT_OF_INDEXES)
            {
                Internal::DependencyMap* pStorageNoRef = static_cast<Internal::DependencyMap*>(TlsGetValue(m_tlsIndex));
                if (pStorageNoRef)
                {
                    delete pStorageNoRef;
                    TlsSetValue(m_tlsIndex, nullptr);
#ifdef DBG
                    m_initializedThreads--;
#endif
                }
            }
        }
    }

    void UninitializeProcess()
    {
        Internal::LocalDependencyStorage::Instance().Reset();
        Internal::ExternalDependencyStorage::Instance().Get().Reset();
    }

    void InitializeProcess()
    {
        Internal::LocalDependencyStorage::Instance().InitializeTls();
    }

    void UninitializeThread()
    {
        Internal::LocalDependencyStorage::Instance().UninitializeThread();
    }
}
