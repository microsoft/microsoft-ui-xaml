// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <xcpwindows.h>
#include <ole2.h>
#include <oleauto.h>

#include <objbase.h>
#include <stdlib.h>
#include <malloc.h>
#include <tchar.h>

#include <inspectable.h>
#include <activation.h>
#include <shobjidl_core.h>

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

#include <wrl\async.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.foundation.h>
#include <windows.foundation.collections.h>
#include <windows.foundation.declarations.h>
#include <windows.ui.viewmanagement.h>
#include <robuffer.h>

#include <windows.applicationmodel.h>
#include <windows.security.cryptography.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <windows.globalization.h>
#include <UIAutomationCore.h>
#include <UIAutomationCoreAPI.h>

// WinUI Details ABI headers (Lifted UDK)
// UDK types are required by generated files (ContentDialog.g.h, ComboBox.g.h, etc.)
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

// Debug logger
#include "DebugOutput.h"

#include "UtilityFunctions.h"

#include <Experimental.h>

#include "MUX-ETWEvents.h"

#include <FeatureFlags.h>

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
#include "TextBlockAutomationPeer.h"
#include "RichTextBlockAutomationPeer.h"
#include "RichTextBlockOverflowAutomationPeer.h"
#include "SplitView.h"
#include "SplitViewTemplateSettings.h"

#include "image.h"
#include "MediaPlayerPresenter.h"
#include "MediaPlayerElement.h"

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
#include "SvgImageSourceFailedEventArgs.h"
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
#include "pixelformatutils.h"
#include "cdeployment.h"
#include "application.h"
#include "text.h"
#include "ChangedPropertyEventArgs.h"
#include "IsEnabledChangedEventArgs.h"
#include "DragEventArgs.h"
#include "HyperlinkClickEventArgs.h"

#include "triggers.h"
#include "CKeyboardAcceleratorCollection.h"
#include "CKeyboardAccelerator.h"
#include "storyboard.h"
#include "timemgr.h"
#include "easingfunctions.h"
#include "KeyFrame.h"
#include "DoubleAnimation.h"
#include "DoubleKeyFrame.h"
#include "ColorAnimation.h"
#include "ColorAnimationUsingKeyFrames.h"
#include "ColorKeyFrame.h"
#include "ObjectAnimationUsingKeyFrames.h"
#include "ObjectKeyFrame.h"
#include "PointAnimation.h"
#include "PointKeyFrame.h"
#include "PointerAnimationUsingKeyFrames.h"
#include "PointerKeyFrame.h"
#include "DynamicTimeline.h"
#include "KeySpline.h"
#include "KeyTime.h"
#include "TimelineCollection.h"
#include "RepeatBehavior.h"
#include <CColor.h>
#include <LinearGradientBrush.h>
#include <XamlCompositionBrush.h>
#include <XamlLight.h>
#include <XamlLightCollection.h>
#include <TextRange.h>
#include <TextRangeCollection.h>
#include <TextHighlighter.h>
#include <TextHighlighterCollection.h>
#include <ValidationErrorsCollection.h>
#include "UIElementWeakCollection.h"

#include "Transform3D.h"
#include "CompositeTransform3D.h"
#include "PerspectiveTransform3D.h"
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

// Needed because of including xcptypes
#include "hwwalk.h"
#include "ICustomResourceLoader.h"
#include "MsResourceHelpers.h"
#include "CInputScope.h"

#include "LoadedImageSourceLoadCompletedEventArgs.h"
#include "LoadedImageSurface.h"
#include "ImageReloadManager.h"
#include "AutomationPeerCollection.h"
#include "AutomationAnnotationCollection.h"
#include "AutomationPeerAnnotationCollection.h"
#include "DOCollection.h"
#include "VisualStateCollection.h"
#include "SoftwareBitmapSource.h"
#include "SvgImageSource.h"
#include "definitioncollection.h"
#include <GeneratedClasses.g.h>
#include "SemanticZoom.h"
#include "TypeNamePtr.h"

#include "ThemeShadow.h"
#include "SystemBackdrop.h"

#include "PropertyTransitions.h"
#include "ValidationCommand.h"
#include "MenuFlyoutSubItem.h"

#include "CWindowChrome.h"
