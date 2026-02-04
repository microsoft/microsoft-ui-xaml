// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "SharedDeviceGuardUnitTests.h"
#include "D3D11SharedDeviceGuard.h"

using namespace Windows::UI::Xaml::Tests::Graphics;
using namespace wil;

wil::critical_section g_cs;

bool IsD3D11SharedDeviceGuardLocked()
{
    bool result = false;

    // This is a bit silly.  We must spin up another thread to check the status
    // of the lock because a CRITICAL_SECTION can be acquired recursively.
    auto thread = std::thread([&result]()
        {
            // The lock is public so we can directly access it to check
            auto lock = g_cs.try_lock();
            result = !static_cast<bool>(lock);
        });
    thread.join();
    return result;
}

/* ------------------------------------------------------------------------------------------
Shared device guard unit tests
-------------------------------------------------------------------------------------------*/
#pragma prefast(suppress: 26135 "We mock out the win critical section, so it really doesn't take a lock")
void SharedDeviceGuardUnitTests::SharedDeviceGuard()
{
    {
        CD3D11SharedDeviceGuard guard;
        VERIFY_IS_FALSE(IsD3D11SharedDeviceGuardLocked());
    }
    {
        CD3D11SharedDeviceGuard guard;
        guard.TakeLock(&g_cs);
        VERIFY_IS_TRUE(IsD3D11SharedDeviceGuardLocked());
    }
    VERIFY_IS_FALSE(IsD3D11SharedDeviceGuardLocked());
}
