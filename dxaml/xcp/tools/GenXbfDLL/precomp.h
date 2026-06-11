// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
// STL
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <unordered_set>
#include <type_traits>
#include <queue>
#include <memory>
#include <array>

#include <strsafe.h>

// Windows

#pragma warning(push)
#pragma warning(disable : 4005)
#define NTDDI_VERSION NTDDI_WIN8 // Hack: Targetting Win7 in sources file, but need Win8 definitions.
#define _WIN32_WINNT _WIN32_WINNT_WINTHRESHOLD // Hack: Targettign Win7 in sources file, but need Threshold definitions
#pragma warning(pop)

// Forward declare a version of TrackerHandle that's compatible with our stubs
typedef void* TrackerHandle;

#include <xcpwindows.h>
#include <DWrite_3.h>
#include <xmllite.h>
#include <wil\result.h>

#include <inspectable.h>
#include <activation.h>
#include <shobjidl_core.h>

#include <windows.storage.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.h>

#include <windows.ui.core.h> // CoreWindow
#include <windows.applicationmodel.core.h> // CoreApplication
#include <windows.applicationmodel.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <ShellScalingApi.h>

// AgCore

#undef GetClassName

#undef CDECL // CDECL is defined as nothing in windef.h, preventing paltypes.h from defining it as __cdecl.

#define NO_XCP_NEW_AND_DELETE
#define NO_XCP_NEW_AND_DELETE_INPLACE
#include "macros.h"
#include "xcpdebug.h"

#include "pal.h"
#include "DataStructureFunctionProvider.h"
#include "core.h"
#include "values.h"
#include "core_media.h"

#include <urlmon.h> // for IUri
#include "winpal.h"
#include "ApplicationDataProvider.h"

#include "DataStructureFunctionSpecializations.h"
#include "xref_ptr.h"
#include "xvector.h"
#include "xlist.h"
#include "xstack.h"
#include "xqueue.h"
#include "xmap.h"

#include "ParserTypeDefs.h"
#include "TypeBits.h"
#include "ThemeWalkResourceCache.h"
#include "corep.h"
#include "depends.h"
#include "real.h"
#include "memutils.h"
#include "xstrutil.h"
#include "ctypes.h"
#include "xcpmath.h"
#include "ErrorService.h"

#include "XamlParserIncludes.h"

#include "MemoryStreamBuffer.h"

#include "Pixelformatutils.h"

#include "Host.h"
#include "ControlBase.h"

#include <windows.applicationmodel.datatransfer.dragdrop.core.h>
#include "JupiterWindow.h"

// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

// DXaml

#include "JupiterControl.h"
#include "DXamlInstanceStorage.h"

#include "DXamlTypes.h"
#include "ComTemplates.h"

#include "comMacros.h"
#include "comTemplateLibrary.h"
#include "comUtils.h"
#include "ComObjectBase.h"
#include "ComObject.h"
#include "comModule.h"
#include "ReferenceTrackerExtension.h"

// Undefine conflicting names temporarily.
#undef IBinding
#undef IDependencyObject
#undef Created
#undef BitmapCreateOptions
#undef GetCurrentTime

#include "Microsoft.UI.Xaml.h"
#include "Microsoft.UI.Xaml.private.h"
#include "synonyms.h"
#include "synonyms.g.h"

#include "CStaticLock.h"
#include "MetadataAPI.h"
#include "ComPtr.h"
#include "AutoPeg.h"
#include "TrackerTargetReference.h"
#include "TrackerPtr.h"
#include "WeakReferenceSource.h"
#include "LifetimeUtils.h"
#include "ComEventHandlerTraits.h"
#include "stubs.h"
#include "XamlParserCallbacks.h"
#include "DependencyPropertyHandle.h"
#include "OleAuto.h"
#include "XbfParserErrorService.h"
#include "XamlMetadataProviderStub.h"
#include "EnumDefs.h"

#include "MUX-ETWEvents.h"

