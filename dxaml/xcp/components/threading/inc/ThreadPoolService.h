// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.system.threading.h>

// In order to be free-threaded, the wrl::Callback must implement FtmBase (stands for FreeThreadedMarshaller)
template <typename T>
using FreeThreaded = wrl::Implements<wrl::RuntimeClassFlags<wrl::ClassicCom>, T, wrl::FtmBase>;

class ThreadPoolService final
{
public:
    static ThreadPoolService& GetInstance()
    {
         // Safe because static initialization
        static ThreadPoolService *instance = nullptr;
        static INIT_ONCE s_InitOnce = INIT_ONCE_STATIC_INIT;

        InitOnceExecuteOnce(&s_InitOnce, InitOnceCallback, &instance, nullptr);
        return *instance;
    }

    // Detach global COM objects to prevent shutdown crashes on unloaded modules
    static void DetachFactories()
    { 
        if (s_hasInstance)
        {
            GetInstance().m_spThreadPoolFactory.Detach();
            GetInstance().m_spThreadPoolTimerFactory.Detach();
        }
    }

    // Since this is a singleton, caller can use the interfaces without incurring
    // the cost of ref counting.  However, caller should never release the object without
    // also doing an AddRef.
    const wrl::ComPtr<wsyt::IThreadPoolStatics>& GetThreadPoolFactory() { return m_spThreadPoolFactory; }
    const wrl::ComPtr<wsyt::IThreadPoolTimerStatics>& GetThreadPoolTimerFactory() { return m_spThreadPoolTimerFactory; }

private:
    static BOOL CALLBACK InitOnceCallback(
        _Inout_      PINIT_ONCE pInitOnce,
        _Inout_opt_  PVOID param,
        _Out_opt_    PVOID* pContext)
    {
        static ThreadPoolService instance;
        *static_cast<ThreadPoolService**>(param) = &instance;
        return TRUE;
    }

    ThreadPoolService();
    ThreadPoolService(const ThreadPoolService&) = delete;
    ThreadPoolService& operator=(const ThreadPoolService&) = delete;

    static bool s_hasInstance;
    wrl::ComPtr<wsyt::IThreadPoolStatics> m_spThreadPoolFactory;
    wrl::ComPtr<wsyt::IThreadPoolTimerStatics> m_spThreadPoolTimerFactory;
};