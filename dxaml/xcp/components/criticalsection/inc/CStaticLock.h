// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // Called from DllMain when our DLL is loaded. This does lightweight init
    // of a synchronization primitive needed to support a global MetadatStore singleton.
    _Check_return_ HRESULT StaticLockGlobalInit();

    // Called from DllMain when our DLL is unloaded. This does lightweight deinit
    // of a synchronization primitive and releases the memory used by a global
    // StaticStore singleton.
    void StaticLockGlobalDeinit();

    class CStaticLock
    {
    friend HRESULT StaticLockGlobalInit();
    friend void StaticLockGlobalDeinit();

    public:
        CStaticLock();
        ~CStaticLock();

        static bool IsOwnedByCurrentThread();
        static bool IsLocked();
        static bool IsInitialized();

    private:

        static void Lock();
        static void Unlock();

        // prohibit copying and allocating on the heap
        CStaticLock(const CStaticLock&) = delete;
        CStaticLock& operator=(const CStaticLock&) = delete;
        static void* operator new(size_t) = delete;
        static void operator delete(void*) = delete;
        static void* operator new[](size_t) = delete;
        static void operator delete[](void*) = delete;

        static bool s_fStaticCSInitialized;
    };
}
