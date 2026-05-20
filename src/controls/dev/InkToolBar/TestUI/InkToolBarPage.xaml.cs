// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "InkToolBar")]
    public sealed partial class InkToolBarPage : TestPage
    {
        public InkToolBarPage()
        {
            this.InitializeComponent();
        }

        private void OnInitialControlsChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolBar == null || InitialControlsSelector == null) return;

            switch (InitialControlsSelector.SelectedIndex)
            {
                case 0:
                    TestInkToolBar.InitialControls = InkToolBarInitialControls.All;
                    break;
                case 1:
                    TestInkToolBar.InitialControls = InkToolBarInitialControls.None;
                    break;
                case 2:
                    TestInkToolBar.InitialControls = InkToolBarInitialControls.PensOnly;
                    break;
                case 3:
                    TestInkToolBar.InitialControls = InkToolBarInitialControls.AllExceptPens;
                    break;
            }

            StatusText.Text = $"InitialControls set to {TestInkToolBar.InitialControls}.";
        }

        private void OnOrientationChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolBar == null || OrientationSelector == null) return;

            TestInkToolBar.Orientation = OrientationSelector.SelectedIndex == 0
                ? Orientation.Horizontal
                : Orientation.Vertical;

            OrientationText.Text = $"Orientation: {TestInkToolBar.Orientation}";
            StatusText.Text = $"Orientation set to {TestInkToolBar.Orientation}.";
        }

        private void OnFlyoutPlacementChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolBar == null || FlyoutPlacementSelector == null) return;

            switch (FlyoutPlacementSelector.SelectedIndex)
            {
                case 0:
                    TestInkToolBar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Auto;
                    break;
                case 1:
                    TestInkToolBar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Top;
                    break;
                case 2:
                    TestInkToolBar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Bottom;
                    break;
                case 3:
                    TestInkToolBar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Left;
                    break;
                case 4:
                    TestInkToolBar.ButtonFlyoutPlacement = InkToolBarButtonFlyoutPlacement.Right;
                    break;
            }

            FlyoutPlacementText.Text = $"Flyout: {TestInkToolBar.ButtonFlyoutPlacement}";
            StatusText.Text = $"Flyout placement set to {TestInkToolBar.ButtonFlyoutPlacement}.";
        }

        private void OnRulerCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (TestInkToolBar == null) return;
            TestInkToolBar.IsRulerButtonChecked = RulerCheckBox.IsChecked == true;
            StatusText.Text = $"Ruler: {TestInkToolBar.IsRulerButtonChecked}.";
        }

        private void OnStencilCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (TestInkToolBar == null) return;
            TestInkToolBar.IsStencilButtonChecked = StencilCheckBox.IsChecked == true;
            StatusText.Text = $"Stencil: {TestInkToolBar.IsStencilButtonChecked}.";
        }

        private void OnActiveToolChanged(InkToolBar sender, object args)
        {
            var activeTool = TestInkToolBar.ActiveTool;
            ActiveToolText.Text = activeTool != null
                ? $"Active Tool: {activeTool.ToolKind}"
                : "Active Tool: -";
            StatusText.Text = "Active tool changed.";
        }

        private void OnInkDrawingAttributesChanged(InkToolBar sender, object args)
        {
            StatusText.Text = "Ink drawing attributes changed.";
        }

        private void OnEraseAllClicked(InkToolBar sender, object args)
        {
            StatusText.Text = "Erase all clicked.";
        }
    }
}
