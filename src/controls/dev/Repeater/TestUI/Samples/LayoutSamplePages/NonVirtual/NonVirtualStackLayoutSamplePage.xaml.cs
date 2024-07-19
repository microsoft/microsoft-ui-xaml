﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utils;
using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Linq;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class NonVirtualStackLayoutSamplePage : Page
    {
        public NonVirtualStackLayoutSamplePage()
        {
            this.InitializeComponent();
            repeater.ItemsSource = Enumerable.Range(0, 10);
        }
    }
}
