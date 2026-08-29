#pragma once

#include "windows.h"
#include "shellscalingapi.h"
#include "dwmapi.h"
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include <wil/stl.h>
#include <stdexcept>
#include <strsafe.h>
#include <optional>
#include <algorithm>

#include "MonitorData.h"
#include "MiscUser32.h"
#include "RegistryHelpers.h"
#include "CurrentMonitorTopology.h"

#ifdef USE_VIRTUAL_DESKTOP_APIS
#include "shobjidl.h"
#include "shobjidl_core.h"
#include <wrl.h>
#include "VirtualDesktopIds.h"
#endif

#include "WindowActions.h"
#include "PlacementEx.h"
