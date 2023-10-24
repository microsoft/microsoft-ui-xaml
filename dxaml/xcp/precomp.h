// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif

#include <xcpwindows.h>
#include <objbase.h>  // for STDMETHOD

#include <ole2.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <hstring.h>

#include <utility>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>

#include "sal.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "paltypes.h"
#include <xref_ptr.h>
#include "values.h"
#include "ctypes.h"
#include "core.h"
#include "refcounting.h"
#include "host.h"
#include "perf.h"
#include "matrix.h"
#include "xcpsafe.h"
#include "real.h"
#include "xcpmath.h"

#include <urlmon.h> // for IUri
#include "winpal.h"
#include "ErrorService.h"
#include "xcp_error.h"
#include "DataStructureFunctionProvider.h"
#include "DataStructureFunctionSpecializations.h"

#include "frameCounter.h"
#include "PendingMessages.h"
#include "xcpwindow.h"

#include "xcplist.h"

#include "CommonPlatformUtilities.h"
#include "WinPlatformUtilities.h"
#include "hal.h"
#include "download.h"
#include "windwnreq.h"
#include "commonBrowserHost.h"
#include "xstrutil.h"
#include "xuriutils.h"
#include "XStringUtils.h"
#include "XStringBuilder.h"

// MapView size = 512k
#define MEDIASTREAM_PAGE_MAP_SIZE 524288
// Minimum size to use Http11StreamBuffer = 10Megs
#define MEDIASTREAM_MIN_SIZE 10240000
// Initial allocation of the mapping file object = 20 MB
#define MEDIASTREAM_MAPPING_SIZE 20971520
//Minimum size to use chuncked memory buffer = 1 MB
#define NONBUFFEREDSTREAM_MIN_SIZE 1024000
#include "memorystream.h"
#include "memorystreambuffer.h"
#include "winstream.h"

#include "AsyncDownloadRequestManager.h"
#include "DownloadRequest.h"
#include "AbortableAsyncDownload.h"

#include "WinBrowserHost.h"

#if XCP_MONITOR
#include "monitorbuffer.h"
#include "dbghelp.h" // On x86 Windows use the debug help library to obtain stack trace
// Note: dbghelp APIs are ANSI. Although setting a #define changes the API external
// references to W versions, the standard dbghelp.lib doesn't support W versions.
// So for convenience we use the ANSI APIs.
#endif
#include "rendercounters.h"


#include "windowspresenttarget.h"
#include "Networkingutilities.h"

#include "downloadrequestinfo.h"

#ifdef ENABLE_UIAUTOMATION
#include "UIAutomationCore.h"
#include "UIAutomationCoreAPI.h"
#include "UIAWindow.h"
#include "UIAWrapper.h"
#include "UIAPatternProviderWrapper.h"
#endif

#include "PalPrintingData.h"
#include "WinPrintingData.h"
#include "enumdefs.h"

#include "UIUtils.h"

#include <dwmapi.h>

// pal\win
#include <DWrite_3.h>

#include "MUX-ETWEvents.h"
