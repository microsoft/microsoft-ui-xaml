// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <inspectable.h>
#include <wrl\wrappers\corewrappers.h>
#include <wrl\module.h>
#include <stdio.h>
#include <tchar.h>
#include <objbase.h>
#include <strsafe.h>
#include <initguid.h>

// STL
#include <memory>
#include <string>
#include <vector>
#include <functional>

#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <shlwapi.h> 
#include <XmlLite.h>
#include "namespacealiases.h"
#include "macros.h"
#include "microsoft.diagnostics.appanalysis.h"
#include "microsoft.diagnostics.appanalysis.internal.h"
#include "AppAnalysisETWEventRecord.h"