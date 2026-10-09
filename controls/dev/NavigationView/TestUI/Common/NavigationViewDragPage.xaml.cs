// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
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

        public NavigationViewDragPage()
        {
            InitializeComponent();
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
                item.DragStarting += (sender, args) =>
                {
                    DragCount.Text = (++dragCount).ToString();
                    DragSource.Text = ((FrameworkElement)sender).Name;
                    args.Data.SetText(DragSource.Text);
                    args.AllowedOperations = DataPackageOperation.Copy;
                    args.Cancel = CancelDrag.IsChecked == true;
                };
                item.DropCompleted += (sender, args) => CompletionResult.Text = args.DropResult.ToString();
            }
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
