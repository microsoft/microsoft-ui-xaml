// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xcpwindows.h"
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include "Microsoft.UI.Xaml.h"

// Standard Windows error handling helpers.
#include <wil\Result.h>

// STL includes
#include <functional>
#include <type_traits>
#include <queue>
#include <vector>
#include <unordered_map>

#include "xcperror.h"
#include "macros.h"
#include "xcpdebug.h"
#include "pal.h"
#include "core.h"
#include "refcounting.h"
#include "values.h"
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
#include "elements.h"
#include "eventmgr.h"
#include "pixelformatutils.h"
#include "host.h"
#include <weakref_ptr.h>

// COM helpers
#include "ComMacros.h"
#include "ComTemplates.h"
#include "ComBase.h"
#include "ComUtils.h"
#include "ComEventHandlerTraits.h"
#include "ComPtr.h"
#include "ComEventHandler.h"

#include "compositor-all.h"

// XAML

#include "host.h"
#include "elements.h"
#include "TextBoxView.h"
#include "TextBoxBase.h"
#include "TextBox.h"
#include "PasswordBox.h"
#include "RichEditBox.h"
#include "TextBoxBaseAutomationPeer.h"
#include "RichEditBoxAutomationPeer.h"
#include "TextBoxAutomationPeer.h"
#include "PasswordBoxAutomationPeer.h"
#include "image.h"

#include "focusmgr.h"
#include "eventmgr.h"
#include "eventargs.h"
#include "rendertargetbitmapmgr.h"
#include "routedeventargs.h"
#include "ExceptionRoutedEventArgs.h"
#include "RateChangedRoutedEventArgs.h"
#include "NetworkStatusEventArgs.h"
#include "textchangedeventargs.h"
#include "InputPointEventArgs.h"
#include "DownloadProgressEventArgs.h"
#include "keyboardeventargs.h"
#include "CharacterReceivedeventargs.h"
#include "ErrorService.h"
#include "xcp_error.h"
#include "ErrorEventArgs.h"
#include "ParserErrorEventArgs.h"
#include "SizeChangedEventArgs.h"
#include "RenderingEventArgs.h"
#include "RenderedEventArgs.h"
#include "ContextMenuEventArgs.h"
#include "DMContent.h"
#include "DMViewport.h"
#include "UIDMContainerHandler.h"
#include "InputServices.h"
#include "InteractionManager.h"
#include "InputPaneHandler.h"
#include "ParametricCurve.h"
#include "pixelformatutils.h"
#include "cdeployment.h"
#include "application.h"
#include "text.h"
#include "ChangedPropertyEventArgs.h"
#include "IsEnabledChangedEventArgs.h"
#include "DragEventArgs.h"
#include "HyperlinkClickEventArgs.h"

#include "timer.h"

#include "downloadrequestinfo.h"

#include "compositor-all.h"

#include "StaggerFunctions.h"

#include "PointerEventArgs.h"
#include "TappedEventArgs.h"
#include "DoubleTappedEventArgs.h"
#include "HoldingEventArgs.h"
#include "RightTappedEventArgs.h"
#include "ManipulationEventArgs.h"
#include "ManipulationStartedEventArgs.h"
#include "ManipulationDeltaEventArgs.h"
#include "ManipulationCompletedEventArgs.h"
#include "ManipulationStartingEventArgs.h"
#include "ManipulationInertiaStartingEventArgs.h"
#include "TextControlPasteEventArgs.h"
#include "TextControlCopyingToClipboardEventArgs.h"
#include "TextControlCuttingToClipboardEventArgs.h"
#include "SvgImageSourceOpenedEventArgs.h"
#include "SvgImageSourceFailedEventArgs.h"

#include <perf.h>

#include <UIDMContainer.h>

// Needed because of including xcptypes
#include "hwwalk.h"
#include "ICustomResourceLoader.h"
#include "MsResourceHelpers.h"
#include "CInputScope.h"

#include "ImageReloadManager.h"
#include "AutomationPeerCollection.h"

#include "bezierd.h"
#include "bezierflattener.h"
#include "figuretask.h"
#include "shapedata.h"
#include "cpen.h"
#include "strokefigure.h"

#include "HitTestHelper.h"

#include "MUX-ETWEvents.h"
