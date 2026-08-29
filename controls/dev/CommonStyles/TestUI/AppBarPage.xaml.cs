// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="AppBar")]
    public sealed partial class AppBarPage : TestPage
    {
        public AppBarPage()
        {
            this.InitializeComponent();
        }

        private void CmbLabelPosition_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbLabelPosition == null) return;

            CommandBarLabelPosition labelPosition = (CommandBarLabelPosition)cmbLabelPosition.SelectedIndex;

            abbA1.LabelPosition = 
            abbA2.LabelPosition = 
            abbA3.LabelPosition = 
            abbB1.LabelPosition = 
            abbB2.LabelPosition = 
            abbB3.LabelPosition = labelPosition;
        }

        private void CmbClosedDisplayMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbClosedDisplayMode == null) return;

            AppBarClosedDisplayMode closedDisplayMode = (AppBarClosedDisplayMode)cmbClosedDisplayMode.SelectedIndex;

            abA.ClosedDisplayMode =
            abB.ClosedDisplayMode = closedDisplayMode;
        }

        private void CmbIsOpen_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbIsOpen == null) return;

            bool isOpen = cmbIsOpen.SelectedIndex == 1;

            abA.IsOpen =
            abB.IsOpen = isOpen;
        }
    }
}