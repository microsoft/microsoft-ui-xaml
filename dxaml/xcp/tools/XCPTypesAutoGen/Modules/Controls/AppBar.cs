// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "7b4c2f42-033d-404e-b24f-53f702e62938")]
    public sealed class AppBarTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Rect ClipRect { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double CompactVerticalDelta { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Thickness CompactRootMargin { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double MinimalVerticalDelta { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Thickness MinimalRootMargin { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double HiddenVerticalDelta { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Thickness HiddenRootMargin { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeCompactVerticalDelta { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeMinimalVerticalDelta { get; internal set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeHiddenVerticalDelta { get; internal set; }

        internal AppBarTemplateSettings() { }
    }
}

namespace Microsoft.UI.Xaml.Controls
{
    [FrameworkTypePattern]
    [DXamlIdlGroup("Controls2")]
    [EnumFlags(HasTypeConverter = true, IsExcludedFromNative = true)]
    public enum AppBarClosedDisplayMode
    {
        Compact = 0,
        Minimal = 1,
        Hidden = 2,
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [InstanceCountTelemetry]
    [Implements(typeof(Microsoft.Internal.FrameworkUdk.IBackButtonPressedListener))]
    [Guids(ClassGuid = "8b02a325-a1ee-434f-aeaa-131cf438b6d3")]
    public class AppBar
     : Microsoft.UI.Xaml.Controls.ContentControl
    {
        public Windows.Foundation.Boolean IsOpen { get; set; }

        public Windows.Foundation.Boolean IsSticky { get; set; }

        public Microsoft.UI.Xaml.Controls.AppBarClosedDisplayMode ClosedDisplayMode { get; set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.AppBarTemplateSettings TemplateSettings { get; private set; }

        public Microsoft.UI.Xaml.Controls.LightDismissOverlayMode LightDismissOverlayMode { get; set; }

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Opening;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Opened;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Closing;

        [EventHandlerType(EventHandlerKind.TypedArgs)]
        public event Microsoft.UI.Xaml.EventHandler Closed;

        public AppBar() { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnClosed(Windows.Foundation.Object e) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnOpened(Windows.Foundation.Object e) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnClosing(Windows.Foundation.Object e) { }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        protected virtual void OnOpening(Windows.Foundation.Object e) { }

    }
}
