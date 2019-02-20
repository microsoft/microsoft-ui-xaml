// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Foundation.Metadata;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

namespace MUXControlsTestApp
{
    public sealed partial class ExtraCommandBarFlyoutPage : TestPage
    {
        public ExtraCommandBarFlyoutPage()
        {
            this.InitializeComponent();

            if (ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Primitives.FlyoutPlacementMode", "BottomEdgeAlignedLeft"))
            {
                TextCommandBarContextFlyout.Placement = FlyoutPlacementMode.BottomEdgeAlignedLeft;
            }
            else
            {
                TextCommandBarContextFlyout.Placement = FlyoutPlacementMode.Top;
            }

            if (ApiInformation.IsEnumNamedValuePresent("Windows.UI.Xaml.Controls.Primitives.FlyoutPlacementMode", "TopEdgeAlignedLeft"))
            {
                TextCommandBarSelectionFlyout.Placement = FlyoutPlacementMode.TopEdgeAlignedLeft;
            }
            else
            {
                TextCommandBarSelectionFlyout.Placement = FlyoutPlacementMode.Top;
            }

            if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "ContextFlyout"))
            {
                TextBox1.ContextFlyout = TextCommandBarContextFlyout;
                RichTextBlock1.ContextFlyout = TextCommandBarContextFlyout;
            }
            
            try
            {
                if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.Controls.TextBox", "SelectionFlyout"))
                {
                    TextBox1.SelectionFlyout = TextCommandBarSelectionFlyout;
                }
            }
            catch (InvalidCastException)
            {
                // RS5 interfaces can change before release, so we need to make sure we don't crash if they do.
            }

            if (ApiInformation.IsPropertyPresent("Windows.UI.Xaml.Controls.RichTextBlock", "SelectionFlyout"))
            {
                RichTextBlock1.SelectionFlyout = TextCommandBarSelectionFlyout;
            }
        }

        private void OnClearClipboardContentsClicked(object sender, object args)
        {
            Clipboard.Clear();
        }

        private void OnCountPopupsClicked(object sender, object args)
        {
            var popups = VisualTreeHelper.GetOpenPopups(Window.Current);
            PopupCountTextBox.Text = popups.Count.ToString();

            var secondaryPopup = popups.Last();
            PopupXPositionTextBox.Text = secondaryPopup.HorizontalOffset.ToString();
            PopupYPositionTextBox.Text = secondaryPopup.VerticalOffset.ToString();
        }
    }
}
