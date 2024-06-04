// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CoreWindowIslandAdapter.h"
#include <microsoft.ui.composition.internal.h>

#if defined(XAML_USE_PUBLIC_SDK)
#include <windows.ui.core.h>
#endif // defined(XAML_USE_PUBLIC_SDK)

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false

CoreWindowIslandAdapter::CoreWindowIslandAdapter(_In_ wuc::ICoreWindow* const coreWindow, _In_ WUComp::ICompositor* const compositor)
{
    wrl::ComPtr<wuc::ICoreWindow> spCoreWindow(coreWindow);

// lifted comp - ICoreWindow_CompositionIslands works with system visuals, and we have a lifted visual
// Task 23878262: Move lifted Xaml to islands for real
//  IFCFAILFAST(spCoreWindow->QueryInterface(IID_PPV_ARGS(&m_coreWindowCI)));

    CreateCompositionIsland(compositor);
}

CoreWindowIslandAdapter::~CoreWindowIslandAdapter()
{
    // Explicitly Dispose() the IslandTarget, which will recycle the CoreWindow and enable creating
    // a new IslandTarget.  (This internally calls DestroyCompositionIsland).
    wrl::ComPtr<wf::IClosable> closable;

    FAIL_FAST_IF_FAILED(m_compositionTarget.As(&closable));
    closable->Close();

    m_compositionTarget = nullptr;

    m_coreWindowIslandCI = nullptr;
    m_coreWindowIslandVTI = nullptr;

    // *** TODO: Verify that Target.Close is recycling and remove this line ***
#if false
    IFCFAILFAST(m_coreWindowCI->DestroyCompositionIsland());
#endif

// lifted comp - ICoreWindow_CompositionIslands works with system visuals, and we have a lifted visual
// Task 23878262: Move lifted Xaml to islands for real
//  m_coreWindowCI = nullptr;
}

void CoreWindowIslandAdapter::CreateCompositionIsland(_In_ WUComp::ICompositor* const compositor)
{
    // Create a target for the current view. This should handle creating a target for both
    // a top-level application and a component. Should be called on same thread that created
    // the core window
    //IFCFAILFAST(compositor->CreateTargetForCurrentView(&m_compositionTarget));

// lifted comp - ICoreWindow_CompositionIslands works with system visuals, and we have a lifted visual
// Task 23878262: Move lifted Xaml to islands for real
//    IFCFAILFAST(m_coreWindowCI->get_CompositionIsland(&m_coreWindowIslandVTI));
//    IFCFAILFAST(m_coreWindowIslandVTI.As(&m_coreWindowIslandCI));
}

#endif
