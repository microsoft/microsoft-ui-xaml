// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.Foundation;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;

namespace MUXControlsTestApp
{

    [TopLevelTestPage(Name = "MenuFlyout", Icon = "MenuFlyout.png")]
    public sealed partial class MenuFlyoutPage : TestPage
    {
        private MenuFlyout sharedFlyout;
        public MenuFlyoutPage()
        {
            this.InitializeComponent();

            sharedFlyout = (MenuFlyout)Resources["SampleContextMenu"];
        }

        private void TestMenuFlyoutItemClick(object sender, object e)
        {
            TestMenuFlyoutItemHeightTextBlock.Text = $"{TestMenuFlyoutItem.ActualHeight}";
            TestMenuFlyoutItemWidthTextBlock.Text = $"{TestMenuFlyoutItem.ActualWidth}";
        }

        private void Grid_ContextRequested(UIElement sender, ContextRequestedEventArgs args)
        {
            var requestedElement = sender as FrameworkElement;

            if (args.TryGetPosition(requestedElement, out Point point))
            {
                sharedFlyout.ShowAt(requestedElement, point);
            }
            else
            {
                sharedFlyout.ShowAt(requestedElement);
            }
        }
    }
}
