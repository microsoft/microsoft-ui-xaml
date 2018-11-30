// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class ParallaxViewPage : TestPage
    {
        public ParallaxViewPage()
        {
            this.InitializeComponent();

            navigateToSimpleRectangle.Click += delegate { Frame.NavigateWithoutAnimation(typeof(SimpleRectanglePage), 0); };
            navigateToStackPanel.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ParallaxViewStackPanelPage), 0); };
            navigateToListViewBackground.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ListViewBackgroundPage), 0); };
            navigateToVSP.Click += delegate { Frame.NavigateWithoutAnimation(typeof(VirtualizingStackPanelPage), 0); };
            navigateToListViewItem.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ListViewItemPage), 0); };
            navigateToListViewHeader.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ListViewHeaderPage), 0); };
            navigateToText.Click += delegate { Frame.NavigateWithoutAnimation(typeof(TextPage), 0); };
            navigateToDynamic.Click += delegate { Frame.NavigateWithoutAnimation(typeof(DynamicPage), 0); };
        }
    }
}
