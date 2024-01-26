// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// This precompiled header file is shared between all libs that build in the
// components directory. It should ONLY include headers that exist at a layer
// below Jupiter or that are so fundemental to Jupiter that it's impossible to
// do anything without them. Basic Windows headers and STL headers are fair game, as
// are any parts of the PAL we haven't removed.

// Do NOT use this file as a place to toss the random Jupiter header of the week.
// Do NOT use this file as a place to include a single header file that will
// likely only be used by a handful of components.

// Not having XCP_MONITOR defined identically everywhere will ruin your day. It
// will result in different-sized objects and spooky memory issues. To ensure
// it's always properly defined in components we DO place it in the precompiled
// header.
#ifdef _PREFAST_
#define XCP_MONITOR 0
#else
#ifndef XCP_MONITOR
#if DBG
#define XCP_MONITOR 1
#else
#define XCP_MONITOR 0
#endif
#endif
#endif

// Important:
//  <new> needs to be first to ensure correct linkage of new/delete operators.
//  If we dont do this, the delete of a vector new[] will be generated as scalar delete instead of a vector delete.
//  This will fail chk builds and can potentially lead to heap corruption.
#include <new>

#include <xcpwindows.h>
#include <strsafe.h>

#include <stdint.h>

// STL includes
#include <utility>
#include <functional>
#include <vector>
#include <algorithm>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <limits>
#include <array>
#include <queue>
#include <chrono>

// Standard Windows error handling helpers.
#include <wil/Result.h>

#pragma warning(push)
#pragma warning(disable : 4296)
#include <wil/safecast.h>
#pragma warning(pop)

#include <wil/result.h>

// WinRT includes
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.declarations.h>

// XAML includes
#include <XamlStructCollectionDeclarations.h>
#include <Microsoft.UI.Xaml.h>
#include <Microsoft.UI.Xaml.controls.controls.h>
#include <Microsoft.UI.Xaml.private.h>

// IXP includes
#include <Microsoft.UI.h>

// Set of min jupiter headers for building lightweight libs without
// deep system dependencies.
#include <macros.h>
#include <minxcptypes.h>
#include <minerror.h>
#include <minpal.h>
#include <mindebug.h>

// Isolated base set of Jupiter components.
#include <vector_map.h>
#include <vector_set.h>
#include <ctypes.h>
#include <xcpmath.h>
#include <xref_ptr.h>
#include <weakref_ptr.h>
#include <xstring_ptr.h>
#include <xstringmap.h>
#include <xstack.h>
#include <xmap.h>

// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

// Common
#include "NamespaceAliases.h"

// CXcpObjectBase (for easy use with xref_ptr)
#include "refcounting.h"

#include <wrl/wrappers/corewrappers.h>
// Undefine these silly xcpmath macros.
#undef min
#undef max

#include <dcompinternal.h>
#include <dcompprivate.h>
