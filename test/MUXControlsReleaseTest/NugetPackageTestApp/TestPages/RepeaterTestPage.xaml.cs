using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp;

namespace NugetPackageTestApp
{
    public sealed partial class RepeaterTestPage : TestPage
    {
        private ObservableCollection<string> items;

        public RepeaterTestPage()
        {
            this.InitializeComponent();

            this.items = new ObservableCollection<string>();
            this.Repeater.ItemsSource = items;
        }

        public void OnAddItemsButtonClick(Object sender, RoutedEventArgs e)
        {
            items.Add("Item1");
            items.Add("Item2");
            items.Add("Item3");
        }
    }
}
