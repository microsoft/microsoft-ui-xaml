// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Forward declarations for external headers.  Please use in header files instead of declaring manually.

#include <abi/xaml_abi.h>

XAML_ABI_NAMESPACE_BEGIN
namespace Microsoft {
namespace UI {
namespace Xaml {
namespace Input {
    class AccessKeyDisplayDismissedEventArgs;
    class AccessKeyDisplayRequestedEventArgs;
    class AccessKeyInvokedEventArgs;
    class AccessKeyManager;
    class CanExecuteRequestedEventArgs;
    class CharacterReceivedRoutedEventArgs;
    class ContextRequestedEventArgs;
    class DoubleTappedRoutedEventArgs;
    class ExecuteRequestedEventArgs;
    class FindNextElementOptions;
    class FocusedElementRemovedEventArgs;
    class FocusManager;
    class FocusManagerGotFocusEventArgs;
    class FocusManagerLostFocusEventArgs;
    class FocusMovementResult;
    class GettingFocusEventArgs;
    class HoldingRoutedEventArgs;
    class InertiaExpansionBehavior;
    class InertiaRotationBehavior;
    class InertiaTranslationBehavior;
    class InputScope;
    class InputScopeName;
    class KeyboardAccelerator;
    class KeyboardAcceleratorInvokedEventArgs;
    class KeyRoutedEventArgs;
    class LosingFocusEventArgs;
    class ManipulationCompletedRoutedEventArgs;
    class ManipulationDeltaRoutedEventArgs;
    class ManipulationInertiaStartingRoutedEventArgs;
    class ManipulationPivot;
    class ManipulationStartedRoutedEventArgs;
    class ManipulationStartingRoutedEventArgs;
    class NoFocusCandidateFoundEventArgs;
    class Pointer;
    class PointerRoutedEventArgs;
    class ProcessKeyboardAcceleratorEventArgs;
    class RightTappedRoutedEventArgs;
    class StandardUICommand;
    class TappedRoutedEventArgs;
    class XamlUICommand;
    enum FocusInputDeviceKind : int;
    enum FocusNavigationDirection : int;
    enum InputScopeNameValue : int;
    enum KeyboardAcceleratorPlacementMode : int;
    enum KeyboardNavigationMode : int;
    enum KeyTipPlacementMode : int;
    enum LastInputDeviceType : int;
    enum ManipulationModes : unsigned int;
    enum StandardUICommandKind : int;
    enum XYFocusKeyboardNavigationMode : int;
    enum XYFocusNavigationStrategy : int;
    enum XYFocusNavigationStrategyOverride : int;
    interface IAccessKeyDisplayDismissedEventArgs;
    interface IAccessKeyDisplayRequestedEventArgs;
    interface IAccessKeyInvokedEventArgs;
    interface IAccessKeyManager;
    interface IAccessKeyManagerStatics;
    interface IAccessKeyManagerStatics2;
    interface ICanExecuteRequestedEventArgs;
    interface ICharacterReceivedRoutedEventArgs;
    interface ICommand;
    interface IContextRequestedEventArgs;
    interface IDoubleTappedEventHandler;
    interface IDoubleTappedRoutedEventArgs;
    interface IExecuteRequestedEventArgs;
    interface IFindNextElementOptions;
    interface IFindNextElementOptionsPrivate;
    interface IFocusedElementRemovedEventArgs;
    interface IFocusedElementRemovedEventHandler;
    interface IFocusManager;
    interface IFocusManagerGotFocusEventArgs;
    interface IFocusManagerLostFocusEventArgs;
    interface IFocusManagerStatics;
    interface IFocusManagerStatics2;
    interface IFocusManagerStatics3;
    interface IFocusManagerStatics4;
    interface IFocusManagerStatics5;
    interface IFocusManagerStatics6;
    interface IFocusManagerStatics7;
    interface IFocusManagerStaticsPrivate;
    interface IFocusMovementResult;
    interface IGettingFocusEventArgs;
    interface IGettingFocusEventArgs2;
    interface IGettingFocusEventArgs3;
    interface IHoldingEventHandler;
    interface IHoldingRoutedEventArgs;
    interface IInertiaExpansionBehavior;
    interface IInertiaRotationBehavior;
    interface IInertiaTranslationBehavior;
    interface IInputManagerStatics;
    interface IInputScope;
    interface IInputScopeName;
    interface IInputScopeNameFactory;
    interface IKeyboardAccelerator;
    interface IKeyboardAcceleratorFactory;
    interface IKeyboardAcceleratorInvokedEventArgs;
    interface IKeyboardAcceleratorInvokedEventArgs2;
    interface IKeyboardAcceleratorStatics;
    interface IKeyEventHandler;
    interface IKeyRoutedEventArgs;
    interface IKeyRoutedEventArgs2;
    interface IKeyRoutedEventArgs3;
    interface ILosingFocusEventArgs;
    interface ILosingFocusEventArgs2;
    interface ILosingFocusEventArgs3;
    interface IManipulationCompletedEventHandler;
    interface IManipulationCompletedRoutedEventArgs;
    interface IManipulationDeltaEventHandler;
    interface IManipulationDeltaRoutedEventArgs;
    interface IManipulationInertiaStartingEventHandler;
    interface IManipulationInertiaStartingRoutedEventArgs;
    interface IManipulationPivot;
    interface IManipulationPivotFactory;
    interface IManipulationStartedEventHandler;
    interface IManipulationStartedRoutedEventArgs;
    interface IManipulationStartedRoutedEventArgsFactory;
    interface IManipulationStartingEventHandler;
    interface IManipulationStartingRoutedEventArgs;
    interface INoFocusCandidateFoundEventArgs;
    interface IPointer;
    interface IPointerEventHandler;
    interface IPointerRoutedEventArgs;
    interface IPointerRoutedEventArgs2;
    interface IProcessKeyboardAcceleratorEventArgs;
    interface IRightTappedEventHandler;
    interface IRightTappedRoutedEventArgs;
    interface IStandardUICommand;
    interface IStandardUICommand2;
    interface IStandardUICommandFactory;
    interface IStandardUICommandStatics;
    interface ITappedEventHandler;
    interface ITappedRoutedEventArgs;
    interface IXamlUICommand;
    interface IXamlUICommandFactory;
    interface IXamlUICommandStatics;
} // Input
} // Xaml
} // UI
} // Microsoft
XAML_ABI_NAMESPACE_END
