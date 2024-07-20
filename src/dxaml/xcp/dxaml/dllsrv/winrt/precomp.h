// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Important:
//  <new> needs to be first to ensure correct linkage of new/delete operators.
//  If we dont do this, the delete of a vector new[] will be generated as scalar delete instead of a vector delete.
//  This will fail chk builds and can potentially lead to heap corruption.

#include <new>

////////////////////////////////////////////////////////////////////////
// Windows headers
//
#include <xcpwindows.h>
#include <ole2.h>
#include <oleauto.h>
#include <shlwapi.h>

#include <objbase.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>

#include <inspectable.h>
#include <activation.h>
#include <shobjidl_core.h>

#include <appmodel.h>

#include <initguid.h>

#include <mfmediaengine.h>

// STL includes
#include <functional>
#include <map>
#include <vector>
#include <deque>
#include <list>
#include <queue>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <memory>
#include <type_traits>
#include <utility>
#include <array>
#include <random>

////////////////////////////////////////////////////////////////////////
// WinRT headers
//

#include <wrl\async.h>
#include <windows.foundation.collections.h>
#include <windows.ui.viewmanagement.h>
#include <windows.foundation.h>
#include <robuffer.h>

#include <windows.ui.core.h> // CoreWindow
#include <windows.applicationmodel.core.h> // CoreApplication
#include <windows.applicationmodel.h>
#include <windows.applicationmodel.background.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <windows.globalization.h>
#include <corewindow.h> // ICoreWindowInterop
#include <windows.ui.core.corewindowfactory.h> // ICoreWindowFactory
#include <UIAutomationCore.h>
#include <UIAutomationCoreAPI.h>

////////////////////////////////////////////////////////////////////////
// Silverlight Core headers
//

#undef CDECL // CDECL is defined as nothing in windef.h, preventing paltypes.h from defining it as __cdecl.

#include "macros.h"
#include "xcpdebug.h"

#include "pal.h"
#include "core.h"
#include "EnumDefs.g.h"
#include "host.h"
#include "values.h"
#include "matrix.h"
#include "xcplist.h"
// Alias Created enum value to deal with conflicting Windows enum value (AsynInfo.h)
#define Created Xcp_Created
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "memutils.h"
#include "rendertypes.h"
#include "xcpmath.h"
#include "elements.h"
#include "TextBoxView.h"
#include "TextBoxBase.h"
#include "TextBox.h"
#include "PasswordBox.h"
#include "RichEditBox.h"
#include "staggerfunctions.h"
#include "xstrutil.h"
#include "Indexes.g.h"
#include "TypeTableStructs.h"
#include "application.h"
#include "real.h"
#include "Pointer.h"
#include "ImageReloadManager.h"
#include "ParametricCurve.h"
#include "ErrorContext.h"

// Hosting layer
#include <strsafe.h>

#include "ControlBase.h"
#include "Values.h"
#include "PendingMessages.h"
#include "Xcpwindow.h"
#include "winpal.h"
#include "xstrutil.h"
#include <windows.applicationmodel.datatransfer.dragdrop.core.h>
#include "JupiterWindow.h"
#include "ResourceManager.h"
#include "JupiterControl.h"
#include "UiaWindow.h"
#include "DXamlInstanceStorage.h"

// Simple COM objects
#include "ComTemplates.h"

#include "DXamlTypes.h"

// COM implementation templates
#include "comMacros.h"
#include "comModule.h"
#include "comTemplateLibrary.h"
#include "comUtils.h"
#include "ComObjectBase.h"
#include "ComObject.h"

////////////////////////////////////////////////////////////////////////
// DXaml headers
//

// Undefine conflicting names temporarily.
#undef Created
#undef GetCurrentTime

#include "Microsoft.UI.Xaml.h"
#include "Microsoft.UI.Xaml.private.h"
#include "Microsoft.UI.Xaml.Media.DXInterop.h"
#include "synonyms.h"
#include "synonyms.g.h"

#include "DXamlServices.h"

#include "comEventHandlerTraits.h"
#include "ComPtr.h"
#include "AutoPeg.h"
#include "TrackerTargetReference.h"
#include "TrackerPtr.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "WeakReferenceSource.h"
#include "comEventHandler.h"

#include "GITHelper.h"
#include "FTMEventSource.h"
#include "StaticStore.h"
#include "EventCallbacks.h"
#include "DependencyObject.h"
#include "DirectManipulationTypes.h"
#include "FrameworkEventArgs.h"
#include "JoltClasses.h"
#include "AutoReentrantReferenceLock.h"

#include "MUX-ETWEvents.h"
