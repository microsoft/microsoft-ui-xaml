// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CStaticLock.h"

using namespace DirectUI;

CRITICAL_SECTION g_csStatic;
bool CStaticLock::s_fStaticCSInitialized = false;

// Called from DllMain when our DLL is loaded. This does lightweight init
// of a synchronization primitive needed to support a global StaticStore singleton.
_Check_return_ HRESULT DirectUI::StaticLockGlobalInit()
{
    if (0 == InitializeCriticalSectionAndSpinCount(&g_csStatic, 0x80000001))
    {
        return E_FAIL;
    }

    CStaticLock::s_fStaticCSInitialized = true;
    return S_OK;
}

// Called from DllMain when our DLL is unloaded. This does lightweight deinit
// of a synchronization primitive used by a global StaticStore singleton.
void DirectUI::StaticLockGlobalDeinit()
{
    if (CStaticLock::IsInitialized())
    {
        DeleteCriticalSection(&g_csStatic);
        CStaticLock::s_fStaticCSInitialized = false;
    }
}

CStaticLock::CStaticLock()
{
    Lock();
}

void CStaticLock::Lock()
{
    ASSERT(CStaticLock::IsInitialized());
    EnterCriticalSection(&g_csStatic);
}

CStaticLock::~CStaticLock()
{
    Unlock();
}

void CStaticLock::Unlock()
{
    LeaveCriticalSection(&g_csStatic);
}

bool CStaticLock::IsOwnedByCurrentThread()
{
    return static_cast<unsigned long>(reinterpret_cast<uintptr_t>(g_csStatic.OwningThread)) == static_cast<unsigned long>(GetCurrentThreadId());
}

bool CStaticLock::IsLocked()
{
    return g_csStatic.OwningThread != 0;
}

bool CStaticLock::IsInitialized()
{
    return CStaticLock::s_fStaticCSInitialized;
}