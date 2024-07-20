// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using MUXControlsTestApp.Utilities;

namespace MUXControlsTestApp
{
    public sealed partial class CommandBarSummaryPage : TestPage
    {
        public CommandBarSummaryPage()
        {
            this.InitializeComponent();
        }

        private void CB_Opened(object sender, object e)
        {
            cmbIsOpen.SelectedIndex = 1;
        }

        private void CB_Closed(object sender, object e)
        {
            cmbIsOpen.SelectedIndex = 0;
        }

        private void CmbLabelLength_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbLabelLength == null || tb == null) return;

            switch (cmbLabelLength.SelectedIndex)
            {
                case 0:
                    tb.Label = "Dog";
                    break;
                case 1:
                    tb.Label = "Golden Retriever";
                    break;
                default:
                    tb.Label = "Distinguished & Unique Old English Sheepdog";
                    break;
            }
        }

        private void CmbDefaultLabelPosition_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbDefaultLabelPosition == null || cb == null) return;

            cb.DefaultLabelPosition = (CommandBarDefaultLabelPosition)cmbDefaultLabelPosition.SelectedIndex;
        }

        private void CmbClosedDisplayMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbClosedDisplayMode == null || cb == null) return;

            cb.ClosedDisplayMode = (AppBarClosedDisplayMode)cmbClosedDisplayMode.SelectedIndex;
        }

        private void CmbFontSize_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbFontSize == null) return;

            double fontSize = cmbFontSize.SelectedIndex == 0 ? 14 : 40;

            if (tb != null)
            {
                tb.FontSize = fontSize;
            }
            if (sb != null)
            {
                sb.FontSize = fontSize;
            }
            if (tsb != null)
            {
                tsb.FontSize = fontSize;
            }
        }

        private void CmbIsOpen_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbIsOpen == null || cb == null) return;

            cb.IsOpen = cmbIsOpen.SelectedIndex == 1;
        }

        private void CmbIsDynamicOverflowEnabled_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbIsDynamicOverflowEnabled == null || cb == null) return;

            cb.IsDynamicOverflowEnabled = cmbIsDynamicOverflowEnabled.SelectedIndex == 1;
        }

        private void CmbHasSecondaryCommand_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbHasSecondaryCommand == null || cb == null) return;

            if (cmbHasSecondaryCommand.SelectedIndex == 1 && cb.SecondaryCommands.Count == 0)
            {
                cb.SecondaryCommands.Add(sc);
            }
            else if (cmbHasSecondaryCommand.SelectedIndex == 0 && cb.SecondaryCommands.Count == 1)
            {
                cb.SecondaryCommands.Clear();
            }
        }

        private void CmbHasSplitButton_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbHasSplitButton == null || ecsb == null) return;

            if (cmbHasSplitButton.SelectedIndex == 1 && !ecsb.IsLoaded)
            {
                cb.PrimaryCommands.Add(ecsb);
            }
            else if (cmbHasSplitButton.SelectedIndex == 0 && ecsb.IsLoaded)
            {
                cb.PrimaryCommands.Remove(ecsb);
            }
        }

        private void CmbHasToggleSplitButton_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (cmbHasToggleSplitButton == null || ectsb == null) return;

            if (cmbHasToggleSplitButton.SelectedIndex == 1 && !ectsb.IsLoaded)
            {
                cb.PrimaryCommands.Add(ectsb);
            }
            else if (cmbHasToggleSplitButton.SelectedIndex == 0 && ectsb.IsLoaded)
            {
                cb.PrimaryCommands.Remove(ectsb);
            }
        }
    }
}