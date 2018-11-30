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
using NEWCONTROL = Microsoft.UI.Xaml.Controls.NEWCONTROL;
#endif

namespace MUXControlsTestApp
{
    public sealed partial class NEWCONTROLPage : TestPage
    {
        public NEWCONTROLPage()
        {
            this.InitializeComponent();
        }
    }
}
