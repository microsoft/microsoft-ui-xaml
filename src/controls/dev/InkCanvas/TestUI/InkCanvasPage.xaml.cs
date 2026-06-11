// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Windows.Storage.Streams;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "InkCanvas")]
    public sealed partial class InkCanvasPage : TestPage
    {
        private InMemoryRandomAccessStream _savedStream;

        public InkCanvasPage()
        {
            this.InitializeComponent();

            TestInkCanvas.StrokeCollected += OnStrokeCollected;
            TestInkCanvas.StrokesErased += OnStrokesErased;
        }

        private void OnModeChanged(object sender, SelectionChangedEventArgs e)
        {
            if (TestInkCanvas == null || ModeSelector == null) return;

            switch (ModeSelector.SelectedIndex)
            {
                case 0:
                    TestInkCanvas.Mode = InkCanvasMode.Draw;
                    break;
                case 1:
                    TestInkCanvas.Mode = InkCanvasMode.Erase;
                    break;
                case 2:
                    TestInkCanvas.Mode = InkCanvasMode.Select;
                    break;
            }

            UpdateStatusDisplay();
        }

        private void OnInputTypeChanged(object sender, RoutedEventArgs e)
        {
            if (TestInkCanvas == null) return;

            var inputTypes = InkInputType.None;
            if (PenInput.IsChecked == true) inputTypes |= InkInputType.Pen;
            if (MouseInput.IsChecked == true) inputTypes |= InkInputType.Mouse;
            if (TouchInput.IsChecked == true) inputTypes |= InkInputType.Touch;

            TestInkCanvas.AllowedInputTypes = inputTypes;
            UpdateStatusDisplay();
        }

        private void OnClearClicked(object sender, RoutedEventArgs e)
        {
            TestInkCanvas.ClearStrokes();
            StatusText.Text = "Strokes cleared.";
            UpdateStrokeCount();
        }

        private async void OnSaveClicked(object sender, RoutedEventArgs e)
        {
            try
            {
                _savedStream = new InMemoryRandomAccessStream();
                await TestInkCanvas.SaveAsync(_savedStream.GetOutputStreamAt(0));
                StatusText.Text = $"Saved ({_savedStream.Size} bytes).";
            }
            catch (Exception ex)
            {
                StatusText.Text = $"Save failed: {ex.Message}";
            }
        }

        private async void OnLoadClicked(object sender, RoutedEventArgs e)
        {
            if (_savedStream == null)
            {
                StatusText.Text = "No saved data. Save first.";
                return;
            }

            try
            {
                await TestInkCanvas.LoadAsync(_savedStream.GetInputStreamAt(0));
                StatusText.Text = "Loaded successfully.";
                UpdateStrokeCount();
            }
            catch (Exception ex)
            {
                StatusText.Text = $"Load failed: {ex.Message}";
            }
        }

        private void OnStrokeCollected(InkCanvas sender, InkCanvasStrokeCollectedEventArgs args)
        {
            UpdateStrokeCount();
            StatusText.Text = "Stroke collected.";
        }

        private void OnStrokesErased(InkCanvas sender, InkCanvasStrokesErasedEventArgs args)
        {
            UpdateStrokeCount();
            StatusText.Text = "Strokes erased.";
        }

        private void UpdateStrokeCount()
        {
            var container = TestInkCanvas.StrokeContainer;
            if (container != null)
            {
                var strokes = container.GetStrokes();
                StrokeCountText.Text = $"Strokes: {strokes.Count}";
            }
        }

        private void UpdateStatusDisplay()
        {
            ModeText.Text = $"Mode: {TestInkCanvas.Mode}";

            var types = new List<string>();
            var inputTypes = TestInkCanvas.AllowedInputTypes;
            if ((inputTypes & InkInputType.Pen) != 0) types.Add("Pen");
            if ((inputTypes & InkInputType.Mouse) != 0) types.Add("Mouse");
            if ((inputTypes & InkInputType.Touch) != 0) types.Add("Touch");
            InputTypesText.Text = $"Input: {(types.Count > 0 ? string.Join(", ", types) : "None")}";
        }
    }
}
