// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

namespace MUXControlsTestApp
{
    using System;
    using System.Threading;
    using Microsoft.UI.Xaml;
    using Microsoft.UI.Xaml.Controls;
    using Windows.UI.Core;
    using Windows.UI.Input.Inking.Core;

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

        private Microsoft.UI.Xaml.Controls.CoreWetStrokeUpdateSource _wetStrokes;
        private volatile bool _cancelWetStrokes;
        private int _wetStarting;
        private int _wetContinuing;
        private int _wetStopping;
        private int _wetCompleted;
        private int _wetCanceled;

        public InkCanvasPage()
        {
            this.InitializeComponent();

            // Mouse too, so interaction tests can ink with injected mouse input.
            TestInkCanvas.InkPresenter.InputDeviceTypes = CoreInputDeviceTypes.Mouse | CoreInputDeviceTypes.Pen;
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

        private void OnListenForWetStrokesClick(object sender, RoutedEventArgs e) => ListenForWetStrokes(cancel: false);

        // Cancels every stroke from WetStrokeContinuing, so the OS must report WetStrokeCanceled.
        private void OnCancelWetStrokesClick(object sender, RoutedEventArgs e) => ListenForWetStrokes(cancel: true);

        private void ListenForWetStrokes(bool cancel)
        {
            _cancelWetStrokes = cancel;
            if (_wetStrokes != null)
            {
                return;
            }

            // The events are raised on the ink thread; count there and update the text on the UI thread.
            _wetStrokes = Microsoft.UI.Xaml.Controls.CoreWetStrokeUpdateSource.Create(TestInkCanvas.InkPresenter);
            _wetStrokes.WetStrokeStarting += (s, args) => Count(ref _wetStarting);
            _wetStrokes.WetStrokeContinuing += (s, args) =>
            {
                if (_cancelWetStrokes)
                {
                    args.Disposition = CoreWetStrokeDisposition.Canceled;
                }
                Count(ref _wetContinuing);
            };
            _wetStrokes.WetStrokeStopping += (s, args) => Count(ref _wetStopping);
            _wetStrokes.WetStrokeCompleted += (s, args) => Count(ref _wetCompleted);
            _wetStrokes.WetStrokeCanceled += (s, args) => Count(ref _wetCanceled);
        }

        private void Count(ref int counter)
        {
            Interlocked.Increment(ref counter);
            DispatcherQueue.TryEnqueue(() =>
                WetStrokeStatus.Text = $"{_wetStarting},{_wetContinuing},{_wetStopping},{_wetCompleted},{_wetCanceled}");
        }
    }
}