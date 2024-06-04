// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using System;
using Windows.Foundation;
using Microsoft.UI.Xaml.Input;
using XamlOM;

namespace Microsoft.UI.Xaml
{
    [Platform(typeof(PrivateApiContract), 1)]
    public interface IUIElementStaticsPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        bool InternalGetIsEnabled(
            Microsoft.UI.Xaml.UIElement element
            );

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void InternalPutIsEnabled(
            Microsoft.UI.Xaml.UIElement element,
            Windows.Foundation.Boolean value
            );
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IUIElementPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean FocusNoActivate(Microsoft.UI.Xaml.FocusState value);
    }

    [CodeGen(partial: true)]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [InterfaceDetails(ClassGuid = "61615723-8486-4376-84d5-27d0ff539580")]
    public sealed class DxamlCoreTestHooks
    {
        public static DxamlCoreTestHooks GetForCurrentThread()
        {
            return default(DxamlCoreTestHooks);
        }

        [AllowCrossThreadAccess]
        public static void PerformProcessWideLeakDetection(int threshold) { }
    }

    [Platform("Feature_XamlMotionSystemHoldbacks", typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Velocity(Feature = "Feature_XamlMotionSystemHoldbacks")]
    [Implements(typeof(Microsoft.UI.Xaml.IUIElementStaticsPrivate), IsStaticInterface = true)]
    [Implements(typeof(Microsoft.UI.Xaml.IUIElementPrivate))]
    partial class UIElement
    {
        public bool ExitDisplayModeOnAccessKeyInvoked
        {
            get;
            set;
        }

        public bool IsAccessKeyScope
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject AccessKeyScopeOwner
        {
            get;
            set;
        }

        public Windows.Foundation.String AccessKey
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        internal Windows.Foundation.Boolean IsGamepadFocusCandidate
        {
            get;
            set;
        }

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<UIElement, AccessKeyDisplayRequestedEventArgs> AccessKeyDisplayRequested;

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<UIElement, AccessKeyDisplayDismissedEventArgs> AccessKeyDisplayDismissed;

        [EventFlags(UseEventManager = true)]
        public event TypedEventHandler<UIElement, AccessKeyInvokedEventArgs> AccessKeyInvoked;

        [PropertyFlags(IsValueInherited = true)]
        public KeyTipPlacementMode KeyTipPlacementMode
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Double KeyTipHorizontalOffset
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Double KeyTipVerticalOffset
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public DependencyObject KeyTipTarget
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("StartBringIntoView")]
        [DXamlOverloadName("StartBringIntoView")]
        public void StartBringIntoView()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [DXamlName("StartBringIntoViewWithOptions")]
        [DXamlOverloadName("StartBringIntoView")]
        public void StartBringIntoView(BringIntoViewOptions options)
        {
        }

        public Microsoft.UI.Xaml.Input.XYFocusKeyboardNavigationMode XYFocusKeyboardNavigation
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusUpNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusDownNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusLeftNavigationStrategy
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.Input.XYFocusNavigationStrategy XYFocusRightNavigationStrategy
        {
            get;
            set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent GettingFocusEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent LosingFocusEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent NoFocusCandidateFoundEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PreviewKeyDownEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent CharacterReceivedEvent
        {
            get;
            private set;
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent PreviewKeyUpEvent
        {
            get;
            private set;
        }

        [CollectionType(CollectionKind.Vector)]
        [PropertyFlags(IsValueCreatedOnDemand = true, IsReadOnlyExceptForParser = true, NeedsInvoke = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Input.KeyboardAcceleratorCollection KeyboardAccelerators
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public DependencyObject KeyboardAcceleratorPlacementTarget
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = true)]
        public KeyboardAcceleratorPlacementMode KeyboardAcceleratorPlacementMode
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnKeyboardAcceleratorInvoked(KeyboardAcceleratorInvokedEventArgs args)
        {
        }

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, ProcessKeyboardAcceleratorEventArgs> ProcessKeyboardAccelerators;

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnProcessKeyboardAccelerators(ProcessKeyboardAcceleratorEventArgs args)
        {
        }

        public void TryInvokeKeyboardAccelerator(ProcessKeyboardAcceleratorEventArgs args)
        {
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent BringIntoViewRequestedEvent
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnBringIntoViewRequested(Microsoft.UI.Xaml.BringIntoViewRequestedEventArgs e)
        {
        }
        
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean Focus(Microsoft.UI.Xaml.FocusState value)
        {
            return default(Windows.Foundation.Boolean);
        }

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.Input.GettingFocusEventArgs> GettingFocus;

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.Input.LosingFocusEventArgs> LosingFocus;

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.Input.NoFocusCandidateFoundEventArgs> NoFocusCandidateFound;

        [EventFlags(IsControlEvent = true, IsImplVirtual = true)]
        public event Microsoft.UI.Xaml.Input.KeyEventHandler PreviewKeyDown;

        [EventFlags(IsControlEvent = true, IsImplVirtual = true)]
        public event Microsoft.UI.Xaml.Input.KeyEventHandler PreviewKeyUp;

        [EventFlags(IsControlEvent = true)]
        public event Windows.Foundation.TypedEventHandler<UIElement, Microsoft.UI.Xaml.BringIntoViewRequestedEventArgs> BringIntoViewRequested;

        [NativeStorageType(OM.ValueType.valueEnum)]
        [PropertyFlags(IsValueInherited = true)]
        public Microsoft.UI.Xaml.ElementHighContrastAdjustment HighContrastAdjustment
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "TabFocusNavigation")]
        [NativeStorageType(OM.ValueType.valueEnum)]
        public Microsoft.UI.Xaml.Input.KeyboardNavigationMode TabFocusNavigation
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static void RegisterAsScrollPort(UIElement element)
        {
        }

        [Strictness(Strictness.StrictOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.ScalarTransition OpacityTransition
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Vector3 Translation
        {
            get;
            set;
        }

        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Translation")]
        public event Microsoft.UI.Xaml.EventHandler TranslationChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Vector3 AnimatedTranslation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Vector3Transition TranslationTransition
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedTranslation")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedTranslationChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CoreType(typeof(Windows.Foundation.Double))]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Float Rotation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Rotation")]
        public event Microsoft.UI.Xaml.EventHandler RotationChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [CoreType(typeof(Windows.Foundation.Double))]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Float AnimatedRotation
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.ScalarTransition RotationTransition
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedRotation")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedRotationChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 1.0f, 1.0f, 1.0f }")]
        public Windows.Foundation.Numerics.Vector3 Scale
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("Scale")]
        public event Microsoft.UI.Xaml.EventHandler ScaleChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 1.0f, 1.0f, 1.0f }")]
        internal Windows.Foundation.Numerics.Vector3 AnimatedScale
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Vector3Transition ScaleTransition
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedScale")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedScaleChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Matrix4x4 TransformMatrix
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("TransformMatrix")]
        public event Microsoft.UI.Xaml.EventHandler TransformMatrixChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Matrix4x4 AnimatedTransformMatrix
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedTransformMatrix")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedTransformMatrixChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        public Windows.Foundation.Numerics.Vector3 CenterPoint
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("CenterPoint")]
        public event Microsoft.UI.Xaml.EventHandler CenterPointChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        internal Windows.Foundation.Numerics.Vector3 AnimatedCenterPoint
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedCenterPoint")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedCenterPointChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 0.0f, 0.0f, 1.0f }")]
        public Windows.Foundation.Numerics.Vector3 RotationAxis
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("RotationAxis")]
        public event Microsoft.UI.Xaml.EventHandler RotationAxisChanged;

        [Strictness(Strictness.StrictOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "{ 0.0f, 0.0f, 1.0f }")]
        internal Windows.Foundation.Numerics.Vector3 AnimatedRotationAxis
        {
            get;
            set;
        }

        [Strictness(Strictness.StrictOnly)]
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [SimplePropertyEvent("AnimatedRotationAxis")]
        internal event Microsoft.UI.Xaml.EventHandler AnimatedRotationAxisChanged;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Windows.Foundation.Numerics.Vector3 ActualOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [DependencyPropertyModifier(Modifier.Private)]
        [ReadOnly]
        public Windows.Foundation.Numerics.Vector2 ActualSize
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void StartAnimation(Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void StopAnimation(Microsoft.UI.Composition.ICompositionAnimationBase animation)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void PopulatePropertyInfoOverride(Windows.Foundation.String propertyName, Microsoft.UI.Composition.AnimationPropertyInfo animationPropertyInfo)
        {
        }

        [AllowCrossThreadAccess]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.RoutedEvent ContextRequestedEvent
        {
            get;
            private set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public XamlRoot XamlRoot { get; set;}

        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true, IsImplVirtual = true)]
        public event Microsoft.UI.Xaml.EventHandler Shown;

        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true, IsImplVirtual = true)]
        public event Microsoft.UI.Xaml.EventHandler Hidden;

        [VelocityFeature("Feature_XamlMotionSystemHoldbacks")]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse)]
        private int KeepAliveCount
        {
            get;
            set;
        }

        [PropertyFlags(IsValueInherited = false)]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(OM.ValueType.valueObject)]
        public Microsoft.UI.Xaml.Media.Shadow Shadow
        {
            get;
            set;
        }

        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "0")]
        private int ThemeShadowReceiverCount
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [SimpleProperty(SimplePropertyStorage.Sparse, DefaultValue = "1.0")]
        public Windows.Foundation.Double RasterizationScale
        {
            get;
            set;
        }

        [ReadOnly]
        public Microsoft.UI.Xaml.FocusState FocusState
        {
            get;
            private set;
        }

        public Windows.Foundation.Boolean UseSystemFocusVisuals
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusLeft
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusRight
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusUp
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject XYFocusDown
        {
            get;
            set;
        }

        [NativeMethod("CUIElement", "IsTabStopPropertyGetterSetter")]
        [NativeStorageType(OM.ValueType.valueBool)]
        public Windows.Foundation.Boolean IsTabStop
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 TabIndex
        {
            get;
            set;
        }

    }
}
