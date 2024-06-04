// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif // _WIN32_WINNT

#include <xcpwindows.h>
#include <objbase.h>  // for STDMETHOD
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.ui.composition.h>

#include <mshtml.h> // for IViewObjectPresentNotifySite
#include "MuiUdk.h"

#include <ole2.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#include <hstring.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <float.h>

#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#define NO_SHLWAPI_GDI
#define NO_SHLWAPI_STRFCNS

#include <shlwapi.h>

#define IMetaData ShellIMetaData // Allow shlobj.h to compile
#include <shlobj_core.h>
#undef IMetaData

#include <evntrace.h>
#include <initguid.h>

#include "sal.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "paltypes.h"
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

#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"

// DirectManipulation
#include "DirectManipulationContainerHandler.h"
#include "DirectManipulationContainer.h"

#include "depends.h"
#include "collectionbase.h"
#include "DOCollection.h"
#include "LayoutStorage.h"
#include "uielement.h"

#include "hwwalk.h"
#include "hwtexturemgr.h"
#include "compositor-all.h"
#include "windowrendertarget.h"

#include "focusmgr.h"

#include "EventArgs.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "PointerEventArgs.h"
#include "InputServices.h"

#include <urlmon.h> // for IUri
#include "winpal.h"
#include "ErrorService.h"
#include "xcp_error.h"
#include "DataStructureFunctionProvider.h"
#include "DataStructureFunctionSpecializations.h"

#include "pixelformatutils.h"

#include "xcpwindow.h"

#include "xcplist.h"

#include <dxgi1_3.h>
#include <dxgi1_2.h>
#include <dxgi.h>

#include "CommonPlatformUtilities.h"
#include "WinPlatformUtilities.h"

#include "xstrutil.h"
#include "xuriutils.h"
#include "XStringUtils.h"

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

#include <intsafe.h>

#if XCP_MONITOR
#include "monitorbuffer.h"
#include "dbghelp.h" // On x86 Windows use the debug help library to obtain stack trace
// Note: dbghelp APIs are ANSI. Although setting a #define changes the API external
// references to W versions, the standard dbghelp.lib doesn't support W versions.
// So for convenience we use the ANSI APIs.
#endif
#include "rendercounters.h"

#include "windowsgraphicsdevicemanager.h"
#include "windowspresenttarget.h"

#ifdef ENABLE_UIAUTOMATION
#include <UIAutomationCore.h>
#include <UIAutomationCoreAPI.h>
#include "UIAWindow.h"
#include "UIAWrapper.h"
#include "UIAPatternProviderWrapper.h"

#include <Richedit.h>
#include <D2d1.h>
#include <D3d11.h>
#include <DWrite_3.h>
#include "textserv.h"
#include "WindowLessSiteHost.h"
#endif

#include "PalPrintingData.h"
#include "WinPrintingData.h"
#include "enumdefs.h"

#include <d3d11.h>
#include <d3d11_1.h>

#include "d3d11SharedDeviceGuard.h"
#include "d3d11device.h"

#include "elements.h"

#include "core_media_native.h"

#include "SystemMemoryBits.h"

#include "RefreshRateInfo.h"

#include "ImageSharingEngineHost.h"

#include <wrl\ftm.h>
#include "d2dprinttarget.h"
#include "d2dprintingdata.h"

// ARI APIs
#include <appmodel.h>

#include "MsResourceHelpers.h"
#include "UriXStringGetters.h"

#include "Dwmapi.h"

#include "ModernResourceProvider.h"
#include "CommonResourceProvider.h"
#include "ApplicationDataProvider.h"
#include "BasePALResource.h"
#include "FilePathResource.h"

#include "DCompSurface.h"
#include "DCompSurfaceMonitor.h"
#include "DCompSurfaceFactoryManager.h"
#include "DCompInteropCompositorPartnerCallback.h"
#include "ImageUtils.h"

#include "D2DUtils.h"

#include <XamlBehaviorMode.h>

#include "MUX-ETWEvents.h"
