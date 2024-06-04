// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "Microsoft.UI.Xaml.h"

// STL includes
#include <functional>
#include <type_traits>
#include <queue>
#include <vector>
#include <unordered_map>
#include <array>

// Windows Internal Library (WIL) headers
#include <wil\Result.h>
#include <wil\resource.h>

#include <d3d11.h>

#include "macros.h"
#include "pal.h"
#include "perf.h"
#include "core.h"
#include "refcounting.h"
#include "values.h"
#include "core_media.h"
#include "matrix.h"
#include "xcplist.h"
#include "corep.h"
#include "DependencyObjectTraits.g.h"
#include "rendertypes.h"
#include "real.h"
#include "memutils.h"
#include "xstrutil.h"
#include "ctypes.h"
#include "xcpmath.h"
#include "brushtypeutils.h"
#include "span.h"
#include "xcpsafe.h"
#include "ImageReloadManager.h"
#include "elements.h"
#include "TextBoxView.h"
#include "TextBox.h"
#include "PasswordBox.h"
#include "image.h"
#include "sort.h"
#include "focusmgr.h"
#include "eventmgr.h"
#include "EventArgs.h"
#include "rendertargetbitmapmgr.h"
#include "RoutedEventArgs.h"
#include "InputPointEventArgs.h"
#include "RightTappedEventArgs.h"
#include "RateChangedRoutedEventArgs.h"
#include "keyboardeventargs.h"
#include "ContextMenuEventArgs.h"
#include "ErrorService.h"
#include "xcp_error.h"
#include "ErrorEventArgs.h"
#include "DownloadProgressEventArgs.h"
#include "ParserErrorEventArgs.h"
#include "ChangedPropertyEventArgs.h"
#include "IsEnabledChangedEventArgs.h"
#include "PointerEventArgs.h"
#include "InputServices.h"
#include "staggerfunctions.h"
#include "text.h"
#include "DragEventArgs.h"
#include "TappedEventArgs.h"
#include "CInputScope.h"
#include "SemanticZoom.h"

#include "CoreAsyncAction.h"

#include <TrimWhitespace.h>
#include <StringConversions.h>

// SW includes
#include "KnownColors.h"
#include "pixelformatutils.h"
#include "bezierd.h"
#include "bezierflattener.h"
#include "figuretask.h"
#include "shapedata.h"
#include "cpen.h"
#include "strokefigure.h"
#include "application.h"
#include "cdeployment.h"

// TODO: Need to figure out solution to UIA dependency in several classes
#include "host.h"

// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

const XFLOAT ARC_AS_BEZIER = 0.55228475f; // (sqrt(2) - 1) * 4/3

#include "visualstategroupcollection.h"

#include "INamescope.h"
#include "TemplateNamescope.h"

// Compositor

#include "compositor-all.h"

// TODO: Need to figure out solution to DM dependency in UIElement
// Touch/Manipulation-related includes
#include "DMContent.h"
#include "DMViewport.h"
#include "DirectManipulationContainerHandler.h"
#include "DirectManipulationContainer.h"
#include "UIDMContainer.h"
#include "UIDMContainerHandler.h"

// HW includes

#include "hw-all.h"
#include "DCompTreeHost.h"
#include "DCompSurface.h"

#include "DependencyObjectWrapper.h"

#include "ICustomResourceLoader.h"

#include "TransformGeometrySink.h"
#include "GeometryGroupSink.h"
#include "HitTestHelper.h"
#include "PointHitTestHelper.h"
#include "PolygonHitTestHelper.h"
#include "HitTestGeometrySink.h"
#include "PointHitTestGeometrySink.h"
#include "PolygonHitTestGeometrySink.h"
#include "GeometryBoundsHelper.h"
#include "BoundsGeometrySink.h"

#include "UriValidator.h"
#include "winerror.h"

#include "UIAEnums.h"
#include "AutomationEventsHelper.h"
#include "AutomationPeerCollection.h"

#include "MUX-ETWEvents.h"
#include "SizeUtil.h"
#include <Microsoft.UI.Xaml.private.h>
