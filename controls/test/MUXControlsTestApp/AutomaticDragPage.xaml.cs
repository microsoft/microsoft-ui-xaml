// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Linq;
using Microsoft.UI.Input;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Input;
using Windows.ApplicationModel.DataTransfer;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name = "Automatic Drag Tests")]
    public sealed partial class AutomaticDragPage : TestPage
    {
        private int releaseCount;
        private int hoverMoveCount;
        private int dragCount;
        private int dropCount;
        private int copyCompletionCount;
        private int childTouchReleaseCount;
        private int childTouchPressCount;
        private int mouseCaptureLostCount;

        public AutomaticDragPage()
        {
            InitializeComponent();
            DragSource.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((sender, args) =>
            {
                LastPointerDevice.Text = args.Pointer.PointerDeviceType.ToString();
                if (args.Pointer.PointerDeviceType == PointerDeviceType.Mouse)
                {
                    MousePointerId.Text = args.Pointer.PointerId.ToString();
                }
            }), true);
            DragSource.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler((sender, args) =>
            {
                if (args.Pointer.PointerDeviceType == PointerDeviceType.Mouse)
                {
                    MouseCaptureLostCount.Text = (++mouseCaptureLostCount).ToString();
                }
            }), true);
            TouchChild.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((sender, args) =>
            {
                if (args.Pointer.PointerDeviceType == PointerDeviceType.Touch)
                {
                    HandleChildTouch(sender, args);
                    ChildTouchPressCount.Text = (++childTouchPressCount).ToString();
                }
            }), true);
            TouchChild.AddHandler(UIElement.PointerCaptureLostEvent, new PointerEventHandler(HandleChildTouch), true);
            TouchChild.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler((sender, args) =>
            {
                if (args.Pointer.PointerDeviceType == PointerDeviceType.Touch)
                {
                    HandleChildTouch(sender, args);
                    ChildTouchReleaseCount.Text = (++childTouchReleaseCount).ToString();
                }
            }), true);
            DragSource.AddHandler(UIElement.PointerReleasedEvent, new PointerEventHandler((sender, args) =>
            {
                ReleaseCount.Text = (++releaseCount).ToString();
                if (HandleRelease.IsChecked == true)
                {
                    args.Handled = true;
                    DragSource.ReleasePointerCapture(args.Pointer);
                }
            }), true);
            DragSource.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler((sender, args) =>
            {
                var point = args.GetCurrentPoint(DragSource);
                if (args.Pointer.PointerDeviceType.ToString() == LastPointerDevice.Text &&
                    !point.IsInContact && !point.Properties.IsLeftButtonPressed)
                {
                    HoverMoveCount.Text = (++hoverMoveCount).ToString();
                }
            }), true);
            DragSource.DragStarting += (sender, args) =>
            {
                DragCount.Text = (++dragCount).ToString();
                args.Data.SetText("AutomaticDragSource");
                args.AllowedOperations = DataPackageOperation.Copy;
                args.Cancel = CancelDrag.IsChecked == true;
            };
            DragSource.DropCompleted += (sender, args) =>
            {
                CompletionResult.Text = args.DropResult.ToString();
                if (args.DropResult == DataPackageOperation.Copy)
                {
                    CopyCompletionCount.Text = (++copyCompletionCount).ToString();
                }
            };
        }

        private void HandleChildTouch(object sender, PointerRoutedEventArgs args)
        {
            if (args.Pointer.PointerDeviceType == PointerDeviceType.Touch)
            {
                ChildTouchPointerId.Text = args.Pointer.PointerId.ToString();
                args.Handled = true;
            }
        }

        private void DropTarget_DragOver(object sender, DragEventArgs args)
        {
            args.AcceptedOperation = DataPackageOperation.Copy;
            args.Handled = true;
        }

        private async void DropTarget_Drop(object sender, DragEventArgs args)
        {
            var deferral = args.GetDeferral();
            try
            {
                DroppedText.Text = await args.DataView.GetTextAsync();
                DropCount.Text = (++dropCount).ToString();
                args.AcceptedOperation = DataPackageOperation.Copy;
            }
            finally
            {
                deferral.Complete();
            }
        }

        private void Snapshot_Click(object sender, RoutedEventArgs args)
        {
            CaptureCount.Text = (DragSource.PointerCaptures?.Count ?? 0).ToString();
            MouseCaptureCount.Text = (DragSource.PointerCaptures?.Count(
                pointer => pointer.PointerDeviceType == PointerDeviceType.Mouse) ?? 0).ToString();
        }
    }
}
