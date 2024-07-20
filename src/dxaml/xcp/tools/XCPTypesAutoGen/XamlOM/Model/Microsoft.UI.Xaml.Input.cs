// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Input
{
    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public interface IFocusManagerStaticsPrivate
    {
        event Microsoft.UI.Xaml.Input.FocusedElementRemovedEventHandler FocusedElementRemoved;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object FindNextFocusWithSearchRootIgnoreEngagement(FocusNavigationDirection focusNavigationDirection, Windows.Foundation.Object searchRoot);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object FindNextFocusWithSearchRootIgnoreEngagementWithHintRect(FocusNavigationDirection focusNavigationDirection, Windows.Foundation.Object searchRoot, Windows.Foundation.Rect hintRect, Windows.Foundation.Rect exclusionRect);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Object FindNextFocusWithSearchRootIgnoreEngagementWithClip(FocusNavigationDirection focusNavigationDirection, Windows.Foundation.Object searchRoot, Windows.Foundation.Boolean ignoreClipping, Windows.Foundation.Boolean ignoreCone);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetEngagedControl(Windows.Foundation.Object engagedControl);

        [DXamlName("SetFocusedElementWithDirection")]
        [DXamlOverloadName("SetFocusedElement")]
        Windows.Foundation.Boolean SetFocusedElement(Microsoft.UI.Xaml.DependencyObject element, Microsoft.UI.Xaml.FocusState focusState, Windows.Foundation.Boolean animateIfBringIntoView, Windows.Foundation.Boolean forceBringIntoView, FocusNavigationDirection focusNavigationDirection, Windows.Foundation.Boolean requestInputActivation);
    }

    [DXamlIdlGroup("coretypes2")]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "c85f5f82-b82a-434b-b901-6fea33bef485")]
    [Implements(typeof(Microsoft.UI.Xaml.Input.IFocusManagerStaticsPrivate), IsStaticInterface = true)]
    public static class FocusManager
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.IAsyncOperation<FocusMovementResult> TryFocusAsync(Microsoft.UI.Xaml.DependencyObject element, Microsoft.UI.Xaml.FocusState value)
        {
            return default(Windows.Foundation.IAsyncOperation<Microsoft.UI.Xaml.Input.FocusMovementResult>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryMoveFocusAsync")]
        [DXamlOverloadName("TryMoveFocusAsync")]
        public static Windows.Foundation.IAsyncOperation<FocusMovementResult> TryMoveFocusAsync(FocusNavigationDirection focusNavigationDirection)
        {
            return default(Windows.Foundation.IAsyncOperation<FocusMovementResult>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryMoveFocusWithOptionsAsync")]
        [DXamlOverloadName("TryMoveFocusAsync")]
        public static Windows.Foundation.IAsyncOperation<FocusMovementResult> TryMoveFocusAsync(FocusNavigationDirection focusNavigationDirection, FindNextElementOptions focusNavigationOptions)
        {
            return default(Windows.Foundation.IAsyncOperation<FocusMovementResult>);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryMoveFocusWithOptions")]
        [DXamlOverloadName("TryMoveFocus")]
        public static Windows.Foundation.Boolean TryMoveFocus(FocusNavigationDirection focusNavigationDirection, FindNextElementOptions focusNavigationOptions)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("FindNextElement")]
        [DXamlOverloadName("FindNextElement")]
        public static Microsoft.UI.Xaml.DependencyObject FindNextElement(FocusNavigationDirection focusNavigationDirection)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Microsoft.UI.Xaml.DependencyObject FindFirstFocusableElement([Optional] Microsoft.UI.Xaml.DependencyObject searchScope)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Microsoft.UI.Xaml.DependencyObject FindLastFocusableElement([Optional] Microsoft.UI.Xaml.DependencyObject searchScope)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("FindNextElementWithOptions")]
        [DXamlOverloadName("FindNextElement")]
        public static Microsoft.UI.Xaml.DependencyObject FindNextElement(FocusNavigationDirection focusNavigationDirection, FindNextElementOptions focusNavigationOptions)
        {
            return default(Microsoft.UI.Xaml.DependencyObject);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("FindNextFocusableElement")]
        [DXamlOverloadName("FindNextFocusableElement")]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Xaml.UIElement FindNextFocusableElement(FocusNavigationDirection focusNavigationDirection)
        {
            return default(Microsoft.UI.Xaml.UIElement);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("FindNextFocusableElementWithHint")]
        [DXamlOverloadName("FindNextFocusableElement")]
        [ReturnTypeParameterName("result")]
        public static Microsoft.UI.Xaml.UIElement FindNextFocusableElement(FocusNavigationDirection focusNavigationDirection, Windows.Foundation.Rect hintRect)
        {
            return default(Microsoft.UI.Xaml.UIElement);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("TryMoveFocus")]
        [DXamlOverloadName("TryMoveFocus")]
        public static Windows.Foundation.Boolean TryMoveFocus(FocusNavigationDirection focusNavigationDirection)
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Object GetFocusedElement()
        {
            return default(Windows.Foundation.Object);
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.Input.GotFocusEventHandler GotFocus;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.Input.LostFocusEventHandler LostFocus;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.Input.GettingFocusEventHandler GettingFocus;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public static event Microsoft.UI.Xaml.Input.LosingFocusEventHandler LosingFocus;

        [DXamlName("GetFocusedElementWithRoot")]
        [DXamlOverloadName("GetFocusedElement")]
        public static Windows.Foundation.Object GetFocusedElementWithRoot(Microsoft.UI.Xaml.XamlRoot xamlRoot)
        {
            return default(Windows.Foundation.Object);
        }
    }

    [StubDelegate]
    public delegate void GotFocusEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.FocusManagerGotFocusEventArgs args);

    [StubDelegate]
    public delegate void LostFocusEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.FocusManagerLostFocusEventArgs args);

    [StubDelegate]
    public delegate void GettingFocusEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args);

    [StubDelegate]
    public delegate void LosingFocusEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.LosingFocusEventArgs args);

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IFindNextElementOptionsPrivate
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IgnoreOcclusivity
        {
            get;
            set;
        }
    }

    [Guids(ClassGuid = "4a47dcbe-fbaf-4cf0-8702-6ac1f462a218")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    public sealed class FocusMovementResult
        : Windows.Foundation.Object
    {
        public Windows.Foundation.Boolean Succeeded
        {
            get;
            private set;
        }

        internal FocusMovementResult() { }
    }

    [Guids(ClassGuid = "c5856b8c-e7dc-473f-8e34-d0c1dae9e482")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [DXamlIdlGroup("coretypes2")]
    [Implements(typeof(Microsoft.UI.Xaml.Input.IFindNextElementOptionsPrivate))]
    public sealed class FindNextElementOptions
        : Windows.Foundation.Object
    {
        public Microsoft.UI.Xaml.DependencyObject SearchRoot
        {
            get;
            set;
        }

        public Windows.Foundation.Rect ExclusionRect
        {
            get;
            set;
        }

        public Windows.Foundation.Rect HintRect
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategyOverride XYFocusNavigationStrategyOverride
        {
            get;
            set;
        }

        public FindNextElementOptions() { }
    }

    [Guids(ClassGuid = "1d311808-1263-467e-b107-5c90272f972d")]
    public interface ICommand
    {
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        event Microsoft.UI.Xaml.EventHandler CanExecuteChanged;

        Windows.Foundation.Boolean CanExecute([Optional] Windows.Foundation.Object parameter);
        void Execute([Optional] Windows.Foundation.Object parameter);
    }

    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [NativeName("CFocusedElementRemovedEventArgs")]
    [Guids(ClassGuid = "b70caa88-e72a-4cba-ad3d-aa79d0cada3e")]
    [TypeFlags(IsCreateableFromXAML = false)]
    public sealed class FocusedElementRemovedEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_oldFocusedElement")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.DependencyObject OldFocusedElement
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [DelegateToCore]
        [OffsetFieldName("m_newFocusedElement")]
        public Microsoft.UI.Xaml.DependencyObject NewFocusedElement
        {
            get;
            set;
        }

        internal FocusedElementRemovedEventArgs() { }
    }

    [NativeName("CGettingFocusEventArgs")]
    [Guids(ClassGuid = "36548ba1-034c-4ba6-b387-619bdf9972ff")]
    public sealed class GettingFocusEventArgs : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_oldFocusedElement")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.DependencyObject OldFocusedElement
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [DelegateToCore]
        [OffsetFieldName("m_newFocusedElement")]
        public Microsoft.UI.Xaml.DependencyObject NewFocusedElement
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_focusState")]
        public Microsoft.UI.Xaml.FocusState FocusState
        {
            get;
            private set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_focusNavigationDirection")]
        public Microsoft.UI.Xaml.Input.FocusNavigationDirection Direction
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_inputDevice")]
        public Microsoft.UI.Xaml.Input.FocusInputDeviceKind InputDevice
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bCancel")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [DelegateToCore]
        [ReadOnly]
        public Windows.Foundation.Guid CorrelationId
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean TryCancel()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean TrySetNewFocusedElement(Microsoft.UI.Xaml.DependencyObject element)
        {
            return default(Windows.Foundation.Boolean);
        }

        internal GettingFocusEventArgs() { }
    }

    [NativeName("CLosingFocusEventArgs")]
    [Guids(ClassGuid = "3fb5a4d9-deb1-446e-a6eb-9599d3bda271")]
    public sealed class LosingFocusEventArgs : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_oldFocusedElement")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.DependencyObject OldFocusedElement
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [DelegateToCore]
        [OffsetFieldName("m_newFocusedElement")]
        public Microsoft.UI.Xaml.DependencyObject NewFocusedElement
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_focusState")]
        public Microsoft.UI.Xaml.FocusState FocusState
        {
            get;
            private set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_focusNavigationDirection")]
        public Microsoft.UI.Xaml.Input.FocusNavigationDirection Direction
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_inputDevice")]
        public Microsoft.UI.Xaml.Input.FocusInputDeviceKind InputDevice
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bCancel")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Cancel
        {
            get;
            set;
        }

        [DelegateToCore]
        [ReadOnly]
        public Windows.Foundation.Guid CorrelationId
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean TryCancel()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean TrySetNewFocusedElement(Microsoft.UI.Xaml.DependencyObject element)
        {
            return default(Windows.Foundation.Boolean);
        }

        internal LosingFocusEventArgs() { }
    }

    [NativeName("CFocusManagerGotFocusEventArgs")]
    [Guids(ClassGuid = "d9f3f186-9981-4569-9d49-2d7522aa4bdb")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class FocusManagerGotFocusEventArgs : Microsoft.UI.Xaml.EventArgs
    {

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_newFocusedElement")]
        [DelegateToCore]
        [ReadOnly]
        public Microsoft.UI.Xaml.DependencyObject NewFocusedElement
        {
            get;
        }

        [DelegateToCore]
        [ReadOnly]
        public Windows.Foundation.Guid CorrelationId
        {
            get;
        }

        internal FocusManagerGotFocusEventArgs() { }
    }

    [NativeName("CFocusManagerLostFocusEventArgs")]
    [Guids(ClassGuid = "a28a191b-0b16-4ff8-9a6a-cd14c43d8f34")]
    [DXamlIdlGroup("coretypes2")]
    public sealed class FocusManagerLostFocusEventArgs : Microsoft.UI.Xaml.EventArgs
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_oldFocusedElement")]
        [DelegateToCore]
        [ReadOnly]
        public Microsoft.UI.Xaml.DependencyObject OldFocusedElement
        {
            get;
        }

        [DelegateToCore]
        [ReadOnly]
        public Windows.Foundation.Guid CorrelationId
        {
            get;
        }

        internal FocusManagerLostFocusEventArgs() { }
    }

    [NativeName("CNoFocusCandidateFoundEventArgs")]
    [Guids(ClassGuid = "590cf413-aa3d-45ab-a558-cbdf4a034e6b")]
    public sealed class NoFocusCandidateFoundEventArgs : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_focusNavigationDirection")]
        public Microsoft.UI.Xaml.Input.FocusNavigationDirection Direction
        {
            get;
            private set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        [OffsetFieldName("m_inputDevice")]
        public Microsoft.UI.Xaml.Input.FocusInputDeviceKind InputDevice
        {
            get;
            private set;
        }

        internal NoFocusCandidateFoundEventArgs() { }
    }

    [DXamlIdlGroup("coretypes2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [Guids(ClassGuid = "1fa6a32d-2da8-467a-9c6a-fc4fd5bebbf5")]
    public static class InputManager
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Microsoft.UI.Xaml.Input.LastInputDeviceType GetLastInputDeviceType()
        {
            return default(Microsoft.UI.Xaml.Input.LastInputDeviceType);
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CKeyEventArgs")]
    [Guids(ClassGuid = "64a97056-3aaf-4a46-98da-d8b8b63c93ed")]
    public sealed class KeyRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [ReadOnly]
        [DelegateToCore]
        public Windows.System.VirtualKey Key
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Windows.UI.Core.CorePhysicalKeyStatus KeyStatus
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_shift")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Shift
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.CoreOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_ctrl")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Ctrl
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [ReadOnly]
        [DelegateToCore]
        public Windows.System.VirtualKey OriginalKey
        {
            get;
            private set;
        }

        [ReadOnly]
        [DelegateToCore]
        public string DeviceId
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_handledShouldNotImpedeTextInput")]
        [DelegateToCore]
        internal Windows.Foundation.Boolean HandledShouldNotImpedeTextInput
        {
            get;
            set;
        }

        internal KeyRoutedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CPointerEventArgs")]
    [ClassFlags(RequiresCoreServices = true)]
    [Guids(ClassGuid = "39b24594-420e-4a63-af94-f92dbecec701")]
    public sealed class PointerRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPointer")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.Input.Pointer Pointer
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_keyModifiers")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.System.VirtualKeyModifiers KeyModifiers
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_uiGestureFollowing")]
        [ReadOnly]
        [DelegateToCore]
        internal Microsoft.UI.Xaml.Input.GestureModes GestureFollowing
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        internal PointerRoutedEventArgs() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [NativeClassName("CPointerEventArgs")]
        public Microsoft.UI.Input.PointerPoint GetCurrentPoint([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Microsoft.UI.Input.PointerPoint);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [NativeClassName("CPointerEventArgs")]
        public Windows.Foundation.Collections.IVector<Microsoft.UI.Input.PointerPoint> GetIntermediatePoints([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Collections.IVector<Microsoft.UI.Input.PointerPoint>);
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isGenerated")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Boolean IsGenerated
        {
            get;
            private set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CPointerCollection")]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "60e014d5-7025-4f7b-b57b-92095ee9142b")]
    public sealed class PointerCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<Pointer>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Input.Pointer ContentProperty
        {
            get;
            set;
        }

        internal PointerCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CTappedEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "f16b4980-51ee-44de-95d0-abaa9727b779")]
    public sealed class TappedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public TappedRoutedEventArgs() { }

        [PInvoke]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CDoubleTappedEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "31ab39bf-2cb7-4f89-ad58-38708f678372")]
    public sealed class DoubleTappedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public DoubleTappedRoutedEventArgs() { }

        [PInvoke]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CHoldingEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "3956fb95-4fbe-4a13-af5d-6702b46edf7b")]
    public sealed class HoldingRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_holdingState")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.HoldingState HoldingState
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public HoldingRoutedEventArgs() { }

        [PInvoke]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CContextRequestedEventArgs")]
    [ClassFlags(IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "10e7cd16-a374-47f3-b117-4f88bb6569ec")]
    public sealed class ContextRequestedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public ContextRequestedEventArgs() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean TryGetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo, out Windows.Foundation.Point point)
        {
            point = default(Windows.Foundation.Point);
            return default(Windows.Foundation.Boolean);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CRightTappedEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "867d84ea-98f5-4132-bd14-a7bb5972e156")]
    public sealed class RightTappedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public RightTappedRoutedEventArgs() { }

        [PInvoke]
        public Windows.Foundation.Point GetPosition([Optional] Microsoft.UI.Xaml.UIElement relativeTo)
        {
            return default(Windows.Foundation.Point);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationStartingEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "7df2dbb8-9fa6-4b92-9ca5-2f1e5529d3c5")]
    public sealed class ManipulationStartingRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_uiManipulationMode")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.Input.ManipulationModes Mode
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CManipulationEventArgs", "ManipulationContainer")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement Container
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pPivot")]
        [DelegateToCore]
        public Microsoft.UI.Xaml.Input.ManipulationPivot Pivot
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public ManipulationStartingRoutedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationInertiaStartingEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "0a201323-c3ee-46d3-970d-ec8061061ad1")]
    public sealed class ManipulationInertiaStartingRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CManipulationEventArgs", "ManipulationContainer")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement Container
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pExpansion")]
        public Microsoft.UI.Xaml.Input.InertiaExpansionBehavior ExpansionBehavior
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pRotation")]
        public Microsoft.UI.Xaml.Input.InertiaRotationBehavior RotationBehavior
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pTranslation")]
        public Microsoft.UI.Xaml.Input.InertiaTranslationBehavior TranslationBehavior
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDelta")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Delta
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCumulative")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Cumulative
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pVelocities")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationVelocities Velocities
        {
            get;
            private set;
        }

        public ManipulationInertiaStartingRoutedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationStartedEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "15b33f27-58f4-4e3b-8bf5-77a3f1af4631")]
    public class ManipulationStartedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CManipulationEventArgs", "ManipulationContainer")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement Container
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptManipulation")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Point Position
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCumulative")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Cumulative
        {
            get;
            private set;
        }

        public ManipulationStartedRoutedEventArgs() { }

        [PInvoke]
        public void Complete()
        {
        }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationDeltaEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "90cfc789-6ff1-49f8-9dbd-dc0a52af783e")]
    public sealed class ManipulationDeltaRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CManipulationEventArgs", "ManipulationContainer")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement Container
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptManipulation")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Point Position
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bInertial")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Boolean IsInertial
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pDelta")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Delta
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pCumulative")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Cumulative
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pVelocities")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationVelocities Velocities
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        public ManipulationDeltaRoutedEventArgs() { }

        [PInvoke]
        public void Complete()
        {
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationCompletedEventArgs")]
    [ClassFlags(IsCreateableAfterV2 = false, IsVisibleInXAML = false, RequiresCoreServices = true)]
    [Guids(ClassGuid = "05358d37-09ed-4cef-8f08-ba4d6d10dcc9")]
    public sealed class ManipulationCompletedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeMethod("CManipulationEventArgs", "ManipulationContainer")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Xaml.UIElement Container
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptManipulation")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Point Position
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bInertial")]
        [ReadOnly]
        [DelegateToCore]
        public Windows.Foundation.Boolean IsInertial
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pManipulationTotal")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationDelta Cumulative
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pVelocitiesFinal")]
        [ReadOnly]
        public Microsoft.UI.Input.ManipulationVelocities Velocities
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        [DelegateToCore]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        public ManipulationCompletedRoutedEventArgs() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationDelta")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "5ddadc0d-abfc-4738-b4aa-c7fbaad1cb6b")]
    internal sealed class ManipulationDelta
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal ManipulationDelta() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationVelocities")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "da49c6f7-6615-464f-8c47-539e5f26bac1")]
    internal sealed class ManipulationVelocities
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal ManipulationVelocities() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CManipulationPivot")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "b38574e9-ef01-4695-ad24-028bf9da6fe9")]
    public sealed class ManipulationPivot
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valuePoint)]
        [OffsetFieldName("m_ptCenter")]
        public Windows.Foundation.Point Center
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fRadius")]
        public Windows.Foundation.Double Radius
        {
            get;
            set;
        }

        public ManipulationPivot() { }

        [FactoryMethodName("CreateInstanceWithCenterAndRadius")]
        public ManipulationPivot(Windows.Foundation.Point center, Windows.Foundation.Double radius) { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInertiaExpansionBehavior")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "db2325f4-7e34-46b9-9bf7-59ae4ee4a9e1")]
    public sealed class InertiaExpansionBehavior
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fDeceleration")]
        public Windows.Foundation.Double DesiredDeceleration
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fExpansion")]
        public Windows.Foundation.Double DesiredExpansion
        {
            get;
            set;
        }

        internal InertiaExpansionBehavior() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInertiaRotationBehavior")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "9cdeb78d-ca15-4fea-8c89-dab027bac148")]
    public sealed class InertiaRotationBehavior
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fDeceleration")]
        public Windows.Foundation.Double DesiredDeceleration
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fRotation")]
        public Windows.Foundation.Double DesiredRotation
        {
            get;
            set;
        }

        internal InertiaRotationBehavior() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInertiaTranslationBehavior")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "dda979fb-f67f-459b-bb2d-f1f4edfd12eb")]
    public sealed class InertiaTranslationBehavior
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fDeceleration")]
        public Windows.Foundation.Double DesiredDeceleration
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_fDisplacement")]
        public Windows.Foundation.Double DesiredDisplacement
        {
            get;
            set;
        }

        internal InertiaTranslationBehavior() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CPointer")]
    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsCreateableAfterV2 = false, IsVisibleInXAML = false)]
    [Guids(ClassGuid = "3f966f30-289a-45de-960c-049b5674cfde")]
    public sealed class Pointer
     : Microsoft.UI.Xaml.DependencyObject
    {
        [CoreType(typeof(Windows.Foundation.Int32))]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueSigned)]
        [OffsetFieldName("m_uiPointerId")]
        [ReadOnly]
        public Windows.Foundation.UInt32 PointerId
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_pointerDeviceType")]
        [ReadOnly]
        public Microsoft.UI.Input.PointerDeviceType PointerDeviceType
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bInContact")]
        [ReadOnly]
        public Windows.Foundation.Boolean IsInContact
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bInRange")]
        [ReadOnly]
        public Windows.Foundation.Boolean IsInRange
        {
            get;
            private set;
        }

        internal Pointer() { }
    }

    [NativeName("CCharacterReceivedRoutedEventArgs")]
    [Guids(ClassGuid = "bc819a19-069c-4842-ae53-abe22d2daaf2")]
    public sealed class CharacterReceivedRoutedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueUnsigned)]
        [OffsetFieldName("m_platformKeyCode")]
        [CoreType(typeof(Windows.Foundation.Char16))]
        [ReadOnly]
        public Windows.Foundation.Char16 Character
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_physicalKeyStatus")]
        [ReadOnly]
        public Windows.UI.Core.CorePhysicalKeyStatus KeyStatus
        {
            get;
            private set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_bHandled")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        internal CharacterReceivedRoutedEventArgs() { }
    }

    [NativeName("CInputScope")]
    [ClassFlags(HasTypeConverter = true)]
    [Guids(ClassGuid = "1cabfa8e-eee3-45d0-aabd-6e73404576bb")]
    public sealed class InputScope
     : Microsoft.UI.Xaml.DependencyObject
    {
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pNames")]
        [CollectionType(CollectionKind.Vector)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Input.InputScopeNameCollection Names
        {
            get;
            private set;
        }

        public InputScope() { }
    }

    [NativeName("CInputScopeName")]
    [ContentProperty("NameValue")]
    [Guids(ClassGuid = "c805ef39-d016-4e1e-a77d-470c74e597b2")]
    public sealed class InputScopeName
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_nameValue")]
        public Microsoft.UI.Xaml.Input.InputScopeNameValue NameValue
        {
            get;
            set;
        }

        public InputScopeName() { }

        public InputScopeName(Microsoft.UI.Xaml.Input.InputScopeNameValue nameValue) { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CInputScopeNameCollection")]
    [Modifier(Modifier.Internal)]
    [Guids(ClassGuid = "ec5da8a5-5da2-450b-bcd4-2713b7496ead")]
    public sealed class InputScopeNameCollection
     : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<InputScopeName>
    {
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Input.InputScopeName ContentProperty
        {
            get;
            set;
        }

        internal InputScopeNameCollection() { }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "bc061d30-f06e-4294-94e3-dbe9373e2973")]
    public sealed class ProcessKeyboardAcceleratorEventArgs
    {
        internal ProcessKeyboardAcceleratorEventArgs() { }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_key")]
        public Windows.System.VirtualKey Key
        {
            get;
            internal set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_keyModifiers")]
        public Windows.System.VirtualKeyModifiers Modifiers
        {
            get;
            internal set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_handled")]
        public bool Handled
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_handledShouldNotImpedeTextInput")]
        internal bool HandledShouldNotImpedeTextInput
        {
            get;
            set;
        }

    }

    [ClassFlags(HasBaseTypeInDXamlInterface = false, IsHiddenFromIdl = true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [NativeName("CKeyboardAcceleratorCollection")]
    [Guids(ClassGuid = "d7c34d6a-076e-466f-84f6-7d8af5585b9f")]
    public sealed class KeyboardAcceleratorCollection
        : Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<KeyboardAccelerator>
    {
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.Input.KeyboardAccelerator ContentProperty
        {
            get;
            set;
        }

        internal KeyboardAcceleratorCollection() { }
    }

    public enum KeyboardAcceleratorPlacementMode
    {
        Auto = 0,
        Hidden = 1
    }

    [CodeGen(partial: true)]
    [NativeName("CKeyboardAccelerator")]
    [Guids(ClassGuid = "41b6ad62-c40c-4d7c-b5e5-abe7316539c6")]
    public class KeyboardAccelerator : Microsoft.UI.Xaml.DependencyObject
    {
        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_key")]
        public Windows.System.VirtualKey Key
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueEnum)]
        [OffsetFieldName("m_keyModifiers")]
        public Windows.System.VirtualKeyModifiers Modifiers
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueBool)]
        [OffsetFieldName("m_isEnabled")]
        public Windows.Foundation.Boolean IsEnabled
        {
            get;
            set;
        }

        [NativeStorageType(OM.ValueType.valueObject)]
        [OffsetFieldName("m_scopeOwner")]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject ScopeOwner
        {
            get;
            set;
        }

        [EventFlags(UseEventManager = true)]
        public event Windows.Foundation.TypedEventHandler<KeyboardAccelerator, KeyboardAcceleratorInvokedEventArgs> Invoked;

        public KeyboardAccelerator() { }

    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "8e85b6bf-2483-44d5-b97f-7739d956c2c7")]
    public sealed class KeyboardAcceleratorInvokedEventArgs
    : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.Boolean Handled
        {
            get;
            set;
        }

        public DependencyObject Element
        {
            get;
            internal set;
        }

        public KeyboardAccelerator KeyboardAccelerator
        {
            get;
            internal set;
        }

        internal KeyboardAcceleratorInvokedEventArgs() { }
    }

    [CodeGen(CodeGenLevel.CoreOnly)]
    [NativeName("TabletDeviceType")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true, IsNativeTypeDef = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum TabletDeviceType
    {
        [NativeValueName("XcpDeviceTypeMouse")]
        Mouse = 0,
        [NativeValueName("XcpDeviceTypeStylus")]
        Stylus = 1,
        [NativeValueName("XcpDeviceTypeTouch")]
        Touch = 2,
    }

    [NativeName("KeyboardNavigationMode")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [NativeComment("Determines the tab navigation.")]
    public enum KeyboardNavigationMode
    {
        [NativeValueName("Local")]
        Local = 0,
        [NativeValueName("Cycle")]
        Cycle = 1,
        [NativeValueName("Once")]
        Once = 2,
    }

    [NativeName("XcpManipulationModes")]
    [EnumFlags(AreValuesFlags = true, HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    public enum ManipulationModes
    {
        [NativeValueName("XcpManipulationModesNone")]
        None = 0,
        [NativeValueName("XcpManipulationModesTranslateX")]
        TranslateX = 1,
        [NativeValueName("XcpManipulationModesTranslateY")]
        TranslateY = 2,
        [NativeValueName("XcpManipulationModesTranslateRailsX")]
        TranslateRailsX = 4,
        [NativeValueName("XcpManipulationModesTranslateRailsY")]
        TranslateRailsY = 8,
        [NativeValueName("XcpManipulationModesRotate")]
        Rotate = 16,
        [NativeValueName("XcpManipulationModesScale")]
        Scale = 32,
        [NativeValueName("XcpManipulationModesTranslateInertia")]
        TranslateInertia = 64,
        [NativeValueName("XcpManipulationModesRotateInertia")]
        RotateInertia = 128,
        [NativeValueName("XcpManipulationModesScaleInertia")]
        ScaleInertia = 256,
        [NativeValueName("XcpManipulationModesAll")]
        All = 65535,
        [NativeValueName("XcpManipulationModesSystem")]
        System = 65536,
    }

    [NativeName("XcpGestureModes")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    internal enum GestureModes
    {
        [NativeValueName("XcpGestureModesNone")]
        None = 0,
        [NativeValueName("XcpGestureModesTapped")]
        Tapped = 1,
        [NativeValueName("XcpGestureModesDoubleTapped")]
        DoubleTapped = 2,
        [NativeValueName("XcpGestureModesRightTapped")]
        RightTapped = 3,
        [NativeValueName("XcpGestureModesHolding")]
        Holding = 4,
    }

    [NativeName("DragDropMessageType")]
    [TypeTable(IsExcludedFromCore = true, IsExcludedFromDXaml = true, IsExcludedFromNewTypeTable = true)]
    [NativeComment("Used by the DragDrop_RaiseEvent pinvoke to indicate the nature of the drag action to be processed. DragEnter: Start a drag. DragLeave: Cancel a drag. DragOver: Signal drag location changed. Drop: Drop the data.")]
    internal enum DragDropMessageType
    {
        [NativeValueName("DDMT_DragEnter")]
        DragEnter = 0,
        [NativeValueName("DDMT_DragLeave")]
        DragLeave = 1,
        [NativeValueName("DDMT_DragOver")]
        DragOver = 2,
        [NativeValueName("DDMT_Drop")]
        Drop = 3,
    }

    [NativeName("CInputScopeNameValue")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    public enum InputScopeNameValue
    {
        [NativeValueName("InputScopeNameValueDefault")]
        Default = 0,
        [NativeValueName("InputScopeNameValueUrl")]
        Url = 1,
        [NativeValueName("InputScopeNameValueEmailSmtpAddress")]
        EmailSmtpAddress = 5,
        [NativeValueName("InputScopeNameValuePersonalFullName")]
        PersonalFullName = 7,
        [NativeValueName("InputScopeNameValueCurrencyAmountAndSymbol")]
        CurrencyAmountAndSymbol = 20,
        [NativeValueName("InputScopeNameValueCurrencyAmount")]
        CurrencyAmount = 21,
        [NativeValueName("InputScopeNameValueDateMonthNumber")]
        DateMonthNumber = 23,
        [NativeValueName("InputScopeNameValueDateDayNumber")]
        DateDayNumber = 24,
        [NativeValueName("InputScopeNameValueDateYear")]
        DateYear = 25,
        [NativeValueName("InputScopeNameValueDigits")]
        Digits = 28,
        [NativeValueName("InputScopeNameValueNumber")]
        Number = 29,
        [NativeValueName("InputScopeNameValuePassword")]
        Password = 31,
        [NativeValueName("InputScopeNameValueTelephoneNumber")]
        TelephoneNumber = 32,
        [NativeValueName("InputScopeNameValueTelephoneCountryCode")]
        TelephoneCountryCode = 33,
        [NativeValueName("InputScopeNameValueTelephoneAreaCode")]
        TelephoneAreaCode = 34,
        [NativeValueName("InputScopeNameValueTelephoneLocalNumber")]
        TelephoneLocalNumber = 35,
        [NativeValueName("InputScopeNameValueTimeHour")]
        TimeHour = 37,
        [NativeValueName("InputScopeNameValueTimeMinutesOrSeconds")]
        TimeMinutesOrSeconds = 38,
        [NativeValueName("InputScopeNameValueNumberFullWidth")]
        NumberFullWidth = 39,
        [NativeValueName("InputScopeNameValueAlphanumericHalfWidth")]
        AlphanumericHalfWidth = 40,
        [NativeValueName("InputScopeNameValueAlphanumericFullWidth")]
        AlphanumericFullWidth = 41,
        [NativeValueName("InputScopeNameValueHiragana")]
        Hiragana = 44,
        [NativeValueName("InputScopeNameValueKatakanaHalfWidth")]
        KatakanaHalfWidth = 45,
        [NativeValueName("InputScopeNameValueKatakanaFullWidth")]
        KatakanaFullWidth = 46,
        [NativeValueName("InputScopeNameValueHanja")]
        Hanja = 47,
        [NativeValueName("InputScopeNameValueHangulHalfWidth")]
        HangulHalfWidth = 48,
        [NativeValueName("InputScopeNameValueHangulFullWidth")]
        HangulFullWidth = 49,
        [NativeValueName("InputScopeNameValueSearch")]
        Search = 50,
        [NativeValueName("InputScopeNameValueFormula")]
        Formula = 51,
        [NativeValueName("InputScopeNameValueSearchIncremental")]
        SearchIncremental = 52,
        [NativeValueName("InputScopeNameValueChineseHalfWidth")]
        ChineseHalfWidth = 53,
        [NativeValueName("InputScopeNameValueChineseFullWidth")]
        ChineseFullWidth = 54,
        [NativeValueName("InputScopeNameValueNativeScript")]
        NativeScript = 55,
        [NativeValueName("InputScopeNameValueText")]
        Text = 57,
        [NativeValueName("InputScopeNameValueChat")]
        Chat = 58,
        [NativeValueName("InputScopeNameValueNameOrPhoneNumber")]
        NameOrPhoneNumber = 59,
        [NativeValueName("InputScopeNameValueEmailNameOrAddress")]
        EmailNameOrAddress = 60,
        [NativeValueName("InputScopeNameValueMaps")]
        Maps = 62,
        [NativeValueName("InputScopeNameValueNumericPassword")]
        NumericPassword = 63,
        [NativeValueName("InputScopeNameValueNumericPin")]
        NumericPin = 64,
        [NativeValueName("InputScopeNameValueAlphanumericPin")]
        AlphanumericPin = 65,
        [NativeValueName("InputScopeNameValueFormulaNumber")]
        FormulaNumber = 67,
        [NativeValueName("InputScopeNameValueChatWithoutEmoji")]
        ChatWithoutEmoji = 68,
    }

    [NativeName("FocusNavigationDirection")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [NativeComment("Determines the focus navigation direction.")]
    public enum FocusNavigationDirection
    {
        [NativeValueName("Next")]
        Next = 0,
        [NativeValueName("Previous")]
        Previous = 1,
        [NativeValueName("Up")]
        Up = 2,
        [NativeValueName("Down")]
        Down = 3,
        [NativeValueName("Left")]
        Left = 4,
        [NativeValueName("Right")]
        Right = 5,
        [NativeValueName("None")]
        None = 6,
    }

    public enum XYFocusKeyboardNavigationMode
    {
        Auto,
        Enabled,
        Disabled
    }

    public enum XYFocusNavigationStrategy
    {
        Auto,
        Projection,
        NavigationDirectionDistance,
        RectilinearDistance
    }

    public enum XYFocusNavigationStrategyOverride
    {
        None = 0,
        Auto = 1,
        Projection = 2,
        NavigationDirectionDistance = 3,
        RectilinearDistance = 4
    }

    [NativeName("InputDeviceType")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    internal enum InputDeviceType
    {
        [NativeValueName("InputDeviceTypeNone")]
        None = 0,
        [NativeValueName("InputDeviceTypeMouse")]
        Mouse = 1,
        [NativeValueName("InputDeviceTypeTouch")]
        Touch = 2,
        [NativeValueName("InputDeviceTypePen")]
        Pen = 3,
        [NativeValueName("InputDeviceTypeKeyboard")]
        Keyboard = 4,
        [NativeValueName("InputDeviceTypeGamepadOrRemote")]
        GamepadOrRemote = 5,
    }

    [Platform(typeof(PrivateApiContract), 1)]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    public enum LastInputDeviceType
    {
        None = 0,
        Mouse = 1,
        Touch = 2,
        Pen = 3,
        Keyboard = 4,
        GamepadOrRemote = 5,
    }

    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    public enum FocusInputDeviceKind
    {
        None = 0,
        Mouse = 1,
        Touch = 2,
        Pen = 3,
        Keyboard = 4,
        GameController = 5,
    }

    public delegate void KeyEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.KeyRoutedEventArgs e);

    public delegate void PointerEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.PointerRoutedEventArgs e);

    public delegate void TappedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.TappedRoutedEventArgs e);

    public delegate void DoubleTappedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.DoubleTappedRoutedEventArgs e);

    public delegate void HoldingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.HoldingRoutedEventArgs e);

    public delegate void RightTappedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs e);

    internal delegate void RightTappedUnhandledEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs e);

    public delegate void ManipulationStartingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.ManipulationStartingRoutedEventArgs e);

    public delegate void ManipulationInertiaStartingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.ManipulationInertiaStartingRoutedEventArgs e);

    public delegate void ManipulationStartedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.ManipulationStartedRoutedEventArgs e);

    public delegate void ManipulationDeltaEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.ManipulationDeltaRoutedEventArgs e);

    public delegate void ManipulationCompletedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.ManipulationCompletedRoutedEventArgs e);

    [Platform(typeof(PrivateApiContract), 1)]
    public delegate void FocusedElementRemovedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Input.FocusedElementRemovedEventArgs e);
}
