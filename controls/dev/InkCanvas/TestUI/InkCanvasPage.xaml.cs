// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    using System;
    using Microsoft.UI.Xaml;
    using Microsoft.UI.Xaml.Controls;
    using Windows.UI.Core;

    // The InkCanvas inks by default — a bare <InkCanvas/> produces wet ink for
    // mouse, pen, and touch with no app code. The optional InkToolbar in the XAML
    // drives the canvas's InkPresenter (pen / eraser / stroke color and width).
    [TopLevelTestPage(Name = "InkCanvas")]
    public sealed partial class InkCanvasPage : TestPage
    {
        private int _unprocessedPressed;
        private int _unprocessedMoved;
        private int _unprocessedReadErrors;
        private int _unprocessedReleased;
        private int _strokesCollected;
        private int _underlayPointerPressed;

        public InkCanvasPage()
        {
            this.InitializeComponent();

            var presenter = TestInkCanvas.InkPresenter;
            presenter.InputDeviceTypes = CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Pen;
            presenter.StrokesCollected += (s, e) =>
            {
                _strokesCollected += e.Strokes.Count;
                StrokesCollectedCount.Text = _strokesCollected.ToString();
            };
        }

        // Custom erasers (e.g. Snipping Tool) turn processing off and read the raw pointer from UnprocessedInput.
        private void OnListenForUnprocessedInputClick(object sender, RoutedEventArgs e)
        {
            var presenter = TestInkCanvas.InkPresenter;
            presenter.InputProcessingConfiguration.Mode = InkInputProcessingMode.None;
            presenter.UnprocessedInput.PointerPressed += (s, args) => ReadPointer(args, ref _unprocessedPressed);
            presenter.UnprocessedInput.PointerMoved += (s, args) => ReadPointer(args, ref _unprocessedMoved);
            presenter.UnprocessedInput.PointerReleased += (s, args) => ReadPointer(args, ref _unprocessedReleased);
        }

        private void ReadPointer(PointerEventArgs args, ref int counter)
        {
            try
            {
                var point = args.CurrentPoint;
                _ = point.Position;
                _ = point.Properties.IsEraser;
                _ = args.GetIntermediatePoints().Count;

                // The device is taken with the event, so it is still there after the pointer is released.
                if (point.PointerDevice?.PointerDeviceType != Windows.Devices.Input.PointerDeviceType.Mouse)
                {
                    throw new InvalidOperationException("PointerDevice is missing or wrong.");
                }
                counter++;
            }
            catch (Exception)
            {
                _unprocessedReadErrors++;
            }

            UnprocessedInputStatus.Text = $"{_unprocessedPressed},{_unprocessedMoved},{_unprocessedReadErrors},{_unprocessedReleased}";
        }

        private void OnCollapseCanvasHostClick(object sender, RoutedEventArgs e)
        {
            CanvasHost.Visibility = Visibility.Collapsed;
        }

        private void OnUnderlayClick(object sender, RoutedEventArgs e)
        {
            _underlayPointerPressed++;
            UnderlayPointerCount.Text = _underlayPointerPressed.ToString();
        }
    }
}
