// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class UniformGridLayoutDemo : Page
    {
        public IEnumerable<int> collection;

        public UniformGridLayoutDemo()
        {
            collection = Enumerable.Range(0, 40);
            this.InitializeComponent();
        }

        public void GetRepeaterActualHeightButtonClick(object sender, RoutedEventArgs e)
        {
            RepeaterActualHeightLabel.Text = UniformGridRepeater.ActualHeight.ToString();
        }
    }
}
