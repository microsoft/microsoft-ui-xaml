// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <strsafe.h>
#include <tdh.h>
#include <evntrace.h>
#include <evntcons.h>
#include <wil\result.h>

// STL
#include <functional>
#include <vector>
#include <memory>
#include <map>

// WRL
#include <inspectable.h>
#include <wrl\module.h>
#include <wrl\wrappers\corewrappers.h>

#include <initguid.h>
#include "namespacealiases.h"

#include "microsoft.diagnostics.appanalysis.h"
#include "microsoft.diagnostics.appanalysis.internal.h"
#include "macros.h"
#include "RuleTriggeredEventArgs.h"

#include "resource.h"

#include "MUX-ETWEvents.h"

#include <windowscollections.h>