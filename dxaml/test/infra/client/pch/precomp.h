// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <winnt.h>
#include <strsafe.h>
#include <initguid.h>

// WRL
#include <wrl\module.h>
#include <wrl\async.h>

#include <windows.foundation.declarations.h>
#include <windows.storage.h>
#include <windows.system.power.h>

// STL
#include <list>
#include <vector>
#include <memory>
#include <map>
#include <functional>
#include <sstream>
#include <unordered_map>
#include <chrono>
#include <cstdint>
#include <type_traits>
#include <unordered_set>
#include <array>
#include <algorithm>
#include <codecvt>
#include <string>

#include <WexTestClass.h>

#include <NamespaceAliasesTest.h>

bool _utilities_IsBVT();
#define COM_GROUP_ISBVT (_utilities_IsBVT())
#include <ComWrapper.h>

#define DUAL_NAMESPACE Microsoft
#include <private.infrastructure.h>
#include <Microsoft.UI.Xaml.private.h>
#include <private.infrastructure.hosting.h>

// Standard Windows error handling helpers.
#include <wil\result_macros.h>
#include <wil\resource.h>

#include <XamlLogging.h>

#define BVT_OUTPUT(fmt, ...) if (COM_GROUP_ISBVT) { LOG_OUTPUT(fmt, __VA_ARGS__); }

