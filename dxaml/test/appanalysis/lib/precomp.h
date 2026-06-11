// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <strsafe.h>

// WRL
#include <inspectable.h>
#include <wrl\module.h>
#include <wrl\wrappers\corewrappers.h>

// STL
#include <functional>
#include <memory>
#include <map>
#include <vector>

#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>

#include <initguid.h>
#include "NamespaceAliases.h"
#include "macros.h"
#include "microsoft.diagnostics.appanalysis.h"
#include "microsoft.diagnostics.appanalysis.internal.h"

#include "MUX-ETWEvents.h"

#include "wex.common.h"
#include "wexstring.h"