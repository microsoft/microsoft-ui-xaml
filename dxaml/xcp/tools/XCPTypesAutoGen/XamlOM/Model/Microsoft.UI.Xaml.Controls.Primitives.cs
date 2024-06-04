// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [CodeGen(CodeGenLevel.LookupOnly)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    [HandWritten]
    [Guids(ClassGuid = "608438ad-9343-41e7-a548-7b8999ba78b2")]
    public static class LayoutInformation
    {
        [ReturnTypeParameterName("element")]
        public static Microsoft.UI.Xaml.UIElement GetLayoutExceptionElement(object dispatcher)
        {
            return null;
        }

        [ReturnTypeParameterName("slot")]
        public static Windows.Foundation.Rect GetLayoutSlot(FrameworkElement element)
        {
            return default(Windows.Foundation.Rect);
        }

        [ReturnTypeParameterName("availableSize")]
        public static Windows.Foundation.Size GetAvailableSize(UIElement element)
        {
            return default(Windows.Foundation.Size);
        }
    }

    [ClassFlags(IsHiddenFromIdl = true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "d8b34289-4e60-4991-aa34-71410f123960")]
    public abstract class ListViewBaseItemTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        internal ListViewBaseItemTemplateSettings() { }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "92dfa119-7bcd-4e38-be2d-b5da7d207a76")]
    public sealed class ListViewItemTemplateSettings
     : Microsoft.UI.Xaml.Controls.Primitives.ListViewBaseItemTemplateSettings
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Int32 DragItemsCount
        {
            get;
            internal set;
        }

        internal ListViewItemTemplateSettings() { }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "c0222939-04c4-42f4-bed4-bb6b83a10fff")]
    public sealed class GridViewItemTemplateSettings
     : Microsoft.UI.Xaml.Controls.Primitives.ListViewBaseItemTemplateSettings
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Int32 DragItemsCount
        {
            get;
            internal set;
        }

        internal GridViewItemTemplateSettings() { }
    }

    [Guids(ClassGuid = "39993b65-1c2a-45e3-b7ab-a4e7ddadd6aa")]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    public sealed class ComboBoxTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double DropDownOpenedHeight
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double DropDownClosedHeight
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double DropDownOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.AnimationDirection SelectedItemDirection
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double DropDownContentMinWidth
        {
            get;
            internal set;
        }

        internal ComboBoxTemplateSettings() { }
    }

    [Guids(ClassGuid = "d8af2956-b38f-4a75-b4c1-c6aa4e837c08")]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    public sealed class MenuFlyoutPresenterTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double FlyoutContentMinWidth
        {
            get;
            internal set;
        }

        internal MenuFlyoutPresenterTemplateSettings() { }
    }

    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "c0c8200d-8072-450f-a7a4-500df815e7ec")]
    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    public sealed class MenuFlyoutItemTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KeyboardAcceleratorTextMinWidth
        {
            get;
            internal set;
        }

        internal MenuFlyoutItemTemplateSettings() { }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "18dd595b-4dc8-46ca-9b78-ca401c0c3d94")]
    public sealed class ToolTipTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double FromHorizontalOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double FromVerticalOffset
        {
            get;
            internal set;
        }

        internal ToolTipTemplateSettings() { }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "eb2d4ff5-d111-4301-a691-6eb7538dfc79")]
    public sealed class ToggleSwitchTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KnobCurrentToOnOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KnobCurrentToOffOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KnobOnToOffOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KnobOffToOnOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CurtainCurrentToOnOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CurtainCurrentToOffOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CurtainOnToOffOffset
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CurtainOffToOnOffset
        {
            get;
            internal set;
        }

        internal ToggleSwitchTemplateSettings() { }
    }

    [CodeGen(partial: true)]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "5752c258-e1ad-41e1-9747-38b10fec6b3a")]
    public class ToggleButton
     : Microsoft.UI.Xaml.Controls.Primitives.ButtonBase
    {
        public Windows.Foundation.Boolean? IsChecked
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsThreeState
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.RoutedEventHandler Checked;

        public event Microsoft.UI.Xaml.RoutedEventHandler Unchecked;

        public event Microsoft.UI.Xaml.RoutedEventHandler Indeterminate;

        public ToggleButton() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnToggle()
        {
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "bf6b7629-ce9e-436c-a3c5-7b39c238ca46")]
    public sealed class RepeatButton
     : Microsoft.UI.Xaml.Controls.Primitives.ButtonBase
    {
        public Windows.Foundation.Int32 Delay
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 Interval
        {
            get;
            set;
        }

        public RepeatButton() { }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "12abc41a-bba3-4a29-96a7-1be5ef3ff7a7")]
    public abstract class ButtonBase
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public Microsoft.UI.Xaml.Controls.ClickMode ClickMode
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean IsPointerOver
        {
            get;
            internal set;
        }

        public Windows.Foundation.Boolean IsPressed
        {
            get;
            internal set;
        }

        public Microsoft.UI.Xaml.Input.ICommand Command
        {
            get;
            set;
        }

        public Windows.Foundation.Object CommandParameter
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.RoutedEventHandler Click;

        public ButtonBase() { }
    }

    internal interface IScrollInfo
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean CanVerticallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean CanHorizontallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double ExtentWidth
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double ExtentHeight
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double ViewportWidth
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double ViewportHeight
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double MinHorizontalOffset
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double MinVerticalOffset
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double HorizontalOffset
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Double VerticalOffset
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Object ScrollOwner
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void LineUp();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void LineDown();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void LineLeft();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void LineRight();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void PageUp();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void PageDown();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void PageLeft();

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void PageRight();

        [CodeGen(CodeGenLevel.Idl)]
        void MouseWheelUp(Windows.Foundation.UInt32 mouseWheelDelta);

        [CodeGen(CodeGenLevel.Idl)]
        void MouseWheelDown(Windows.Foundation.UInt32 mouseWheelDelta);

        [CodeGen(CodeGenLevel.Idl)]
        void MouseWheelLeft(Windows.Foundation.UInt32 mouseWheelDelta);

        [CodeGen(CodeGenLevel.Idl)]
        void MouseWheelRight(Windows.Foundation.UInt32 mouseWheelDelta);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetHorizontalOffset(Windows.Foundation.Double offset);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void SetVerticalOffset(Windows.Foundation.Double offset);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        void MakeVisible(
            Microsoft.UI.Xaml.UIElement visual,
            Windows.Foundation.Rect rectangle,
            Windows.Foundation.Boolean useAnimation,
            Windows.Foundation.Double horizontalAlignmentRatio,
            Windows.Foundation.Double verticalAlignmentRatio,
            Windows.Foundation.Double offsetX,
            Windows.Foundation.Double offsetY,
            out Windows.Foundation.Rect resultRectangle,
            out Windows.Foundation.Double appliedOffsetX,
            out Windows.Foundation.Double appliedOffsetY);
    }

    public interface IScrollSnapPointsInfo
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean AreHorizontalSnapPointsRegular
        {
            get;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        Windows.Foundation.Boolean AreVerticalSnapPointsRegular
        {
            get;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        event Microsoft.UI.Xaml.EventHandler HorizontalSnapPointsChanged;

        [TypeTable(IsExcludedFromCore = true)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        event Microsoft.UI.Xaml.EventHandler VerticalSnapPointsChanged;

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Collections.IVectorView<Windows.Foundation.Float> GetIrregularSnapPoints(Microsoft.UI.Xaml.Controls.Orientation orientation, Microsoft.UI.Xaml.Controls.Primitives.SnapPointsAlignment alignment);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Float GetRegularSnapPoints(Microsoft.UI.Xaml.Controls.Orientation orientation, Microsoft.UI.Xaml.Controls.Primitives.SnapPointsAlignment alignment, [CoreType(typeof(Windows.Foundation.Double))] out Windows.Foundation.Float offset);
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "463a80e0-2629-420c-a58b-ca2a049a669b")]
    public sealed class ScrollBar
     : Microsoft.UI.Xaml.Controls.Primitives.RangeBase
    {
        [FieldBacked]
        public Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Double ViewportSize
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.Controls.Primitives.ScrollingIndicatorMode IndicatorMode
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.Primitives.ScrollEventHandler Scroll;

        internal event Microsoft.UI.Xaml.Controls.Primitives.DragStartedEventHandler ThumbDragStarted;

        internal event Microsoft.UI.Xaml.Controls.Primitives.DragCompletedEventHandler ThumbDragCompleted;

        public ScrollBar() { }

        protected override void OnMinimumChanged(Windows.Foundation.Double oldMinimum, Windows.Foundation.Double newMinimum)
        {
        }

        protected override void OnMaximumChanged(Windows.Foundation.Double oldMaximum, Windows.Foundation.Double newMaximum)
        {
        }

        protected override void OnValueChanged(Windows.Foundation.Double oldValue, Windows.Foundation.Double newValue)
        {
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "2333f37b-d79f-4a68-a2de-ffe767d7171d")]
    public sealed class ScrollEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double NewValue
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.Primitives.ScrollEventType ScrollEventType
        {
            get;
            internal set;
        }

        public ScrollEventArgs() { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "52ad6e1b-c540-4189-84f9-4c1e6ad3cde4")]
    public class DragStartedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            internal set;
        }

        internal DragStartedEventArgs() { }

        [FactoryMethodName("CreateInstanceWithHorizontalOffsetAndVerticalOffset")]
        public DragStartedEventArgs(Windows.Foundation.Double horizontalOffset, Windows.Foundation.Double verticalOffset) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "aa87d427-7d00-449c-994a-a36fb2e2fe6f")]
    public class DragDeltaEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double HorizontalChange
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double VerticalChange
        {
            get;
            internal set;
        }

        internal DragDeltaEventArgs() { }

        [FactoryMethodName("CreateInstanceWithHorizontalChangeAndVerticalChange")]
        public DragDeltaEventArgs(Windows.Foundation.Double horizontalChange, Windows.Foundation.Double verticalChange) { }
    }

    [CodeGen(partial: true)]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "b2710805-5d88-454d-99e4-f6fb002b35bc")]
    public class DragCompletedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double HorizontalChange
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double VerticalChange
        {
            get;
            internal set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean Canceled
        {
            get;
            internal set;
        }

        internal DragCompletedEventArgs() { }

        [FactoryMethodName("CreateInstanceWithHorizontalChangeVerticalChangeAndCanceled")]
        public DragCompletedEventArgs(Windows.Foundation.Double horizontalChange, Windows.Foundation.Double verticalChange, Windows.Foundation.Boolean canceled) { }
    }

    [CodeGen(partial: true)]
    [NativeName("CRangeBase")]
    [Guids(ClassGuid = "478496af-bcc2-4c59-aeff-65f260ee218b")]
    public abstract class RangeBase
     : Microsoft.UI.Xaml.Controls.Control
    {
        [FieldBacked]
        public Windows.Foundation.Double Minimum
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Double Maximum
        {
            get;
            set;
        }

        public Windows.Foundation.Double SmallChange
        {
            get;
            set;
        }

        public Windows.Foundation.Double LargeChange
        {
            get;
            set;
        }

        [FieldBacked]
        public Windows.Foundation.Double Value
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventHandler ValueChanged;

        public RangeBase() { }

        [TypeTable(IsExcludedFromCore = true)]
        [NativeClassName("RangeBase")]
        protected virtual void OnMinimumChanged(Windows.Foundation.Double oldMinimum, Windows.Foundation.Double newMinimum)
        {
        }

        [TypeTable(IsExcludedFromCore = true)]
        [NativeClassName("RangeBase")]
        protected virtual void OnMaximumChanged(Windows.Foundation.Double oldMaximum, Windows.Foundation.Double newMaximum)
        {
        }

        [TypeTable(IsExcludedFromCore = true)]
        [NativeClassName("RangeBase")]
        protected virtual void OnValueChanged(Windows.Foundation.Double oldValue, Windows.Foundation.Double newValue)
        {
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "dfe8786a-b69e-4138-9bc6-8cc643ba04b4")]
    public sealed class Thumb
     : Microsoft.UI.Xaml.Controls.Control
    {
        public Windows.Foundation.Boolean IsDragging
        {
            get;
            internal set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Boolean IsPointerOver
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.Primitives.DragStartedEventHandler DragStarted;

        public event Microsoft.UI.Xaml.Controls.Primitives.DragDeltaEventHandler DragDelta;

        public event Microsoft.UI.Xaml.Controls.Primitives.DragCompletedEventHandler DragCompleted;

        public Thumb() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void CancelDrag()
        {
        }
    }

    [CodeGen(partial: true)]
    [NativeName("CPopup")]
    [ContentProperty("Child")]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Implements(typeof(Microsoft.UI.Composition.ICompositionSupportsSystemBackdrop), Version = 3)]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 2)]
    [Platform(3, typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    [Guids(ClassGuid = "ab84a122-f289-4c08-9c1b-6589d5189a29")]
    public sealed class Popup
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [RequiresMultipleAssociationCheck]
        [NativeMethod("CPopup", "Child")]
        public Microsoft.UI.Xaml.UIElement Child { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fIsOpen")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Boolean IsOpen { get; set; }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eHOffset")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double HorizontalOffset { get; set; }

        [NativeStorageType(ValueType.valueFloat)]
        [OffsetFieldName("m_eVOffset")]
        [RenderDirtyFlagClassName("CUIElement")]
        [RenderDirtyFlagMethodName("NWSetContentAndBoundsDirty")]
        public Windows.Foundation.Double VerticalOffset { get; set; }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_pChildTransitions")]
        [PropertyFlags(IsValueCreatedOnDemand = true)]
        public Microsoft.UI.Xaml.Media.Animation.TransitionCollection ChildTransitions { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fIsLightDismissEnabled")]
        public Windows.Foundation.Boolean IsLightDismissEnabled { get; set; }

        [NativeStorageType(ValueType.valueEnum)]
        [OffsetFieldName("m_lightDismissOverlayMode")]
        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        public Windows.Foundation.Boolean ShouldConstrainToRootBounds { get; set; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean IsConstrainedToRootBounds { get; }

        [Version(2)]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.FrameworkElement PlacementTarget { get; set; }

        [Version(2)]
        public Microsoft.UI.Xaml.Controls.Primitives.PopupPlacementMode DesiredPlacement { get; set; }

        [Version(2)]
        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.Controls.Primitives.PopupPlacementMode ActualPlacement { get; }

        [RequiresMultipleAssociationCheck]
        [Version(3)]
        // Don't associate with the m_systemBackdrop field. We'll set the field manually in CPopup and do other things at the same time.
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop { get; set; }

        [NativeMethod("CPopup", "AssociatedFlyout")]
        internal Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase AssociatedFlyout { get; set; }

        [NativeStorageType(ValueType.valueObject)]
        [OffsetFieldName("m_overlayElement")]
        internal Microsoft.UI.Xaml.FrameworkElement OverlayElement { get; private set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        internal Microsoft.UI.Xaml.DependencyObject OverlayInputPassThroughElement { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_disableOverlayIsLightDismissCheck")]
        internal Windows.Foundation.Boolean DisableOverlayIsLightDismissCheck { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fIsApplicationBarService")]
        internal Windows.Foundation.Boolean IsApplicationBarService { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fIsContentDialog")]
        internal Windows.Foundation.Boolean IsContentDialog { get; set; }

        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_fIsSubMenu")]
        internal Windows.Foundation.Boolean IsSubMenu { get; set; }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Opened;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler Closed;

        [Version(2)]
        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.EventHandler ActualPlacementChanged;

        public Popup() { }
    }

    [CodeGen(partial: true)]
    [ClassFlags(IsISupportInitialize = true)]
    [Implements(typeof(Microsoft.UI.Xaml.ComponentModel.ISupportInitialize))]
    [Guids(ClassGuid = "ea21182d-8ec7-468c-998f-2cd20f52f288")]
    public abstract class Selector
     : Microsoft.UI.Xaml.Controls.ItemsControl
    {
        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        internal Windows.Foundation.Boolean CanSelectMultiple
        {
            get;
            private set;
        }

        public Windows.Foundation.Int32 SelectedIndex
        {
            get;
            set;
        }

        public Windows.Foundation.Object SelectedItem
        {
            get;
            set;
        }

        public Windows.Foundation.Object SelectedValue
        {
            get;
            set;
        }

        public Windows.Foundation.String SelectedValuePath
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean? IsSynchronizedWithCurrentItem
        {
            get;
            set;
        }

        internal Windows.Foundation.Boolean IsSelectionActive
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.SelectionChangedEventHandler SelectionChanged;


        [CodeGen(CodeGenLevel.Excluded)]
        internal Selector() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public static Windows.Foundation.Boolean GetIsSelectionActive(Microsoft.UI.Xaml.DependencyObject element)
        {
            return default(Windows.Foundation.Boolean);
        }
    }

    [CodeGen(partial: true)]
    [Guids(ClassGuid = "fc7f5516-860f-4955-84f5-1da1c81c5516")]
    public abstract class SelectorItem
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        internal Windows.Foundation.Boolean IsPointerOver
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [FieldBacked]
        public Windows.Foundation.Boolean IsSelected
        {
            get;
            set;
        }

        public SelectorItem() { }
    }

    [FrameworkTypePattern]
    [TypeFlags(IsDXamlSystemType = true)]
    internal struct ElementInfo
    {
    }

    [DXamlIdlGroup("Main")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "4ec3881d-c807-45fa-9b38-16561969e6b6")]
    public struct GeneratorPosition
    {
        public Windows.Foundation.Int32 Index
        {
            get;
            set;
        }

        public Windows.Foundation.Int32 Offset
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.Idl)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition FromIndexAndOffset(Windows.Foundation.Int32 index, Windows.Foundation.Int32 offset)
        {
            return default(Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition);
        }
    }

    [FrameworkTypePattern]
    [TypeTable(IsExcludedFromNewTypeTable = true)]
    [Guids(ClassGuid = "d64e4fc4-086a-44e3-a6a6-e6cd7cdcd07b")]
    public sealed class ItemsChangedEventArgs
    {
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 Action
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition Position
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Microsoft.UI.Xaml.Controls.Primitives.GeneratorPosition OldPosition
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 ItemCount
        {
            get;
            internal set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Int32 ItemUICount
        {
            get;
            internal set;
        }

        internal ItemsChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [CoreBaseType(typeof(Microsoft.UI.Xaml.Controls.Panel))]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "8e9eec1f-706a-4f47-be93-e78f9ae876d1")]
    public sealed class TickBar
     : Microsoft.UI.Xaml.FrameworkElement
    {
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Media.Brush Fill
        {
            get;
            set;
        }

        public TickBar() { }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IOrientedPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IItemLookupPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IPaginatedPanel))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IInsertionPanel))]
    [Guids(ClassGuid = "c1bff631-094e-4b61-a970-4e5b6dd92261")]
    public abstract class OrientedVirtualizingPanel
     : Microsoft.UI.Xaml.Controls.VirtualizingPanel
    {
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        [DependencyPropertyModifier(Modifier.Internal)]
        internal static Windows.Foundation.Boolean AttachedIsContainerGeneratedForInsert
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanVerticallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanHorizontallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object ScrollOwner
        {
            get;
            set;
        }

        internal OrientedVirtualizingPanel() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetHorizontalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetVerticalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Rect MakeVisible(Microsoft.UI.Xaml.UIElement visual, Windows.Foundation.Rect rectangle)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [CodeGen(partial: true)]
    [FrameworkTypePattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollInfo))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.Primitives.IScrollSnapPointsInfo))]
    [Guids(ClassGuid = "1542faa5-e39e-4210-b15f-f5e188e24de7")]
    public class CarouselPanel
     : Microsoft.UI.Xaml.Controls.VirtualizingPanel
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanVerticallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Boolean CanHorizontallyScroll
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ExtentHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportWidth
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double ViewportHeight
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double HorizontalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public Windows.Foundation.Double VerticalOffset
        {
            get;
            private set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Object ScrollOwner
        {
            get;
            set;
        }

        public CarouselPanel() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void LineRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void PageRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelUp()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelDown()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelLeft()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void MouseWheelRight()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetHorizontalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public void SetVerticalOffset(Windows.Foundation.Double offset)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Rect MakeVisible(Microsoft.UI.Xaml.UIElement visual, Windows.Foundation.Rect rectangle)
        {
            return default(Windows.Foundation.Rect);
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "dc32416f-263a-48d3-ba91-b3a60127041e")]
    public sealed class RangeBaseValueChangedEventArgs
     : Microsoft.UI.Xaml.RoutedEventArgs
    {
        [Comment("Should be declared as IsReadOnly=True")]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double OldValue
        {
            get;
            internal set;
        }

        [Comment("Should be declared as IsReadOnly=True")]
        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Double NewValue
        {
            get;
            internal set;
        }

        internal RangeBaseValueChangedEventArgs() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Main")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [Guids(ClassGuid = "29fd3b8c-df98-437d-9761-8f1d3904739e")]
    public class FlyoutShowOptions
    {
        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Point? Position
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public Windows.Foundation.Rect? ExclusionRect
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public FlyoutShowMode ShowMode
        {
            get;
            set;
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        public FlyoutPlacementMode Placement
        {
            get;
            set;
        }

        public FlyoutShowOptions() { }
    }

    [Platform(typeof(PrivateApiContract), 1)]
    public interface IFlyoutBasePrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean UsePickerFlyoutTheme
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        void PlaceFlyoutForDateTimePicker(Windows.Foundation.Point point);

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        Windows.Foundation.Boolean IsLightDismissOverlayEnabled
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Main")]
    [CodeGen(partial: true)]
    [NativeName("CFlyoutBase")]
    [Implements(typeof(IFlyoutBasePrivate))]
    [Guids(ClassGuid = "938a03da-5470-4111-8b38-200a86db33f6")]
    [Platform(2, typeof(Microsoft.UI.Xaml.WinUIContract), 5)]
    public abstract class FlyoutBase
     : Microsoft.UI.Xaml.DependencyObject
    {
        public FlyoutBase() { }

        [TypeTable(IsExcludedFromCore = true)]
        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutPlacementMode Placement { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [TypeTable(IsExcludedFromCore = true)]
        [DependencyPropertyModifier(Modifier.Internal)]
        public Microsoft.UI.Xaml.FrameworkElement Target { get; internal set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [ReadOnly]
        public static Microsoft.UI.Xaml.DependencyProperty TargetProperty { get; }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Boolean AllowFocusOnInteraction { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        [PropertyFlags(IsValueInherited = true)]
        public Windows.Foundation.Boolean AllowFocusWhenDisabled { get; set; }

        public Microsoft.UI.Xaml.Controls.Primitives.FlyoutShowMode ShowMode { get; set; }

        public Windows.Foundation.Boolean InputDevicePrefersPrimaryCommands { get; private set; }

        public Windows.Foundation.Boolean AreOpenCloseAnimationsEnabled { get; set; }

        public Windows.Foundation.Boolean ShouldConstrainToRootBounds { get; set; }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean IsConstrainedToRootBounds { get; }

        [RequiresMultipleAssociationCheck]
        [Version(2)]
        public Microsoft.UI.Xaml.Media.SystemBackdrop SystemBackdrop { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void ShowAt(Microsoft.UI.Xaml.FrameworkElement placementTarget)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [DXamlName("ShowAtWithOptions")]
        [DXamlOverloadName("ShowAt")]
        public void ShowAtWithOptions([Optional] Microsoft.UI.Xaml.DependencyObject placementTarget, [Optional] FlyoutShowOptions showOptions)
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public void Hide()
        {
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        protected virtual Microsoft.UI.Xaml.Controls.Control CreatePresenter()
        {
            return default(Microsoft.UI.Xaml.Controls.Control);
        }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.EventHandler Opened;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.EventHandler Closed;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event Microsoft.UI.Xaml.EventHandler Opening;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [TypeTable(IsExcludedFromCore = true)]
        public event FlyoutBaseClosingEventHandler Closing;

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyFlags(IsExcludedFromVisualTree = true)]
        [TypeTable(IsExcludedFromCore = true)]
        public static Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase AttachedAttachedFlyout { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        public static void ShowAttachedFlyout(Microsoft.UI.Xaml.FrameworkElement flyoutOwner)
        {
        }

        [NativeStorageType(ValueType.valueEnum)]
        public Microsoft.UI.Xaml.ElementSoundMode ElementSoundMode
        {
            get;
            set;
        }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.DependencyObject OverlayInputPassThroughElement
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.Boolean IsOpen
        {
            get;
        }

        public void TryInvokeKeyboardAccelerator(Microsoft.UI.Xaml.Input.ProcessKeyboardAcceleratorEventArgs args)
        {
        }

        [TypeTable(IsExcludedFromCore = true)]
        protected virtual void OnProcessKeyboardAccelerators(Microsoft.UI.Xaml.Input.ProcessKeyboardAcceleratorEventArgs args)
        {
        }

        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Microsoft.UI.Xaml.XamlRoot XamlRoot {set; get; }
    }

    [DXamlIdlGroup("Main")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "8c49f507-4b48-4b20-b840-8a143df8bb8e")]
    public sealed class FlyoutBaseClosingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        public Windows.Foundation.Boolean Cancel { get; set; }

        internal FlyoutBaseClosingEventArgs() { }
    }

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void FlyoutBaseClosingEventHandler(Microsoft.UI.Xaml.Controls.Primitives.FlyoutBase sender, Microsoft.UI.Xaml.Controls.Primitives.FlyoutBaseClosingEventArgs e);

    [Comment("Represents the second, private layer of chrome in a ListViewBaseItem")]
    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [NativeName("CListViewBaseItemSecondaryChrome")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "bb3055ca-b3da-4750-94c7-2e621425f4ed")]
    internal class ListViewBaseItemSecondaryChrome
     : Microsoft.UI.Xaml.FrameworkElement
    {
        public ListViewBaseItemSecondaryChrome() { }

    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("ListViewItemPresenterCheckMode")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum ListViewItemPresenterCheckMode
    {
        [NativeValueName("ListViewItemPresenterCheckModeInline")]
        Inline = 0,
        [NativeValueName("ListViewItemPresenterCheckModeOverlay")]
        Overlay = 1,
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("ListViewItemPresenterSelectionIndicatorMode")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true)]
    public enum ListViewItemPresenterSelectionIndicatorMode
    {
        [NativeValueName("ListViewItemPresenterSelectionIndicatorModeInline")]
        Inline = 0,
        [NativeValueName("ListViewItemPresenterSelectionIndicatorModeOverlay")]
        Overlay = 1,
    }

    [Comment("Represents the chrome in a ListViewBaseItem")]
    [CodeGen(partial: true)]
    [ClassFlags(IsHiddenFromIdl = true)]
    [NativeName("CListViewBaseItemChrome")]
    [TypeTable(IsExcludedFromDXaml = true)]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "ce067044-972a-4fd8-b884-de2d3f2d411d")]
    public abstract class ListViewBaseItemPresenter
     : Microsoft.UI.Xaml.Controls.ContentPresenter
    {
        public ListViewBaseItemPresenter() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CListViewItemChrome")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "db9feda1-2a2c-42a8-be1a-7deb6837c638")]
    public class ListViewItemPresenter
     : Microsoft.UI.Xaml.Controls.Primitives.ListViewBaseItemPresenter
    {
        public ListViewItemPresenter() { }

        [Comment("control whether selection check mark is shown or not")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueBool)]
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Boolean SelectionCheckMarkVisualEnabled { get; set; }

        [Comment("Checkmark behind the item, shown when haven't swiped far enough to select. Identical to below, only shown at 50% opacity.")]
        [OffsetFieldName("m_pCheckHintBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckHintBrush { get; set; }

        [Comment("Checkmark behind the item, shown when swiped far enough to select.")]
        [OffsetFieldName("m_pCheckSelectingBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckSelectingBrush { get; set; }

        [Comment("Checkmark above the item, shown when selected. Not same color as above 2 in Light Theme.")]
        [OffsetFieldName("m_pCheckBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBrush { get; set; }

        [Comment("Color of multi-drag DND overlay. Semitransparent.")]
        [OffsetFieldName("m_pDragBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush DragBackground { get; set; }

        [Comment("Color of multi-drag DND overlay item count.")]
        [OffsetFieldName("m_pDragForeground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush DragForeground { get; set; }

        [Comment("Shown when has keyboard focus.")]
        [OffsetFieldName("m_pFocusBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush FocusBorderBrush { get; set; }

        [Comment("Background fill of placeholder item.")]
        [OffsetFieldName("m_pPlaceholderBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PlaceholderBackground { get; set; }

        [Comment("Background fill of pointer over. NOTE! Backgrounds can and do mix if, say, user mousing over placeholder.")]
        [OffsetFieldName("m_pPointerOverBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PointerOverBackground { get; set; }

        [Comment("Selection border color. When selected: Background of item.")]
        [OffsetFieldName("m_pSelectedBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedBackground { get; set; }

        [Comment("Foreground set on the ContentPresenter for: Pointer over, Selecting, Selected, SelectedSwiping, SelectedUnfocused.")]
        [OffsetFieldName("m_pSelectedForeground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedForeground { get; set; }

        [Comment("Only when selected: Selection background color becomes this on pointer over, pointer press.")]
        [OffsetFieldName("m_pSelectedPointerOverBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPointerOverBackground { get; set; }

        [Comment("In same visual states as above, except it's the selection border color. Identical to above.")]
        [OffsetFieldName("m_pSelectedPointerOverBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPointerOverBorderBrush { get; set; }

        [Comment("Thickness of selection border.")]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Thickness SelectedBorderThickness { get; set; }

        [Comment("Opacity of ContentPresenter (only) when disabled.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double DisabledOpacity { get; set; }

        [Comment("Opacity of InnerDragContent when dragging.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double DragOpacity { get; set; }

        [Comment("Amount containers move for reorder hints.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double ReorderHintOffset { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.HorizontalContentAlignment instead of ListViewItemPresenterHorizontalContentAlignment. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.HorizontalAlignment ListViewItemPresenterHorizontalContentAlignment { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.VerticalContentAlignment instead of ListViewItemPresenterVerticalContentAlignment. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.VerticalAlignment ListViewItemPresenterVerticalContentAlignment { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.Padding instead of GridViewItemPresenterPadding. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.Thickness ListViewItemPresenterPadding { get; set; }


        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness PointerOverBackgroundMargin { get; set; }

        [OffsetFieldName("m_contentMargin")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness ContentMargin { get; set; }

        [Comment("Background/Border color becomes this when pressed and selected")]
        [OffsetFieldName("m_pSelectedPressedBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPressedBackground { get; set; }

        [Comment("Background/Border color becomes this when pressed.")]
        [OffsetFieldName("m_pPressedBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PressedBackground { get; set; }

        [Comment("Brush for the check box background")]
        [OffsetFieldName("m_pCheckBoxBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxBrush { get; set; }

        [Comment("Shown when has keyboard focus.")]
        [OffsetFieldName("m_pFocusSecondaryBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush FocusSecondaryBorderBrush { get; set; }

        [OffsetFieldName("m_checkMode")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true, AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        public ListViewItemPresenterCheckMode CheckMode { get; set; }

        [Comment("Foreground set on the ContentPresenter for: Pointer over.")]
        [OffsetFieldName("m_pPointerOverForeground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PointerOverForeground { get; set; }

        [Comment("Brush set for reveal background.")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush RevealBackground { get; set; }

        [Comment("Brush set for reveal border")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush RevealBorderBrush { get; set; }

        [Comment("Thickness for reveal border")]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Thickness RevealBorderThickness { get; set; }

        [Comment("Draw reveal over content or under Content")]
        [NativeStorageType(ValueType.valueBool)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Windows.Foundation.Boolean RevealBackgroundShowsAboveContent { get; set; }

        [Comment("Brush for the selected disabled background")]
        [OffsetFieldName("m_pSelectedDisabledBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedDisabledBackground { get; set; }

        [Comment("Brush for the pressed selection checkmark")]
        [OffsetFieldName("m_pCheckPressedBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckPressedBrush { get; set; }

        [Comment("Brush for the disabled selection checkmark")]
        [OffsetFieldName("m_pCheckDisabledBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckDisabledBrush { get; set; }

        [Comment("Brush for the pointer-over multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxPointerOverBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxPointerOverBrush { get; set; }

        [Comment("Brush for the pressed multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxPressedBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxPressedBrush { get; set; }

        [Comment("Brush for the disabled multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxDisabledBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxDisabledBrush { get; set; }

        [Comment("Brush for the selected multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxSelectedBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxSelectedBrush { get; set; }

        [Comment("Brush for the selected pointer-over multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxSelectedPointerOverBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxSelectedPointerOverBrush { get; set; }

        [Comment("Brush for the selected pressed multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxSelectedPressedBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxSelectedPressedBrush { get; set; }

        [Comment("Brush for the selected disabled multi-select check box background")]
        [OffsetFieldName("m_pCheckBoxSelectedDisabledBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxSelectedDisabledBrush { get; set; }

        [Comment("Brush for the multi-select check box border")]
        [OffsetFieldName("m_pCheckBoxBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxBorderBrush { get; set; }

        [Comment("Brush for the pointer-over multi-select check box border")]
        [OffsetFieldName("m_pCheckBoxPointerOverBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxPointerOverBorderBrush { get; set; }

        [Comment("Brush for the pressed multi-select check box border")]
        [OffsetFieldName("m_pCheckBoxPressedBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxPressedBorderBrush { get; set; }

        [Comment("Brush for the disabled multi-select check box border")]
        [OffsetFieldName("m_pCheckBoxDisabledBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBoxDisabledBorderBrush { get; set; }

        [Comment("Corner radius set on the multi-select check box")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueCornerRadius)]
        public Microsoft.UI.Xaml.CornerRadius CheckBoxCornerRadius { get; set; }

        [Comment("Corner radius set on the multi-select check box")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueCornerRadius)]
        public Microsoft.UI.Xaml.CornerRadius SelectionIndicatorCornerRadius { get; set; }

        [Comment("Control whether selection indicator is shown or not")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueBool)]
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Boolean SelectionIndicatorVisualEnabled { get; set; }

        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true, AffectsArrange = true)]
        [NativeStorageType(ValueType.valueEnum)]
        public ListViewItemPresenterSelectionIndicatorMode SelectionIndicatorMode { get; set; }

        [Comment("Brush for the selection indicator")]
        [OffsetFieldName("m_pSelectionIndicatorBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectionIndicatorBrush { get; set; }

        [Comment("Brush for the pointer-over selection indicator")]
        [OffsetFieldName("m_pSelectionIndicatorPointerOverBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectionIndicatorPointerOverBrush { get; set; }

        [Comment("Brush for the pressed selection indicator")]
        [OffsetFieldName("m_pSelectionIndicatorPressedBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectionIndicatorPressedBrush { get; set; }

        [Comment("Brush for the disabled selection indicator")]
        [OffsetFieldName("m_pSelectionIndicatorDisabledBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectionIndicatorDisabledBrush { get; set; }

        [Comment("Brush for the outer selection border")]
        [OffsetFieldName("m_pSelectedBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedBorderBrush { get; set; }

        [Comment("Brush for the outer selection pressed border")]
        [OffsetFieldName("m_pSelectedPressedBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPressedBorderBrush { get; set; }

        [Comment("Brush for the outer selection disabled border")]
        [OffsetFieldName("m_pSelectedDisabledBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedDisabledBorderBrush { get; set; }

        [Comment("Brush for the inner selection border")]
        [OffsetFieldName("m_pSelectedInnerBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedInnerBorderBrush { get; set; }

        [Comment("Brush for the pointer-over outer border")]
        [OffsetFieldName("m_pPointerOverBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PointerOverBorderBrush { get; set; }
    }

    [CodeGen(partial: true)]
    [NativeName("CGridViewItemChrome")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "60d77662-f823-4524-9433-7e49c6d21a1c")]
    public class GridViewItemPresenter
     : Microsoft.UI.Xaml.Controls.Primitives.ListViewBaseItemPresenter
    {
        public GridViewItemPresenter() { }

        [Comment("control whether selection check mark is shown or not")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueBool)]
        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Boolean SelectionCheckMarkVisualEnabled { get; set; }

        [Comment("Checkmark behind the item, shown when haven't swiped far enough to select. Identical to below, only shown at 50% opacity.")]
        [OffsetFieldName("m_pCheckHintBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckHintBrush { get; set; }

        [Comment("Checkmark behind the item, shown when swiped far enough to select.")]
        [OffsetFieldName("m_pCheckSelectingBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckSelectingBrush { get; set; }

        [Comment("Checkmark above the item, shown when selected. Not same color as above 2 in Light Theme.")]
        [OffsetFieldName("m_pCheckBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush CheckBrush { get; set; }

        [Comment("Color of multi-drag DND overlay. Semitransparent.")]
        [OffsetFieldName("m_pDragBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush DragBackground { get; set; }

        [Comment("Color of multi-drag DND overlay item count.")]
        [OffsetFieldName("m_pDragForeground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush DragForeground { get; set; }

        [Comment("Shown when has kbd focus.")]
        [OffsetFieldName("m_pFocusBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush FocusBorderBrush { get; set; }

        [Comment("Background fill of placeholder item.")]
        [OffsetFieldName("m_pPlaceholderBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PlaceholderBackground { get; set; }

        [Comment("Background fill of pointer over. NOTE! Backgrounds can and do mix if, say, user mousing over placeholder.")]
        [OffsetFieldName("m_pPointerOverBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush PointerOverBackground { get; set; }

        [Comment("Selection border color. When selected: Background of item.")]
        [OffsetFieldName("m_pSelectedBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedBackground { get; set; }

        [Comment("Foreground set on the ContentPresenter for: Pointer over, Selecting, Selected, SelectedSwiping, SelectedUnfocused.")]
        [OffsetFieldName("m_pSelectedForeground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedForeground { get; set; }

        [Comment("Only when selected: Selection background color becomes this on pointer over, pointer press.")]
        [OffsetFieldName("m_pSelectedPointerOverBackground")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPointerOverBackground { get; set; }

        [Comment("In same visual states as above, except it's the selection border color. Identical to above.")]
        [OffsetFieldName("m_pSelectedPointerOverBorderBrush")]
        [NativeStorageType(ValueType.valueObject)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Media.Brush SelectedPointerOverBorderBrush { get; set; }

        [Comment("Thickness of selection border.")]
        [NativeStorageType(ValueType.valueThickness)]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        public Microsoft.UI.Xaml.Thickness SelectedBorderThickness { get; set; }

        [Comment("Opacity of ContentPresenter (only) when disabled.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double DisabledOpacity { get; set; }

        [Comment("Opacity of InnerDragContent when dragging.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double DragOpacity { get; set; }

        [Comment("Amount containers move for reorder hints.")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [NativeStorageType(ValueType.valueFloat)]
        public Windows.Foundation.Double ReorderHintOffset { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.HorizontalContentAlignment instead of GridViewItemPresenterHorizontalContentAlignment. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.HorizontalAlignment GridViewItemPresenterHorizontalContentAlignment { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.VerticalContentAlignment instead of GridViewItemPresenterVerticalContentAlignment. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.VerticalAlignment GridViewItemPresenterVerticalContentAlignment { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [TypeTable(IsExcludedFromCore = true)]
        [Deprecated("Use ContentPresenter.Padding instead of GridViewItemPresenterPadding. For more info, see MSDN.")]
        public Microsoft.UI.Xaml.Thickness GridViewItemPresenterPadding { get; set; }

        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness PointerOverBackgroundMargin { get; set; }

        [OffsetFieldName("m_contentMargin")]
        [RenderDirtyFlagClassName("CListViewBaseItemChrome")]
        [RenderDirtyFlagMethodName("NWSetContentDirty")]
        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueThickness)]
        public Microsoft.UI.Xaml.Thickness ContentMargin { get; set; }
    }

    [Comment("Specifies the resource location for the given component.")]
    [DXamlIdlGroup("Main")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ComponentResourceLocation
    {
        Application = 0,
        Nested = 1,
    }

    [Comment("Specifies the type of action used to raise the Scroll event.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ScrollEventType
    {
        [NativeComment("Thumb was moved a small distance. The user clicked the left(horizontal) or top(vertical) scroll arrow.")]
        [NativeValueName("ScrollEventTypeSmallDecrement")]
        SmallDecrement = 0,
        [NativeComment("Thumb was moved a small distance. The user clicked the right(horizontal) or bottom(vertical) scroll arrow.")]
        [NativeValueName("ScrollEventTypeSmallIncrement")]
        SmallIncrement = 1,
        [NativeComment("Thumb was moved a large distance. The user clicked the scroll bar to the left(horizontal) or above(vertical) the scroll box.")]
        [NativeValueName("ScrollEventTypeLargeDecrement")]
        LargeDecrement = 2,
        [NativeComment("Thumb was moved a large distance. The user clicked the scroll bar to the right(horizontal) or below(vertical) the scroll box.")]
        [NativeValueName("ScrollEventTypeLargeIncrement")]
        LargeIncrement = 3,
        [NativeComment("The Thumb moved to a new position because the user selected Scroll Here in the shortcut menu of the ScrollBar. This movement corresponds to the ScrollHereCommand. To view the shortcut menu, right-click the mouse when the pointer is over the ScrollBar.")]
        [NativeValueName("ScrollEventTypeThumbPosition")]
        ThumbPosition = 4,
        [NativeComment("The Thumb was dragged and is currently being moved due to a MouseMove event.")]
        [NativeValueName("ScrollEventTypeThumbTrack")]
        ThumbTrack = 5,
        [NativeComment("The Thumb moved to the Minimum position of the ScrollBar.")]
        [NativeValueName("ScrollEventTypeFirst")]
        First = 6,
        [NativeComment("The Thumb moved to the Maximum position of the ScrollBar.")]
        [NativeValueName("ScrollEventTypeLast")]
        Last = 7,
        [NativeComment("The Thumb was being dragged to a new position and has stopped moving.")]
        [NativeValueName("ScrollEventTypeEndScroll")]
        EndScroll = 8,
    }

    [Comment("This enum is used by the ItemContainerGenerator and its client to specify the direction in which the generator produces UI.")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum GeneratorDirection
    {
        [NativeComment("Generate forward through the item collection.")]
        [NativeValueName("GeneratorDirectionForward")]
        Forward = 0,
        [NativeComment("Generate backward through the item collection.")]
        [NativeValueName("GeneratorDirectionBackward")]
        Backward = 1,
    }

    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PlacementMode
    {
        [NativeValueName("PlacementModeBottom")]
        Bottom = 2,
        [NativeValueName("PlacementModeLeft")]
        Left = 9,
        [NativeValueName("PlacementModeMouse")]
        Mouse = 7,
        [NativeValueName("PlacementModeRight")]
        Right = 4,
        [NativeValueName("PlacementModeTop")]
        Top = 10,
    }

    [DXamlIdlGroup("Main")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum FlyoutPlacementMode
    {
        [NativeValueName("FlyoutPlacementModeTop")]
        Top = 0,
        [NativeValueName("FlyoutPlacementModeBottom")]
        Bottom = 1,
        [NativeValueName("FlyoutPlacementModeLeft")]
        Left = 2,
        [NativeValueName("FlyoutPlacementModeRight")]
        Right = 3,
        [NativeValueName("FlyoutPlacementModeFull")]
        Full = 4,
        [NativeValueName("FlyoutPlacementModeTopEdgeAlignedLeft")]
        TopEdgeAlignedLeft = 5,
        [NativeValueName("FlyoutPlacementModeTopEdgeAlignedRight")]
        TopEdgeAlignedRight = 6,
        [NativeValueName("FlyoutPlacementModeBottomEdgeAlignedLeft")]
        BottomEdgeAlignedLeft = 7,
        [NativeValueName("FlyoutPlacementModeBottomEdgeAlignedRight")]
        BottomEdgeAlignedRight = 8,
        [NativeValueName("FlyoutPlacementModeLeftEdgeAlignedTop")]
        LeftEdgeAlignedTop = 9,
        [NativeValueName("FlyoutPlacementModeLeftEdgeAlignedBottom")]
        LeftEdgeAlignedBottom = 10,
        [NativeValueName("FlyoutPlacementModeRightEdgeAlignedTop")]
        RightEdgeAlignedTop = 11,
        [NativeValueName("FlyoutPlacementModeRightEdgeAlignedBottom")]
        RightEdgeAlignedBottom = 12,
        [NativeValueName("FlyoutPlacementModeAuto")]
        Auto = 13,
    }

    [DXamlIdlGroup("Main")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum FlyoutShowMode
    {
        [NativeValueName("FlyoutShowModeAuto")]
        Auto = 0,
        [NativeValueName("FlyoutShowModeStandard")]
        Standard = 1,
        [NativeValueName("FlyoutShowModeTransient")]
        Transient = 2,
        [NativeValueName("FlyoutShowModeTransientWithDismissOnPointerMoveAway")]
        TransientWithDismissOnPointerMoveAway = 3,
    }

    [DXamlIdlGroup("Main")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    [Platform(typeof(Microsoft.UI.Xaml.XamlContract), 1)]
    public enum PopupPlacementMode
    {
        [NativeValueName("PopupPlacementModeAuto")]
        Auto = 0,
        [NativeValueName("PopupPlacementModeTop")]
        Top = 1,
        [NativeValueName("PopupPlacementModeBottom")]
        Bottom = 2,
        [NativeValueName("PopupPlacementModeLeft")]
        Left = 3,
        [NativeValueName("PopupPlacementModeRight")]
        Right = 4,
        [NativeValueName("PopupPlacementModeTopEdgeAlignedLeft")]
        TopEdgeAlignedLeft = 5,
        [NativeValueName("PopupPlacementModeTopEdgeAlignedRight")]
        TopEdgeAlignedRight = 6,
        [NativeValueName("PopupPlacementModeBottomEdgeAlignedLeft")]
        BottomEdgeAlignedLeft = 7,
        [NativeValueName("PopupPlacementModeBottomEdgeAlignedRight")]
        BottomEdgeAlignedRight = 8,
        [NativeValueName("PopupPlacementModeLeftEdgeAlignedTop")]
        LeftEdgeAlignedTop = 9,
        [NativeValueName("PopupPlacementModeLeftEdgeAlignedBottom")]
        LeftEdgeAlignedBottom = 10,
        [NativeValueName("PopupPlacementModeRightEdgeAlignedTop")]
        RightEdgeAlignedTop = 11,
        [NativeValueName("PopupPlacementModeRightEdgeAlignedBottom")]
        RightEdgeAlignedBottom = 12
    }

    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum TickPlacement
    {
        [NativeValueName("TickPlacementNone")]
        None = 0,
        [NativeValueName("TickPlacementTopLeft")]
        TopLeft = 1,
        [NativeValueName("TickPlacementBottomRight")]
        BottomRight = 2,
        [NativeValueName("TickPlacementOutside")]
        Outside = 3,
        [NativeValueName("TickPlacementInline")]
        Inline = 4,
    }

    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ScrollingIndicatorMode
    {
        [NativeValueName("ScrollingIndicatorModeNone")]
        None = 0,
        [NativeValueName("ScrollingIndicatorModeTouchIndicator")]
        TouchIndicator = 1,
        [NativeValueName("ScrollingIndicatorModeMouseIndicator")]
        MouseIndicator = 2,
    }

    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum SliderSnapsTo
    {
        [NativeValueName("SliderSnapsToStepValues")]
        StepValues = 0,
        [NativeValueName("SliderSnapsToTicks")]
        Ticks = 1,
    }

    [Comment("Determines the alignment of snap points in relation to the DirectManipulation container viewport.")]
    [NativeName("XcpSnapPointsAlignment")]
    [EnumFlags(HasTypeConverter = true, IsNativeTypeDef = true, IsTypeConverter = true)]
    [NativeComment("Determines the alignment of snap points in regards to the DirectManipulation container.")]
    public enum SnapPointsAlignment
    {
        [NativeComment("Snap points align to the left or top of the container.")]
        [NativeValueName("XcpSnapPointsAlignmentNear")]
        Near = 0,
        [NativeComment("Snap points align to the center of the container.")]
        [NativeValueName("XcpSnapPointsAlignmentCenter")]
        Center = 1,
        [NativeComment("Snap points align to the right or bottom of the container.")]
        [NativeValueName("XcpSnapPointsAlignmentFar")]
        Far = 2,
    }

    [DXamlIdlGroup("Main")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true)]
    public enum EdgeTransitionLocation
    {
        Left = 0,
        Top = 1,
        Right = 2,
        Bottom = 3,
    }

    [DXamlIdlGroup("Main")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum AnimationDirection
    {
        Left = 0,
        Top = 1,
        Right = 2,
        Bottom = 3,
    }

    public delegate void DragStartedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.DragStartedEventArgs e);

    public delegate void DragDeltaEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.DragDeltaEventArgs e);

    public delegate void DragCompletedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.DragCompletedEventArgs e);

    public delegate void ScrollEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.ScrollEventArgs e);

    public delegate void ItemsChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.ItemsChangedEventArgs e);

    public delegate void RangeBaseValueChangedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.Primitives.RangeBaseValueChangedEventArgs e);

    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum GroupHeaderPlacement
    {
        Top = 0,
        Left = 1,
    }


    [DXamlIdlGroup("Controls2")]
    [CodeGen(partial: true)]
    [ControlPattern]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IOrientedPanel))]
    [Guids(ClassGuid = "b6927e47-7e23-4d44-a729-1517febe8049")]
    public sealed class CalendarPanel
        : Microsoft.UI.Xaml.Controls.ModernCollectionBasePanel
    {

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The dimension in which the items are arranged.")]
        internal Microsoft.UI.Xaml.Controls.Orientation Orientation
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The number of items per column showing in ScrollViewer's viewport")]
        internal Windows.Foundation.Int32 Rows
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The number of items per row showing in ScrollViewer's viewport")]
        internal Windows.Foundation.Int32 Cols
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        [Comment("The position we start the first item. it should be less than Cols (vertical) or Rows (horizontal)")]
        internal Windows.Foundation.Int32 StartIndex
        {
            get;
            set;
        }

        [Comment("The minwidth of each item in the CalendarPanel.")]
        [TypeTable(IsExcludedFromCore = true)]
        internal Windows.Foundation.Double ItemMinWidth
        {
            get;
            set;
        }

        [Comment("The minheight of each item in the CalendarPanel.")]
        [TypeTable(IsExcludedFromCore = true)]
        internal Windows.Foundation.Double ItemMinHeight
        {
            get;
            set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        internal Windows.Foundation.Int32 FirstCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        internal Windows.Foundation.Int32 FirstVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        internal Windows.Foundation.Int32 LastVisibleIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        internal Windows.Foundation.Int32 LastCacheIndex
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromDXaml = true)]
        [PropertyKind(PropertyKind.PropertyOnly)]
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        [ReadOnly]
        internal Microsoft.UI.Xaml.Controls.PanelScrollingDirection ScrollingDirection
        {
            get;
            private set;
        }

        [TypeTable(IsExcludedFromCore = true)]
        internal Windows.Foundation.Double CacheLength
        {
            get;
            set;
        }
    }

}
