// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include <rpcsal.h>

#pragma once

namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Graphics {
    class SharedDeviceGuardUnitTests;
} } } } }

// RAII class for locking the critical section inside a shared CD3D11DeviceInstance. The instance methods on
// CD3D11DeviceInstance expect the caller to have taken the CS first, and require a CD3D11SharedDeviceGuard
// to be passed in as proof that the caller owns the lock.
class CD3D11SharedDeviceGuard
{
    friend class CD3D11DeviceInstance;
    friend class Windows::UI::Xaml::Tests::Graphics::SharedDeviceGuardUnitTests;

public:
    CD3D11SharedDeviceGuard()
    {
    }

    bool HasLock() const { return lockguard != nullptr; }
    bool LockMatches(_In_ const wil::critical_section* cs) const { return m_pCriticalSection == cs; }

private:
#pragma prefast(suppress: 26165 "m_locktaken ensures that if we take the lock we will release it in the destructor")
    _Acquires_shared_lock_(m_pCriticalSection->m_cs)
    void TakeLock(_In_ wil::critical_section* cs)
    {
        m_pCriticalSection = cs;
        lockguard = m_pCriticalSection->lock();
    }

private:
    wil::cs_leave_scope_exit lockguard = nullptr;
    wil::critical_section* m_pCriticalSection { nullptr };

    // Disable copy and force onto stack
    CD3D11SharedDeviceGuard(const CD3D11SharedDeviceGuard& other);
    CD3D11SharedDeviceGuard& operator=(CD3D11SharedDeviceGuard& other);
    static void* operator new(size_t);
    static void operator delete(void*);
    static void* operator new[](size_t);
    static void operator delete[](void*);
};
