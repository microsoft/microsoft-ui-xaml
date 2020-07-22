// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Linq;
using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ActivityFeedSamplePage : Page
    {
        public ActivityFeedSamplePage()
        {
            this.InitializeComponent();
            repeater.ItemTemplate = elementFactory;
            repeater.ItemsSource = Enumerable.Range(0, 500);
        }
    }
}
