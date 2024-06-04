// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

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
#include <chrono>
#include <mutex>

// Standard Windows error handling helpers.
#include <wil\Result.h>

////////////////////////////////////////////////////////////////////////
// WinRT headers
//
#include <wrl\wrappers\corewrappers.h>
#include <HStringUtil.h>

#include <wrl\async.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.declarations.h>
#include <windows.ui.viewmanagement.h>
#include <robuffer.h>

#include <windows.system.h>
#include <microsoft.ui.input.h>
#include <windows.applicationmodel.h>
#include <windows.security.cryptography.h>
#include <windows.applicationmodel.background.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <windows.globalization.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreAPI.h>
#include <windows.ui.composition.interop.h>
#include <Microsoft.UI.Input.InputKeyboardSource.Interop.h>

// Required by generated headers (SplitView.g.h, AppBar.g.h, etc.)
#include <FrameworkUdk/BackButtonIntegration.h>

////////////////////////////////////////////////////////////////////////
// Silverlight Core headers
//

#undef CDECL // CDECL is defined as nothing in windef.h, preventing paltypes.h from defining it as __cdecl.

#include "macros.h"
#include "xcpdebug.h"

#include "pal.h"
#include "core.h"
#include "EnumDefs.g.h"
#include "values.h"
#include "matrix.h"
// Alias Created enum value to deal with conflicting Windows enum value (AsynInfo.h)
#define Created Xcp_Created
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "xcpmath.h"
#include "elements.h"
#include "Indexes.g.h"
#include "TypeTableStructs.h"
#include "real.h"
#include "ParametricCurve.h"
#include "ErrorContext.h"

#include "Storyboard.h"
#include "PointerAnimationUsingKeyFrames.h"
#include "Timer.h"
#include "Duration.h"

// Simple COM objects
#include "ComTemplates.h"

#include "DXamlTypes.h"

// COM implementation templates
#include "ComMacros.h"
#include "ComModule.h"
#include "ComTemplateLibrary.h"
#include "ComUtils.h"
#include "ComObjectBase.h"
#include "ComObject.h"

////////////////////////////////////////////////////////////////////////
// DXaml headers
//

// Undefine conflicting names temporarily.
#undef Created
#undef GetCurrentTime

#include "XamlStructCollectionDeclarations.h"
#include "Microsoft.UI.Xaml.h"
#include "Microsoft.UI.Xaml.private.h"
#include "Microsoft.UI.Xaml.Media.DXInterop.h"
#include "synonyms.h"
#include "synonyms.g.h"
#include "NamespaceAliases.h"

#include "DXamlServices.h"

#include "comEventHandlerTraits.h"
#include "ComPtr.h"
#include "AutoPeg.h"
#include "TrackerTargetReference.h"
#include "TrackerPtr.h"
#include "WeakReferenceSourceNoThreadId.h"
#include "WeakReferenceSource.h"
#include "LifetimeUtils.h"
#include "comEventHandler.h"
#include "EventCallbacks.h"

#include "comInstantiation.h"

#include "InternalStructs.h"
#include "MetadataAPI.h"
#include "ActivationAPI.h"
#include "CoreImports.h"
#include "ErrorInfo.h"
#include "ErrorHelper.h"
#include "TrackerPtrWrapper.h"
#include "EffectiveValueEntry.h"
#include "value.h"
#include "DependencyObject.h"
#include "FrameworkEventArgs.h"
#include "JoltClasses.h"
#include "DependencyPropertyHandle.h"
#include "ValueBoxer.h"
#include "JoltCollections.h"
#include "TrackerCollections.h"
#include "DXamlCore.h"
#include "CustomProperty.h"
#include "LayoutInformation.h"
#include "DoubleUtil.h"
#include "RectUtil.h"
#include "TimelineTimer.h"
#include "Matrix_Partial.h"
#include "Matrix3D_Partial.h"
#include "Duration_Partial.h"
#include "RepeatBehavior_Partial.h"
#include "Uri.h"
#include "TypeNamePtr.h"
#include "LayoutBoundsChangedHelper.h"

// Debug logger
#include "DebugOutput.h"

#include "UtilityFunctions.h"

#include <Experimental.h>

#include "MUX-ETWEvents.h"

#include <FeatureFlags.h>
