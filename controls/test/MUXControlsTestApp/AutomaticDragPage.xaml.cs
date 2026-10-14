// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
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

        public AutomaticDragPage()
        {
            InitializeComponent();
            DragSource.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((sender, args) =>
                LastPointerDevice.Text = args.Pointer.PointerDeviceType.ToString()), true);
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
        }
    }
}
