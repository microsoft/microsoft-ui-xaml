// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "TextControls")]
    public sealed partial class TextControlsPage : TestPage
    {
        private bool isRootGridDefaultSize = true;

        public TextControlsPage()
        {
            this.InitializeComponent();
            resizeWindowButton.Click += ResizeRootGrid;
        }

        private void ResizeRootGrid(object sender, RoutedEventArgs e)
        {
            if (isRootGridDefaultSize)
            {
                this.FindVisualParentByType<TestFrame>().SetRootGridSizeToCustomSize(1, 1);
            }
            else
            {
                // Reset to default size
                this.FindVisualParentByType<TestFrame>().SetRootGridSizeToCustomSize(-1, -1);
            }
            isRootGridDefaultSize = !isRootGridDefaultSize;
        }
    }
}
