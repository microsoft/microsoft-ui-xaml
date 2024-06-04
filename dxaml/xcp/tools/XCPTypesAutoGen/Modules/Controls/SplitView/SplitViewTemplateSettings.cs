// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using OM;
using XamlOM;

namespace Microsoft.UI.Xaml.Controls.Primitives
{
    [NativeName("CSplitViewTemplateSettings")]
    [DXamlIdlGroup("Controls2")]
    [Guids(ClassGuid = "d3646b32-9a6e-4d57-b897-b3fbba994e38")]
    public sealed class SplitViewTemplateSettings
     : Microsoft.UI.Xaml.DependencyObject
    {
        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OpenPaneLength
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeOpenPaneLength
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double OpenPaneLengthMinusCompactLength
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Windows.Foundation.Double NegativeOpenPaneLengthMinusCompactLength
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.GridLength OpenPaneGridLength
        {
            get;
            internal set;
        }

        [DependencyPropertyModifier(Modifier.Private)]
        public Microsoft.UI.Xaml.GridLength CompactPaneGridLength
        {
            get;
            internal set;
        }

        internal SplitViewTemplateSettings() { }
    }
}
