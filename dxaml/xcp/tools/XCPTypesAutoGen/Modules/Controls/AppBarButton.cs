// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "e84e0164-2a66-4ca7-8e49-8be256167a00")]
    public sealed class AppBarButtonTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KeyboardAcceleratorTextMinWidth
        {
            get;
            internal set;
        }

        internal AppBarButtonTemplateSettings() { }
    }

    [DXamlIdlGroup("Controls2")]
    [Platform(typeof(Microsoft.UI.Xaml.WinUIContract), 1)]
    [Guids(ClassGuid = "b2044b08-f453-401d-8b89-7a076ece1138")]
    public sealed class AppBarToggleButtonTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double KeyboardAcceleratorTextMinWidth
        {
            get;
            internal set;
        }

        internal AppBarToggleButtonTemplateSettings() { }
    }
}

namespace Microsoft.UI.Xaml.Controls
{
    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarElement))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarOverflowElement), Version = 1, Order = 2)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarLabeledElement), Version = 1, Order = 3)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ISubMenuOwner), Version = 1, Order = 6)]
    [Guids(ClassGuid = "bb9dc4e8-e180-4474-bc99-027996196f5f")]
    public class AppBarButton
     : Microsoft.UI.Xaml.Controls.Button
    {
        public Windows.Foundation.String Label { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.IconElement Icon { get; set; }

        public Microsoft.UI.Xaml.Controls.CommandBarLabelPosition LabelPosition { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.String KeyboardAcceleratorTextOverride { get; set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.AppBarButtonTemplateSettings TemplateSettings { get; private set; }

        public AppBarButton() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarElement))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarOverflowElement), Version = 1, Order = 2)]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarLabeledElement), Version = 1, Order = 3)]
    [Guids(ClassGuid = "755c72cc-519d-4d29-9923-9165ae798ac9")]
    public class AppBarToggleButton
     : Microsoft.UI.Xaml.Controls.Primitives.ToggleButton
    {
        public Windows.Foundation.String Label { get; set; }

        [PropertyFlags(IsExcludedFromVisualTree = true)]
        public Microsoft.UI.Xaml.Controls.IconElement Icon { get; set; }

        public Microsoft.UI.Xaml.Controls.CommandBarLabelPosition LabelPosition { get; set; }

        [CodeGen(CodeGenLevel.IdlAndPartialStub)]
        public Windows.Foundation.String KeyboardAcceleratorTextOverride { get; set; }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.Controls.Primitives.AppBarToggleButtonTemplateSettings TemplateSettings { get; private set; }

        public AppBarToggleButton() { }
    }

    [CodeGen(partial: true)]
    [DXamlIdlGroup("Controls2")]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarElement))]
    [Implements(typeof(Microsoft.UI.Xaml.Controls.ICommandBarOverflowElement), Version = 1, Order = 2)]
    [Guids(ClassGuid = "efd45a66-f1fa-4468-ba76-91d62f08d3d7")]
    public class AppBarSeparator
     : Microsoft.UI.Xaml.Controls.Control
    {
        public AppBarSeparator() { }
    }
}
