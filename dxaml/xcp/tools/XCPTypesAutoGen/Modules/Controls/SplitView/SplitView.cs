// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using Microsoft.UI.Xaml.Markup;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls
{
    [DXamlIdlGroup("Controls2")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum SplitViewPanePlacement
    {
        Left = 0,
        Right = 1,
    }

    [DXamlIdlGroup("Controls2")]
    [EnumFlags(HasTypeConverter = true)]
    public enum SplitViewDisplayMode
    {
        Overlay = 0,
        Inline = 1,
        CompactOverlay = 2,
        CompactInline = 3
    }

    [DXamlIdlGroup("Controls2")]
    [NativeName("CSplitViewPaneClosingEventArgs")]
    [ClassFlags(IsVisibleInXAML = false)]
    [Guids(ClassGuid = "f43a58c6-ab0b-42be-aa33-43c050c29659")]
    public sealed class SplitViewPaneClosingEventArgs
     : Microsoft.UI.Xaml.EventArgs
    {
        [OffsetFieldName("m_cancel")]
        [DelegateToCore]
        public Windows.Foundation.Boolean Cancel { get; set; }

        internal SplitViewPaneClosingEventArgs() { }
    }

    [CodeGen(partial: true)]
    [NativeName("CSplitView")]
    [DXamlIdlGroup("Controls2")]
    [ContentProperty("Content")]
    [InstanceCountTelemetry]
    [Guids(ClassGuid = "abe890c2-bd57-423e-98f3-dd3ccd2d974c")]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    public class SplitView
     : Microsoft.UI.Xaml.Controls.Control
    {
        [PropertyFlags(AffectsMeasure = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement Content { get; set; }

        [PropertyFlags(AffectsMeasure = true, IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.UIElement Pane { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        [NativeStorageType(ValueType.valueBool)]
        [OffsetFieldName("m_isPaneOpen")]
        public Windows.Foundation.Boolean IsPaneOpen { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Double OpenPaneLength { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public Windows.Foundation.Double CompactPaneLength { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public SplitViewPanePlacement PanePlacement { get; set; }

        [PropertyFlags(AffectsMeasure = true)]
        public SplitViewDisplayMode DisplayMode { get; set; }

        [OffsetFieldName("m_pTemplateSettings")]
        [NativeStorageType(ValueType.valueObject)]
        public Microsoft.UI.Xaml.Controls.Primitives.SplitViewTemplateSettings TemplateSettings { get { return default(Primitives.SplitViewTemplateSettings); } }

        public Microsoft.UI.Xaml.Media.Brush PaneBackground { get; set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.SplitViewPaneClosingEventHandler PaneClosing;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.SplitViewPaneClosedEventHandler PaneClosed;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.SplitViewPaneOpeningEventHandler PaneOpening;

        [EventHandlerType(EventHandlerKind.TypedSenderAndArgs)]
        [EventFlags(UseEventManager = true)]
        public event Microsoft.UI.Xaml.Controls.SplitViewPaneOpenedEventHandler PaneOpened;

        public SplitView() { }
    }

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    public delegate void SplitViewPaneClosingEventHandler(Microsoft.UI.Xaml.Controls.SplitView sender, Microsoft.UI.Xaml.Controls.SplitViewPaneClosingEventArgs e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    public delegate void SplitViewPaneClosedEventHandler(Microsoft.UI.Xaml.Controls.SplitView sender, Windows.Foundation.Object e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    public delegate void SplitViewPaneOpeningEventHandler(Microsoft.UI.Xaml.Controls.SplitView sender, Windows.Foundation.Object e);

    [StubDelegate]
    [CodeGen(CodeGenLevel.LookupOnly)]
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    public delegate void SplitViewPaneOpenedEventHandler(Microsoft.UI.Xaml.Controls.SplitView sender, Windows.Foundation.Object e);
}
