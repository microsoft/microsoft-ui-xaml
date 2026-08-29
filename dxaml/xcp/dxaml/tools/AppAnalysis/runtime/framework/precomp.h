// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <evntrace.h>
#include <evntcons.h>
#include <tdh.h>
#include <objbase.h>

// STL
#include <vector>
#include <memory>

// WRL
#include <inspectable.h>
#include <wrl\module.h>
#include <wrl\wrappers\corewrappers.h>

#include <initguid.h>
#include "namespacealiases.h"
#include "macros.h"
#include "microsoft.diagnostics.appanalysis.h"
#include "microsoft.diagnostics.appanalysis.internal.h"
#include "AppAnalysisETWEventRecord.h"
#include "wil_resource.h"

#include <windowscollections.h>