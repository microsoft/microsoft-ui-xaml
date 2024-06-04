// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include "Unknwn.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "Microsoft.UI.Xaml.h"

// STL includes
#include <functional>
#include <type_traits>
#include <queue>
#include <vector>
#include <unordered_map>

#include "macros.h"
#include "paltypes.h"
#include "pal.h"
#include "perf.h"
#include "core.h"
#include "refcounting.h"
#include "values.h"
#include "core_media.h"
#include "matrix.h"
#include "xcplist.h"
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "real.h"
#include "memutils.h"
#include "xstrutil.h"
#include "ctypes.h"
#include "xcpmath.h"
#include "span.h"
#include "xcpsafe.h"

// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

#include "guiddef.h"

#include "Elements.h"

#include "eventmgr.h"
#include "EventArgs.h"
#include "RoutedEventArgs.h"
#include "ErrorService.h"
#include "RateChangedRoutedEventArgs.h"
#include "xcp_error.h"
#include "ErrorEventArgs.h"

// Compositor
#include "compositor-all.h"

#include "UriValidator.h"
