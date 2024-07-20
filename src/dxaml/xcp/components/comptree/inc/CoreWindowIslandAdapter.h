// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <fwd/windows.ui.core.h>
#include <fwd/windows.ui.composition.h>

// CONTENT-TODO: If this is about OneCoreTransforms on OneCore, it all needs to be removed.  This
// will be completely replaced by the OneCore version of CoreWindowSiteBridge instead.  The private
// APIs on CoreWindow are not available for Lifted Xaml.


// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
class CoreWindowIslandAdapter final
{
public:
    CoreWindowIslandAdapter(_In_ wuc::ICoreWindow* const coreWindow, _In_ WUComp::ICompositor* const compositor);
    ~CoreWindowIslandAdapter();

    WUComp::ICompositionTarget* GetCompositionTargetNoRef() const { return m_compositionTarget.Get(); }
    WUComp::ICompositionIsland* GetCompostionIslandNoRef() const { return m_coreWindowIslandCI.Get(); }

private:
    void CreateCompositionIsland(_In_ WUComp::ICompositor* const compositor);

    wrl::ComPtr<WUComp::ICompositionTarget> m_compositionTarget;

    // These are all QI'd from CoreWindow. We store these pointers merely
    // for convenience
    wrl::ComPtr<WUComp::IVisualTreeIsland> m_coreWindowIslandVTI;
    wrl::ComPtr<WUComp::ICompositionIsland> m_coreWindowIslandCI;

// lifted comp - ICoreWindow_CompositionIslands works with system visuals, and we have a lifted visual
// Task 23878262: Move lifted Xaml to islands for real
//  wrl::ComPtr<wuc::ICoreWindow_CompositionIslands> m_coreWindowCI;
};
#endif
