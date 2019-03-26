using System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;


namespace MUXControlsAdhocApp.GridPages
{
    public sealed partial class GridPlaygroundPage : Page
    {
        public GridPlaygroundPage()
        {
            this.InitializeComponent();
        }

        private void InvalidateButton_Click(object sender, RoutedEventArgs e)
        {
            _grid.InvalidateMeasure();
        }
    }
}
