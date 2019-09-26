// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    /// <summary>
    /// Test page used for clearing existing SwipeControls
    /// </summary>
    public sealed partial class SwipeControlClearPage : TestPage
    {
        public SwipeControlClearPage()
        {
            this.InitializeComponent();
            SwipeItemsChildSum.Text = (DefaultSwipeItemsHorizontal.Count + DefaultSwipeItemsVertical.Count).ToString();
        }

        public void AddSwipeItemsButton_Click(object sender, RoutedEventArgs e)
        {
            DefaultSwipeItemsHorizontal.Clear();
            DefaultSwipeItemsVertical.Clear();

            DefaultSwipeItemsHorizontal.Mode = Microsoft.UI.Xaml.Controls.SwipeMode.Reveal;
            DefaultSwipeItemsHorizontal.Add(DefaultSwipeItemHorizontal);

            // Throws exception:
            //"The parameter is incorrect. This SwipeControl is horizontal and can not have vertical items."
            //DefaultSwipeItemsVertical.Mode = Microsoft.UI.Xaml.Controls.SwipeMode.Reveal;
            //DefaultSwipeItemsVertical.Add(DefaultSwipeItemVertical);

            SwipeItemsChildSum.Text = (DefaultSwipeItemsHorizontal.Count + DefaultSwipeItemsVertical.Count).ToString();
        }
        public void ClearSwipeItemsButton_Click(object sender, RoutedEventArgs e)
        {
            DefaultSwipeItemsHorizontal.Clear();
            DefaultSwipeItemsVertical.Clear();
            SwipeItemsChildSum.Text = (DefaultSwipeItemsHorizontal.Count + DefaultSwipeItemsVertical.Count).ToString();
        }

    }
}
