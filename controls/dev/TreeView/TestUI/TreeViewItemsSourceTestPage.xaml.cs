// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using System.Collections.ObjectModel;

namespace MUXControlsTestApp
{
    public sealed partial class TreeViewItemsSourceTestPage : TestPage
    {
        private readonly TreeViewViewModel viewModel = new();

        public TreeViewItemsSourceTestPage()
        {
            this.InitializeComponent();
        }
    }
}
