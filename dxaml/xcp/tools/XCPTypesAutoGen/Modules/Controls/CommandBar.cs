// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "5d2ffc7f-c0c3-42f8-9e14-3a28a18a727a")]
    public sealed class CommandBarTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double ContentHeight { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Rect OverflowContentClipRect { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentMinWidth { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentMaxWidth { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentMaxHeight { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentHorizontalOffset { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentHeight { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeOverflowContentHeight { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Visibility EffectiveOverflowButtonVisibility { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentCompactYTranslation { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentMinimalYTranslation { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OverflowContentHiddenYTranslation { get; internal set; }

        internal CommandBarTemplateSettings() { }
    }
}

namespace Microsoft.UI.Xaml.Controls
{
    [Platform(1, typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum CommandBarDefaultLabelPosition
    {
        Bottom = 0,
        Right = 1,
        Collapsed = 2
    }

    [Platform(1, typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum CommandBarLabelPosition
    {
        Default = 0,
        Collapsed = 1
    }

    [Platform(1, typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum CommandBarOverflowButtonVisibility
    {
        Auto = 0,
        Visible = 1,
        Collapsed = 2
    }

    [DXamlIdlGroup("Controls2")]
    [TypeTable(IsExcludedFromCore = true)]
    public interface ICommandBarElement
    {
        Windows.Foundation.Boolean IsCompact
        {
            get;
            set;
        }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Windows.Foundation.Boolean IsInOverflow
        {
            get;
        }

        Windows.Foundation.Int32 DynamicOverflowOrder
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true)]
    public interface ICommandBarOverflowElement
    {
        Windows.Foundation.Boolean UseOverflowStyle
        {
            get;
            set;
        }
    }

    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromCore = true)]
    public interface ICommandBarLabeledElement
    {
        void SetDefaultLabelPosition(Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition defaultLabelPosition);
        Windows.Foundation.Boolean GetHasBottomLabel();
    }

    [CodeGen(partial: true)]
    [ClassFlags(IsObservable = true, IsHiddenFromIdl = true)]
    [NativeName("CCommandBarElementCollection")]
    [OldCodeGenBaseType(typeof(Microsoft.UI.Xaml.Collections.PresentationFrameworkCollection<ICommandBarElement>))]
    [TypeTable(IsExcludedFromVisualTree = true, IsExcludedFromReferenceTrackerWalk = true)]
    [Guids(ClassGuid = "054a1b31-5c5c-40e5-914c-072e92198657")]
    public sealed class CommandBarElementCollection
     : Microsoft.UI.Xaml.Collections.ObservablePresentationFrameworkCollection<ICommandBarElement>
    {
        [CoreType(typeof(Microsoft.UI.Xaml.DependencyObject))]
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.ICommandBarElement ContentProperty
        {
            get;
            set;
        }

        internal CommandBarElementCollection() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [NativeName("CCommandBar")]
    [InstanceCountTelemetry]
    [ContentProperty("PrimaryCommands")]
    [TemplatePart("PrimaryItemsControl", typeof(Microsoft.UI.Xaml.Controls.ItemsControl))]
    [TemplatePart("SecondaryItemsControl", typeof(Microsoft.UI.Xaml.Controls.ItemsControl))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarStaticsPrivate), IsStaticInterface = true)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.IMenu))]
    [Guids(ClassGuid = "07a2d60d-8cd0-4673-a1af-7897772c0c48")]
    public class CommandBar
     : Microsoft.UI.Xaml.Controls.AppBar
    {
        [CollectionType(CollectionKind.Observable)]
        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.CommandBarElementCollection PrimaryCommands { get; private set; }

        [CollectionType(CollectionKind.Observable)]
        [PropertyFlags(IsReadOnlyExceptForParser = true)]
        [ReadOnly]
        public Microsoft.UI.Xaml.Controls.CommandBarElementCollection SecondaryCommands { get; private set; }

        public Microsoft.UI.Xaml.Style CommandBarOverflowPresenterStyle
        {
            get;
            set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.CommandBarTemplateSettings CommandBarTemplateSettings { get; private set; }

        public Microsoft.UI.Xaml.Controls.CommandBarDefaultLabelPosition DefaultLabelPosition { get; set; }

        public Microsoft.UI.Xaml.Controls.CommandBarOverflowButtonVisibility OverflowButtonVisibility { get; set; }

        public Windows.Foundation.Boolean IsDynamicOverflowEnabled { get; set; }

        public CommandBar() { }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        public event Microsoft.UI.Xaml.Controls.DynamicOverflowItemsChangingEventHandler DynamicOverflowItemsChanging;
    }

    // The class factory's private static interface must be declared after the class,
    // if it uses the class type.
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(PrivateApiContract), 1)]
    [TypeTable(IsExcludedFromDXaml = true, IsExcludedFromCore = true)]
    public interface ICommandBarStaticsPrivate
    {
        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        Microsoft.UI.Xaml.Controls.CommandBar GetCurrentBottomCommandBar(Microsoft.UI.Xaml.XamlRoot xamlRoot);
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [FrameworkTypePattern]
    [Guids(ClassGuid = "6a056867-ab9a-42fa-a472-7f0a626a1b62")]
    public class CommandBarOverflowPresenter
     : Microsoft.UI.Xaml.Controls.ItemsControl
    {
        public CommandBarOverflowPresenter() { }
    }

    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum CommandBarDynamicOverflowAction
    {
        AddingToOverflow = 0,
        RemovingFromOverflow = 1,
    }

    [DXamlIdlGroup("Controls2")]
    [TypeFlags(IsCreateableFromXAML = false)]
    [FrameworkTypePattern]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "91c393d4-c11a-4d8e-9c1a-248c41ba46e3")]
    public sealed class DynamicOverflowItemsChangingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.CommandBarDynamicOverflowAction Action
        {
            get;
            internal set;
        }

        public DynamicOverflowItemsChangingEventArgs() { }
    }

    [StubDelegate]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [CodeGen(CodeGenLevel.LookupOnly)]
    public delegate void DynamicOverflowItemsChangingEventHandler(Windows.Foundation.Object sender, Microsoft.UI.Xaml.Controls.DynamicOverflowItemsChangingEventArgs e);
}
