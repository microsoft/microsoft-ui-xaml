// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "WindowsGraphicsDeviceManager.h"
#include <PalWorkItem.h>
#include <DCompTreeHost.h>
#include <MUX-ETWEvents.h>

// Blocks, if necessary, to wait for completion of the initialize work item.  Performs this blocking in a thread-safe manner.
_Check_return_ HRESULT WindowsGraphicsDeviceManager::WaitForInitializationThreadCompletion()
{
    wil::unique_handle  initializationThreadHandle;

    // Fast path:  Usually m_initializationThreadHandle is already NULL.
    // Optimistically check for NULL first, avoids taking a lock.
    if (m_initializationThreadHandle != NULL)
    {
        // Take a lock here to avoid racing on release.
        auto guard = m_InitializeWorkItemLock.lock();

        // m_initializationThreadHandle may have been released between
        // first NULL check and taking the lock.  Perform a 2nd NULL check.
        if (m_initializationThreadHandle != NULL)
        {
            IFCW32_RETURN(DuplicateHandle(GetCurrentProcess(), m_initializationThreadHandle, GetCurrentProcess(), &initializationThreadHandle, 0 /*desiredaccess*/, FALSE/*inherithandle*/, DUPLICATE_SAME_ACCESS));
        }
    }

    // At this point we either took another reference on m_initializationThreadHandle
    // or it was already released.  Only block if the work item is not NULL.
    if (initializationThreadHandle != NULL)
    {
        TraceCreateAcceleratedGraphicsBegin();
        IFCCHECK_RETURN(WaitForSingleObject(initializationThreadHandle.get(), INFINITE) != WAIT_FAILED);
        TraceCreateAcceleratedGraphicsEnd();
    }
    ASSERT(m_initializationThreadHandle == NULL);

    // If off-thread device init failed, report the failure here on the UI thread.
    IFC_RETURN(m_hrInitializeWorkItem);

    return S_OK;
}

// Ensures that the graphics device wrapper is created.
//
// The pattern is that the device manager _does not_ wait for resource creation automatically
// when you try to access an underlying resource that hasn't been initialized yet. It's up to
// the caller to ensure that they do not access these resources until the 'safe' point where
// WaitForD3DDependentResourceCreation has been called.
//
// Anyone adding calls to this API should keep in mind that calling this too early in the start-
// up path will block the UI thread on this expensive work, removing the benefit of doing
// the work off-thread.
_Check_return_ HRESULT WindowsGraphicsDeviceManager::WaitForD3DDependentResourceCreation()
{
    IFC_RETURN(WaitForInitializationThreadCompletion());

    // Ensure the WinRT Composition device is created.
    ASSERT(m_pDCompTreeHost != nullptr);
    IFC_RETURN(m_pDCompTreeHost->EnsureResources());

    return S_OK;
}

// Use this function to ensure we've created the DComp device.
// *Note carefully* that this does NOT guarantee creation of the D3D device or main SurfaceFactory,
// particularly, in a device lost situation.  Use with care - if you need to create any D3D device-dependent
// resources, use WaitForD3DDependentResourceCreation() instead.
_Check_return_ HRESULT WindowsGraphicsDeviceManager::EnsureDCompDevice()
{
    ASSERT(m_pDCompTreeHost != nullptr);

    IFC_RETURN(m_pDCompTreeHost->EnsureDCompDevice());

    return S_OK;
}


