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

#if !BUILD_WINDOWS
using NumberBox = Microsoft.UI.Xaml.Controls.NumberBox;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class NumberBoxPage : TestPage
    {
        public NumberBoxPage()
        {
            this.InitializeComponent();
        }
    }
}
