// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.OnPropertyChangeCallback)]
    [Guids(ClassGuid = "d26867db-2367-4cbc-84b8-61cd5ea2d4ba")]
    public class Pivot : Controls.ItemsControl
    {
        public Windows.Foundation.Object Title
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Microsoft.UI.Xaml.DataTemplate TitleTemplate
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Windows.Foundation.Object LeftHeader
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Microsoft.UI.Xaml.DataTemplate LeftHeaderTemplate
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Windows.Foundation.Object RightHeader
        {
            get;
            set;
        }

        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        public Microsoft.UI.Xaml.DataTemplate RightHeaderTemplate
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.DataTemplate HeaderTemplate
        {
            get;
            set;
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

        public Windows.Foundation.Boolean IsLocked
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Controls.PivotHeaderFocusVisualPlacement HeaderFocusVisualPlacement
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public PivotSlideInAnimationGroup SlideInAnimationGroup
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public bool IsHeaderItemsCarouselEnabled
        {
            get;
            set;
        }

        [Attached(TargetType = typeof(Microsoft.UI.Xaml.FrameworkElement))]
        [PropertyChange(PropertyChangeCallbackType.NoCallback)]
        internal Windows.Foundation.Object SlideInElementInformation
        {
            get;
            set;
        }

        public event Microsoft.UI.Xaml.Controls.SelectionChangedEventHandler SelectionChanged;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.PivotItemLoadingEventHandler PivotItemLoading;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.PivotItemLoadedEventHandler PivotItemLoaded;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.PivotItemUnloadingEventHandler PivotItemUnloading;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.PivotItemUnloadedEventHandler PivotItemUnloaded;
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Phone")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PivotHeaderFocusVisualPlacement
    {
        ItemHeaders,
        SelectedItemHeader,
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "2ae0c86d-c538-4d47-8fb1-bb262cbee9d1")]
    public class PivotItem : Controls.ContentControl
    {
        public Windows.Foundation.Object Header
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [ContentProperty("Content")]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "6a56efac-16c1-401d-86bf-78777519b5c0")]
    public sealed class PickerFlyout : Microsoft.UI.Xaml.Controls.Primitives.PickerFlyoutBase
    {
        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.PickerConfirmedEventHandler Confirmed;

        public Microsoft.UI.Xaml.UIElement Content
        {
            get;
            set;
        }

        public Windows.Foundation.Boolean ConfirmationButtonsVisible
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.Stub)]
        protected override Windows.Foundation.Boolean ShouldShowConfirmationButtons()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.Boolean> ShowAtAsync(Microsoft.UI.Xaml.FrameworkElement target)
        {
            return null;
        }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "7827b49a-66f6-46f3-8698-f7bf5ae92a99")]
    public sealed class PickerFlyoutPresenter : Microsoft.UI.Xaml.Controls.ContentControl
    {
        internal PickerFlyoutPresenter() { }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.OnPropertyChangeCallback)]
    [Guids(ClassGuid = "85f354c0-e063-4129-a76c-5950923d950a")]
    public sealed class ListPickerFlyout : Microsoft.UI.Xaml.Controls.Primitives.PickerFlyoutBase
    {
        public Windows.Foundation.Object ItemsSource
        {
            get;
            set;
        }

        public Microsoft.UI.Xaml.DataTemplate ItemTemplate
        {
            get;
            set;
        }

        public string DisplayMemberPath
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Microsoft.UI.Xaml.Controls.ListPickerFlyoutSelectionMode SelectionMode
        {
            get;
            set;
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

        [DependencyPropertyModifier(Modifier.Private)]
        [CollectionType(CollectionKind.Vector)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.ItemCollection SelectedItems
        {
            get;
            private set;
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.ItemsPickedEventHandler ItemsPicked;

        [CodeGen(CodeGenLevel.Stub)]
        protected override Windows.Foundation.Boolean ShouldShowConfirmationButtons()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.Collections.IVectorView<Windows.Foundation.Object>> ShowAtAsync(Microsoft.UI.Xaml.FrameworkElement target)
        {
            return null;
        }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "a31cbece-710f-40ea-9068-ad9ae5f1cad6")]
    public sealed class ListPickerFlyoutPresenter : Controls.Control
    {
        internal ListPickerFlyoutPresenter() { }

        [DependencyPropertyModifier(Modifier.Internal)]
        internal Microsoft.UI.Xaml.Controls.ListViewBase ItemsHost
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "a7b89128-9a9e-4918-8046-15a4f4a659f0")]
    public sealed class DatePickerFlyout : Microsoft.UI.Xaml.Controls.Primitives.PickerFlyoutBase
    {
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.String CalendarIdentifier
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.DateTime Date
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean DayVisible
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean MonthVisible
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean YearVisible
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.DateTime MinYear
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.DateTime MaxYear
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.DatePickedEventHandler DatePicked;

        [CodeGen(CodeGenLevel.Stub)]
        protected override Windows.Foundation.Boolean ShouldShowConfirmationButtons()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.DateTime?> ShowAtAsync(Microsoft.UI.Xaml.FrameworkElement target)
        {
            return null;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.String DayFormat
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.String MonthFormat
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.String YearFormat
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "8e14ff78-2883-4eb5-9152-cb01c37d50d7")]
    public sealed class DatePickerFlyoutPresenter : Controls.Control
    {
        internal DatePickerFlyoutPresenter() { }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean IsDefaultShadowEnabled { get; set; }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "8a819c6d-77f5-45b4-9e93-608e879dc5ef")]
    public sealed class TimePickerFlyout : Microsoft.UI.Xaml.Controls.Primitives.PickerFlyoutBase
    {
        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.String ClockIdentifier
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.TimeSpan Time
        {
            get;
            set;
        }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Int32 MinuteIncrement
        {
            get;
            set;
        }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.TimePickedEventHandler TimePicked;

        [CodeGen(CodeGenLevel.Stub)]
        protected override Windows.Foundation.Boolean ShouldShowConfirmationButtons()
        {
            return default(Windows.Foundation.Boolean);
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.IAsyncOperation<Windows.Foundation.TimeSpan?> ShowAtAsync(Microsoft.UI.Xaml.FrameworkElement target)
        {
            return null;
        }

    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "d44de20b-6ea6-450d-a487-fc55ba279531")]
    public sealed class TimePickerFlyoutPresenter : Controls.Control
    {
        internal TimePickerFlyoutPresenter() { }

        [PropertyInitialization(PropertyInitializationType.CallbackRetrievedValue)]
        public Windows.Foundation.Boolean IsDefaultShadowEnabled { get; set; }
    }

    [DXamlIdlGroup("Phone")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Implements(typeof(Microsoft.UI.Xaml.Data.ICustomPropertyProvider))]
    [Guids(ClassGuid = "50461953-ba4e-4cd8-aea0-1d79215db0bc")]
    public sealed class DatePickerFlyoutItem : DependencyObject
    {
        internal DatePickerFlyoutItem() { }

        public Windows.Foundation.String PrimaryText
        {
            get;
            set;
        }

        public Windows.Foundation.String SecondaryText
        {
            get;
            set;
        }
    }

    //
    // Enumerations
    //

    [DXamlIdlGroup("Phone")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum ListPickerFlyoutSelectionMode
    {
        Single,
        Multiple,
    }

    [DXamlIdlGroup("Phone")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum PivotSlideInAnimationGroup
    {
        Default = 0,
        GroupOne = 1,
        GroupTwo = 2,
        GroupThree = 3,
    }

    // 
    // Events
    //

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "395da899-178d-4d7f-9149-edd15c39b48d")]
    public sealed class PivotItemEventArgs
        : Microsoft.UI.Xaml.EventArgs
    {
        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        public Microsoft.UI.Xaml.Controls.PivotItem Item
        {
            get;
            set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [Guids(ClassGuid = "b0932efa-1cc0-48f0-a463-111e9d504153")]
    public sealed class PickerConfirmedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "74d46c25-4a68-4175-954b-d3be04f36299")]
    public sealed class DatePickedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.DateTime OldDate
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.DateTime NewDate
        {
            get;
            internal set;
        }

    }


    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "94c8fc76-382d-421e-8a9d-d6b042597fd7")]
    public sealed class TimePickedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.TimeSpan OldTime
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        public Windows.Foundation.TimeSpan NewTime
        {
            get;
            internal set;
        }
    }

    [TypeFlags(IsCreateableFromXAML = false)]
    [DXamlIdlGroup("Phone")]
    [CodeGen(partial: true)]
    [PropertyChange(PropertyChangeCallbackType.NoCallback)]
    [Guids(ClassGuid = "308aeb10-3397-477d-9395-1b0e922a94d4")]
    public sealed class ItemsPickedEventArgs
        : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Internal)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection AddedItems
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Internal)]
        [CollectionType(CollectionKind.Vector)]
        public Microsoft.UI.Xaml.Controls.ItemCollection RemovedItems
        {
            get;
            internal set;
        }
    }

    //
    // Delegates
    //

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PivotItemLoadingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.PivotItemEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PivotItemLoadedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.PivotItemEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PivotItemUnloadingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.PivotItemEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PivotItemUnloadedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.PivotItemEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void PickerConfirmedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.PickerConfirmedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DatePickedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.DatePickedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void TimePickedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.TimePickedEventArgs e);

    [StubDelegate]
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void ItemsPickedEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.ItemsPickedEventArgs e);

}

