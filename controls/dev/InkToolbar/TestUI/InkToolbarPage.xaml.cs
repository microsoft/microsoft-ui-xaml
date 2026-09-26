// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    // Harness page for the InkToolbar interaction tests. The selectors and toggles let the
    // tests exercise InitialControls, Orientation, ButtonFlyoutPlacement, ruler and stencil
    // state, and observe the active tool without needing any product code changes.
    [TopLevelTestPage(Name = "InkToolbar Tests")]
    public sealed partial class InkToolbarPage : TestPage
    {
        public InkToolbarPage()
        {
            this.InitializeComponent();
            TestInkToolbar.ActiveToolChanged += OnActiveToolChanged;
            Loaded += (s, e) => UpdateActiveToolText();
        }

        private void InitialControlsSelector_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolbar != null &&
                TryGetSelectedContent(InitialControlsSelector, out string value) &&
                Enum.TryParse(value, out InkToolbarInitialControls parsed))
            {
                TestInkToolbar.InitialControls = parsed;
                StatusText.Text = "InitialControls=" + value;
            }
        }

        private void OrientationSelector_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolbar != null &&
                TryGetSelectedContent(OrientationSelector, out string value) &&
                Enum.TryParse(value, out Orientation parsed))
            {
                TestInkToolbar.Orientation = parsed;
                OrientationText.Text = value;
            }
        }

        private void FlyoutPlacementSelector_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkToolbar != null &&
                TryGetSelectedContent(FlyoutPlacementSelector, out string value) &&
                Enum.TryParse(value, out InkToolbarButtonFlyoutPlacement parsed))
            {
                TestInkToolbar.ButtonFlyoutPlacement = parsed;
                FlyoutPlacementText.Text = value;
            }
        }

        private void RulerCheckBox_Toggled(object sender, RoutedEventArgs e)
        {
            if (TestInkToolbar == null)
            {
                return;
            }

            TestInkToolbar.IsRulerButtonChecked = RulerCheckBox.IsChecked == true;
            StatusText.Text = "Ruler=" + TestInkToolbar.IsRulerButtonChecked;
        }

        private void StencilCheckBox_Toggled(object sender, RoutedEventArgs e)
        {
            if (TestInkToolbar == null)
            {
                return;
            }

            TestInkToolbar.IsStencilButtonChecked = StencilCheckBox.IsChecked == true;
            StatusText.Text = "Stencil=" + TestInkToolbar.IsStencilButtonChecked;
        }

        private void OnActiveToolChanged(InkToolbar sender, object args)
        {
            UpdateActiveToolText();
        }

        private void UpdateActiveToolText()
        {
            var active = TestInkToolbar?.ActiveTool;
            ActiveToolText.Text = active != null ? active.GetType().Name : "None";
        }

        private static bool TryGetSelectedContent(ComboBox comboBox, out string content)
        {
            if (comboBox?.SelectedItem is ComboBoxItem item && item.Content is string s)
            {
                content = s;
                return true;
            }

            content = null;
            return false;
        }
    }
}
