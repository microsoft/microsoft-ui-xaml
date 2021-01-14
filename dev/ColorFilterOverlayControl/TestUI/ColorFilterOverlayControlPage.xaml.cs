// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Markup;
using Windows.UI;
using System.Windows.Input;

using ColorFilterOverlayControl = Microsoft.UI.Xaml.Controls.Primitives.ColorFilterOverlayControl;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "ColorFilterOverlayControl")]
    public sealed partial class ColorFilterOverlayControlPage : TestPage
    {
        public ColorFilterOverlayControlPage()
        {
            this.InitializeComponent();
        }
    }
}
