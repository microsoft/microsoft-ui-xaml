// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using ConditionalControls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;


namespace Conditionals
{
    public sealed partial class NameEventsLoad : UserControl
    {
        public NameEventsLoad()
        {
            this.InitializeComponent();
        }

        private void Button_Click(object sender, RoutedEventArgs e)
        {
            buttonResult.Text = "always";
        }

        private void Button_Click_V1(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            buttonResult.Text = obj.V1Property;
        }

        private void Button_Click_V2(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            buttonResult.Text = obj.V2Property;
        }

        private void Button_Click_V3(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            buttonResult.Text = obj.V3Property;
        }

        private void Button_Click_notV3(object sender, RoutedEventArgs e)
        {
            IVersionedProperties obj = sender as IVersionedProperties;
            buttonResult.Text = "notV3here's" + obj.V2Property;
        }
    }
}
