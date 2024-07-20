// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="ListView", Icon="ListView.png")]
    public sealed partial class ListViewPage : TestPage
    {
        public ListViewPage()
        {
            this.InitializeComponent();

            navigateToListView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(ListViewBasePage), false /*use GridView*/); };
            navigateToGroupedListView.Click += delegate { Frame.NavigateWithoutAnimation(typeof(GroupedListViewBasePage), false /*use GridView*/); };
            navigateToNestedListViews.Click += delegate { Frame.NavigateWithoutAnimation(typeof(NestedListViewsPage)); };
        }
    }
}
