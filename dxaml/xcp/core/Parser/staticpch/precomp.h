// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "Microsoft.UI.Xaml.h"
#include <wil\result.h>

// STL includes
#include <functional>
#include <type_traits>
#include <queue>
#include <vector>
#include <unordered_map>
#include <functional>
#include <array>
#include <memory>

#include "xcperror.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"

// Data structures and smart pointers
void* __CRTDECL operator new(size_t _Size, _Writable_bytes_(_Size) void* _Where) noexcept;

#include "DataStructureFunctionProvider.h"
#include "DataStructureFunctionSpecializations.h"
#include "xref_ptr.h"
#include "xvector.h"
#include "xlist.h"
#include "xstack.h"
#include "xqueue.h"
#include "xmap.h"

#include "ParserTypeDefs.h"
#include "TypeBits.h"
#include "corep.h"
#include "memutils.h"
#include "xcpmath.h"
#include "elements.h"
#include "ErrorService.h"

#include "ParserStringConstants.h"
#include "XamlParserIncludes.h"


// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

#include "MUX-ETWEvents.h"

