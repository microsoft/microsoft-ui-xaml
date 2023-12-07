// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define INITGUID

#include <xcpwindows.h>
#include <oleauto.h>
#include <synchapi.h>

#include <objbase.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <hstring.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>

#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"
#include "refcounting.h"
#include "host.h"
#include <xlist.h>
#include "hal.h"
#include "winpal.h"
#include "xstrutil.h"
#include "XCPList.h"
#include "values.h"
#include "PendingMessages.h"
#include "frameCounter.h"
#include "commonBrowserHost.h"
#include "AsyncDownloadRequestManager.h"

#ifdef ENABLE_UIAUTOMATION
#include <UIAutomation.h>
#include "UIAWindow.h"
#include "UIAWrapper.h"
#include "UIAPatternProviderWrapper.h"
#include "SafeArrayUtil.h"

// These includes are required for AutomationPeerCollection.h
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "depends.h"
#include "xcpmath.h"
#include "collectionbase.h"
#include "DOCollection.h"
#include "AutomationPeer.h"
#include "AutomationPeerCollection.h"

#endif // ENABLE_UIAUTOMATION

#include "ReentrancyGuard.h"

#include "MUX-ETWEvents.h"
