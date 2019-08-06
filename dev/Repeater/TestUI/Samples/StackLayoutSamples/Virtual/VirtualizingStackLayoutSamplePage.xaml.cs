// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utils;
using System;
using System.Collections.ObjectModel;
using System.Collections.Specialized;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using MUXControlsTestApp.Samples.Model;

using ItemsSourceView = Microsoft.UI.Xaml.Controls.ItemsSourceView;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ElementFactory = Microsoft.UI.Xaml.Controls.ElementFactory;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class VirtualizingStackLayoutSamplePage : Page
    {
        private string _lorem = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus. Lorem ipsum dolor sit amet, consectetur adipiscing elit. Etiam laoreet erat vel massa rutrum, eget mollis massa vulputate. Vivamus semper augue leo, eget faucibus nulla mattis nec. Donec scelerisque lacus at dui ultricies, eget auctor ipsum placerat. Integer aliquet libero sed nisi eleifend, nec rutrum arcu lacinia. Sed a sem et ante gravida congue sit amet ut augue. Donec quis pellentesque urna, non finibus metus. Proin sed ornare tellus.";

        public VirtualizingStackLayoutSamplePage()
        {
            this.InitializeComponent();
            repeater.ItemTemplate = elementFactory;

            var rnd = new Random();
            var data = new ObservableCollection<Recipe>(Enumerable.Range(0, 300).Select(k =>
                           new Recipe
                           {
                               ImageUri = new Uri(string.Format("ms-appx:///Images/recipe{0}.png", k % 8 + 1)),
                               Description = k + " - " + _lorem.Substring(0, rnd.Next(50, 350))
                           }));

            repeater.ItemsSource = data;
            bringIntoView.Click += BringIntoView_Click;
        }

        private void BringIntoView_Click(object sender, RoutedEventArgs e)
        {
            int index = 0;
            if (int.TryParse(tb.Text, out index))
            {
                var anchor = repeater.GetOrCreateElement(index);
                anchor.StartBringIntoView();
            }
        }
    }
}
