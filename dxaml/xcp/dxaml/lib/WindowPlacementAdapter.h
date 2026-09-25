// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "WindowPlacementPolicy.h"

// Keep the engine's layout and optional helpers identical in product and test builds.
#define USE_VIRTUAL_DESKTOP_APIS
#define PLACEMENTEX_STRING_SERIALIZATION 0
#include <User32Utils.h>

namespace DirectUI::WindowPlacementPersistence
{
    // Checked value translation only. Neither operation queries or changes a window.
    // Outputs are replaced only on success. Native flag values stay out of NativeRequest.
    bool TryCreateEnginePlacement(const NativeRequest& request, PlacementEx& placement);

    // Reads an engine working value, including edits by MoveToMonitor/Cascade. This is
    // not post-SetPlacement capture: that requires GetPlacement and WinUI state caches.
    bool TryReadEnginePlacement(const PlacementEx& placement, NativeRequest& request);

    // Placement policy geometry is routed through these engine operations. The topology
    // overload is a deterministic test seam; production callers provide live monitor data.
    bool TryCreateEnginePlacement(
        const Snapshot& snapshot,
        bool allowSizing,
        PlacementEx& placement);
    bool TrySelectEngineMonitor(
        const Snapshot& snapshot,
        const Topology& topology,
        MonitorDescription& monitor);
    bool TryMoveEngineToMonitor(
        PlacementEx& placement,
        const MonitorDescription& monitor);
    bool TryCascadeEngine(
        PlacementEx& placement,
        int32_t offset);

}
