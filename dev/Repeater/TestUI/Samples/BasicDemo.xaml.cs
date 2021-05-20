// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;
using RecyclingElementFactory = Microsoft.UI.Xaml.Controls.RecyclingElementFactory;
using SelectTemplateEventArgs = Microsoft.UI.Xaml.Controls.SelectTemplateEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class BasicDemo : TestPage
    {
        public ObservableCollection<string> simpleStringsList = new ObservableCollection<string>();

        public BasicDemo()
        {
            this.InitializeComponent();
            repeater.ItemTemplate = elementFactory;
            var stack = repeater.Layout as StackLayout;
            int numItems = (stack != null && stack.DisableVirtualization) ? 10 : 10000;
            repeater.ItemsSource = Enumerable.Range(0, numItems).Select(x => x.ToString());
        }

        private void OnAddRecipeButton_Click(object sender, RoutedEventArgs e)
        {
            InsertAtStartChildCountLabel.Text = VisualTreeHelper.GetChildrenCount(insertStartTestRepeater).ToString();
            simpleStringsList.Insert(0,"Item" + simpleStringsList.Count );
        }

        private void OnSelectTemplateKey(RecyclingElementFactory sender, SelectTemplateEventArgs args)
        {
            args.TemplateKey = (int.Parse(args.DataContext.ToString()) % 2 == 0) ? "even" : "odd";
        }
    }
}
