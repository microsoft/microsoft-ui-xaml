// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Windows.ApplicationModel.DataTransfer;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewDragPage : TestPage
    {
        private int dragCount;
        private int invocationCount;
        private int retemplateCount;
        private int hoverMoveCount;
        private DragOperationDeferral dragDeferral;

        public NavigationViewDragPage()
        {
            InitializeComponent();
            Unloaded += (sender, args) => CompletePendingDragDeferral();
            DragItem.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((sender, args) =>
            {
                if (RetemplateOnPress.IsChecked == true)
                {
                    var template = DragItem.Template;
                    DragItem.Template = new ControlTemplate();
                    DragItem.ApplyTemplate();
                    DragItem.Template = template;
                    DragItem.ApplyTemplate();
                    RetemplateCount.Text = (++retemplateCount).ToString();
                }
            }), true);
            foreach (var item in new[] { DragItem, NestedDragItem })
            {
                item.AddHandler(UIElement.PointerPressedEvent, new PointerEventHandler((sender, args) =>
                {
                    LastPointerDevice.Text = args.Pointer.PointerDeviceType.ToString();
                    var id = args.Pointer.PointerId;
                    LastPressPointerId.Text = id.ToString();
                    PressCaptureOwner.Text = item.PointerCaptures?.Any(pointer => pointer.PointerId == id) == true
                        ? "Item"
                        : Descendants(item).OfType<UIElement>().Any(element =>
                            element.PointerCaptures?.Any(pointer => pointer.PointerId == id) == true)
                            ? "Presenter" : "None";
                }), true);
                item.AddHandler(UIElement.PointerMovedEvent, new PointerEventHandler((sender, args) =>
                {
                    var point = args.GetCurrentPoint(item);
                    // Count both cross-device legacy guards and native same-pen hover cases.
                    if (!point.IsInContact && !point.Properties.IsLeftButtonPressed)
                    {
                        HoverMoveCount.Text = (++hoverMoveCount).ToString();
                        LastHoverPointerId.Text = args.Pointer.PointerId.ToString();
                        LastHoverDevice.Text = args.Pointer.PointerDeviceType.ToString();
                    }
                }), true);
                item.DragStarting += (sender, args) =>
                {
                    DragStartPointerOver.Text = Descendants((DependencyObject)sender)
                        .OfType<FrameworkElement>()
                        .SelectMany(element => VisualStateManager.GetVisualStateGroups(element))
                        .Any(group => group.CurrentState != null && group.CurrentState.Name.Contains("PointerOver"))
                        .ToString();
                    DragCount.Text = (++dragCount).ToString();
                    DragSource.Text = ((FrameworkElement)sender).Name;
                    args.Data.SetText(DragSource.Text);
                    args.AllowedOperations = DataPackageOperation.Copy;
                    args.Cancel = CancelDrag.IsChecked == true;
                    if (DeferDragStart.IsChecked == true)
                    {
                        if (dragDeferral != null)
                        {
                            throw new InvalidOperationException("Complete the pending drag deferral before starting another drag.");
                        }
                        dragDeferral = args.GetDeferral();
                        CompleteDragDeferral.IsEnabled = true;
                    }
                };
                item.DropCompleted += (sender, args) => CompletionResult.Text = args.DropResult.ToString();
            }
        }

        private void CompleteDragDeferral_Click(object sender, RoutedEventArgs args)
        {
            CompletePendingDragDeferral();
        }

        private void CompletePendingDragDeferral()
        {
            var deferral = dragDeferral;
            dragDeferral = null;
            CompleteDragDeferral.IsEnabled = false;
            deferral?.Complete();
        }

        private void TopMode_Click(object sender, RoutedEventArgs args)
        {
            NavView.PaneDisplayMode = TopMode.IsChecked == true
                ? NavigationViewPaneDisplayMode.Top : NavigationViewPaneDisplayMode.Left;
        }

        private void Draggable_Click(object sender, RoutedEventArgs args)
        {
            DragItem.CanDrag = NestedDragItem.CanDrag = Draggable.IsChecked == true;
        }

        private void DisableDragItem_Click(object sender, RoutedEventArgs args) => DragItem.IsEnabled = false;

        private void EnableDragItem_Click(object sender, RoutedEventArgs args) => DragItem.IsEnabled = true;

        private void FocusDragItem_Click(object sender, RoutedEventArgs args) => DragItem.Focus(FocusState.Keyboard);

        private void DetachDragItem_Click(object sender, RoutedEventArgs args) => NavView.MenuItems.Remove(DragItem);

        private void AttachDragItem_Click(object sender, RoutedEventArgs args)
        {
            if (!NavView.MenuItems.Contains(DragItem))
            {
                NavView.MenuItems.Insert(0, DragItem);
            }
        }

        private void NavView_ItemInvoked(NavigationView sender, NavigationViewItemInvokedEventArgs args)
        {
            InvocationCount.Text = (++invocationCount).ToString();
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
                args.AcceptedOperation = DataPackageOperation.Copy;
            }
            finally
            {
                deferral.Complete();
            }
        }

        private void Snapshot_Click(object sender, RoutedEventArgs args)
        {
            var elements = Descendants(NavView).OfType<FrameworkElement>().ToList();
            CaptureCount.Text = elements.Sum(element => element.PointerCaptures?.Count ?? 0).ToString();
            var pressedStates = elements
                .SelectMany(element => VisualStateManager.GetVisualStateGroups(element))
                .Where(group => group.CurrentState != null && group.CurrentState.Name.Contains("Pressed"))
                .Select(group => group.CurrentState.Name);
            PressedStates.Text = string.Join(",", pressedStates.DefaultIfEmpty("None"));
            ItemPointerOver.Text = Descendants(DragItem).Prepend(DragItem).OfType<FrameworkElement>()
                .SelectMany(element => VisualStateManager.GetVisualStateGroups(element))
                .Any(group => group.CurrentState != null && group.CurrentState.Name.Contains("PointerOver")).ToString();
            ItemWidth.Text = DragItem.ActualWidth.ToString(CultureInfo.InvariantCulture);
            ItemRasterizationScale.Text = XamlRoot.RasterizationScale.ToString(CultureInfo.InvariantCulture);
        }

        private static IEnumerable<DependencyObject> Descendants(DependencyObject root)
        {
            for (int i = 0; i < VisualTreeHelper.GetChildrenCount(root); ++i)
            {
                var child = VisualTreeHelper.GetChild(root, i);
                yield return child;
                foreach (var descendant in Descendants(child))
                {
                    yield return descendant;
                }
            }
        }
    }
}
