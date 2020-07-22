// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class StoreDemoPage : Page
    {
        public StoreDemoPage()
        {
            this.InitializeComponent();
            var data = StoreMockData.Create(15, 10);
            outerRepeater.ItemsSource = data;
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = args.DataContext is Category ? "Category" : "Item";
        }
    }
}
