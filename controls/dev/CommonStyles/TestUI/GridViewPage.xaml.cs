// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="GridView", Icon="GridView.png")]
    public sealed partial class GridViewPage : TestPage
    {
        public GridViewPage()
        {
            this.InitializeComponent();

            navigateToGridView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ListViewBasePage), true /*use GridView*/); };
            navigateToGroupedGridView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(GroupedListViewBasePage), true /*use GridView*/); };
            navigateToNestedGridViews.Click += delegate { Frame.NavigateWithoutAnimation(typeof(NestedGridViewsPage)); };
        }
    }
}
