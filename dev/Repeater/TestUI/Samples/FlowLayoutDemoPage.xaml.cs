// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class FlowLayoutDemoPage : Page
    {
        public FlowLayoutDemoPage()
        {
            this.InitializeComponent();

            Random r = new Random();
            repeater.ItemsSource = (from i in Enumerable.Range(0, 1000)
                               select new MyItem() {
                                   Icon = (Symbol)(r.Next((int)0xE100, (int)0xE200)),
                                   Label = i.ToString() + " " + LoremIpsum.Substring(0, Math.Max(5, r.Next(LoremIpsum.Length)))
                               }).ToList();
        }

        private const string LoremIpsum = "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";
    }

    public class MyItem
    {
        public string Label { get; set; }
        public Symbol Icon { get; set; }
    }
}
