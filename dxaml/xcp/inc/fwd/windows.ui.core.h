// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Windows {
namespace UI {
namespace Core {
    class AcceleratorKeyEventArgs;
    class AutomationProviderRequestedEventArgs;
    class BackRequestedEventArgs;
    class CharacterReceivedEventArgs;
    class ClosestInteractiveBoundsRequestedEventArgs;
    class ContextMenuRequestedEventArgs;
    class CoreAcceleratorKeys;
    class CoreComponentInputSource;
    class CoreCursor;
    class CoreDispatcher;
    class CoreWindow;
    class CoreWindowDialog;
    class CoreWindowEventArgs;
    class CoreWindowFlyout;
    class CoreWindowPopupShowingEventArgs;
    class CoreWindowResizeManager;
    class IdleDispatchedHandlerArgs;
    class InputEnabledEventArgs;
    class KeyEventArgs;
    class PointerEventArgs;
    class SystemNavigationManager;
    class TouchHitTestingEventArgs;
    class VisibilityChangedEventArgs;
    class WindowActivatedEventArgs;
    class WindowSizeChangedEventArgs;
    enum CoreCursorType : int;
    enum CoreInputDeviceTypes : unsigned int;
    enum CoreVirtualKeyStates : unsigned int;
    enum CoreWindowActivationMode : int;
    enum CoreWindowActivationState : int;
    enum NavigationReason : int;
    interface IAcceleratorKeyEventArgs;
    interface IAcceleratorKeyEventArgs2;
    interface IAutomationProviderRequestedEventArgs;
    interface IBackRequestedEventArgs;
    interface ICharacterReceivedEventArgs;
    interface IClosestInteractiveBoundsRequestedEventArgs;
    interface ICoreAcceleratorKeys;
    interface ICoreClosestInteractiveBoundsRequested;
    interface ICoreComponentFocusable;
    interface ICoreCursor;
    interface ICoreCursorFactory;
    interface ICoreDispatcher;
    interface ICoreDispatcher2;
    interface ICoreDispatcherWithTaskPriority;
    interface ICoreInputSourceBase;
    interface ICoreKeyboardInputSource;
    interface ICoreKeyboardInputSource2;
    interface ICorePointerInputSource;
    interface ICorePointerInputSource2;
    interface ICorePointerRedirector;
    interface ICoreTouchHitTesting;
    interface ICoreWindow;
    interface ICoreWindow2;
    interface ICoreWindow3;
    interface ICoreWindow4;
    interface ICoreWindow5;
    interface ICoreWindowDialog;
    interface ICoreWindowDialogFactory;
    interface ICoreWindowEventArgs;
    interface ICoreWindowFlyout;
    interface ICoreWindowFlyoutFactory;
    interface ICoreWindowPopupShowingEventArgs;
    interface ICoreWindowResizeManager;
    interface ICoreWindowResizeManagerLayoutCapability;
    interface ICoreWindowResizeManagerStatics;
    interface ICoreWindowStatic;
    interface ICoreWindowWithContext;
    interface IDispatchedHandler;
    interface IIdleDispatchedHandler;
    interface IIdleDispatchedHandlerArgs;
    interface IInitializeWithCoreWindow;
    interface IInputEnabledEventArgs;
    interface IKeyEventArgs;
    interface IKeyEventArgs2;
    interface IPointerEventArgs;
    interface ISystemNavigationManager;
    interface ISystemNavigationManager2;
    interface ISystemNavigationManagerStatics;
    interface ITouchHitTestingEventArgs;
    interface IVisibilityChangedEventArgs;
    interface IWindowActivatedEventArgs;
    interface IWindowSizeChangedEventArgs;
    struct CorePhysicalKeyStatus;
    struct CoreProximityEvaluation;

    interface IContextMenuRequestedEventArgs;
} // Core

} // UI
} // Windows
XAML_ABI_NAMESPACE_END
