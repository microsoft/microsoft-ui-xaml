// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//
// PlacementEx for WinUI's automatic window placement.
// We may want to keep these files in sync with other copies later.
// WinUI changes are wrapped in #ifdef WINUI_PLACEMENTEX, with the original code in the #else.
// See docs/design-notes/window-placement.md.

#pragma once

#include "windows.h"
#include "shellscalingapi.h"
#include "dwmapi.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <wil/stl.h>
#ifdef WINUI_PLACEMENTEX
// <stdexcept> is only needed by PlacementEx::FromString, which WinUI compiles out.
#else
#include <stdexcept>
#endif
#include <strsafe.h>
#include <optional>
#include <algorithm>

#include "MonitorData.h"
#include "MiscUser32.h"
#ifdef WINUI_PLACEMENTEX
// WinUI doesn't use the registry helpers or the monitor topology cache.
#else
#include "RegistryHelpers.h"
#include "CurrentMonitorTopology.h"
#endif

#ifdef USE_VIRTUAL_DESKTOP_APIS
#include "shobjidl.h"
#include "shobjidl_core.h"
#include <wrl.h>
#include "VirtualDesktopIds.h"
#endif

#if !defined(WINUI_PLACEMENTEX) || defined(WINUI_PLACEMENTEX_LIGHTUP)
#include "WindowActions.h"
#endif
#include "PlacementEx.h"