namespace Microsoft.UI.Xaml.Media.Animation
{
    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [ContentProperty("DefaultNavigationTransitionInfo")]
    [Guids(ClassGuid = "3604db5e-afa3-450d-8740-8fbd7417c81a")]
    public sealed class NavigationThemeTransition
     : Microsoft.UI.Xaml.Media.Animation.Transition
    {
        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        public Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo DefaultNavigationTransitionInfo
        {
            get;
            set;
        }
    }

    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Phone")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum SlideNavigationTransitionEffect
    {
        FromBottom,
        FromLeft,
        FromRight
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "844848b0-9a23-4244-9103-35ccf66661fd")]
    public sealed class SlideNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {
        public SlideNavigationTransitionEffect Effect
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "f2eb7bfb-1866-4eb6-9486-82878e22f3e4")]
    public sealed class DrillInNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {

    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "fff17b53-9239-4a7c-9258-e24512542c73")]
    public sealed class SuppressNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {

    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "ebf201df-d4e5-4ac5-852a-2cc81da4084c")]
     public sealed class EntranceNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {
        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static bool AttachedIsTargetElement
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "786c702f-0268-4e04-bfa1-c0e44ba631f0")]
    public sealed class CommonNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {
        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        public bool IsStaggeringEnabled
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static bool AttachedIsStaggerElement
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Phone")]
    [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
    [Guids(ClassGuid = "adb786b3-9c7c-4659-837d-4f7affbd3fbb")]
    public sealed class ContinuumNavigationTransitionInfo : Microsoft.UI.Xaml.Media.Animation.NavigationTransitionInfo
    {
        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static bool AttachedIsEntranceElement
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        public Microsoft.UI.Xaml.UIElement ExitElement
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.UIElement))]
        public static bool AttachedIsExitElement
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndNoTypeTableEntries)]
        [Attached(TargetType = typeof(Microsoft.UI.Xaml.Controls.ListViewBase))]
        public static bool AttachedExitElementContainer
        {
            get;
            set;
        }
    }
}
