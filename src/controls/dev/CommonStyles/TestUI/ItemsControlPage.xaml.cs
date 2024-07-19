// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="ItemsControl", Icon="ListView.png")]
    public sealed partial class ItemsControlPage : TestPage
    {
        public ItemsControlPage()
        {
            this.InitializeComponent();

            navigateToFlatItemsControl.Click += delegate { Frame.NavigateWithoutAnimation(typeof(FlatItemsControlPage)); };
            navigateToGroupedItemsControl.Click += delegate { Frame.NavigateWithoutAnimation(typeof(GroupedItemsControlPage)); };
            navigateToNestedItemsControls.Click += delegate { Frame.NavigateWithoutAnimation(typeof(NestedItemsControlsPage)); };
        }
    }
}
