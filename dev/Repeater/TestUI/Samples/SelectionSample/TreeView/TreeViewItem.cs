// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System.Collections.Generic;
using System.Diagnostics;
using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using IndexPath = Microsoft.UI.Xaml.Controls.IndexPath;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;

namespace MUXControlsTestApp.Samples.Selection
{
    public class TreeViewItem : ContentControl
    {
        public TreeViewItem()
        {
            IsTabStop = true;
            UseSystemFocusVisuals = true;
            Margin = new Thickness(5);
        }

        public SelectionModel SelectionModel
        {
            get { return (SelectionModel)GetValue(SelectionModelProperty); }
            set { SetValue(SelectionModelProperty, value); }
        }

        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.Register("SelectionModel", typeof(SelectionModel), typeof(TreeViewItem), new PropertyMetadata(null, new PropertyChangedCallback(OnPropertyChanged)));

        public bool? IsSelected
        {
            get { return (bool?)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool?), typeof(TreeViewItem), new PropertyMetadata(false));

        public void Select(bool select)
        {
            if (SelectionModel != null)
            {
                var indexPath = GetIndexPath();
                if(select)
                {
                    SelectionModel.SelectAt(indexPath);
                }
                else
                {
                    SelectionModel.DeselectAt(indexPath);
                }
            }
        }

        public int RepeatedIndex
        {
            get { return (int)GetValue(RepeatedIndexProperty); }
            set { SetValue(RepeatedIndexProperty, value); }
        }

        public static readonly DependencyProperty RepeatedIndexProperty =
            DependencyProperty.Register("RepeatedIndex", typeof(int), typeof(TreeViewItem), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            if (args.Property == RepeatedIndexProperty)
            {
                var item = obj as TreeViewItem;
                var indexPath = item.GetIndexPath();
                if (item.SelectionModel != null)
                {
                    item.IsSelected = IsRealized(indexPath) ? item.SelectionModel.IsSelectedAt(indexPath) : false;
                }

                AutomationProperties.SetLevel(item, indexPath.GetSize());
            }
            else
            if (args.Property == SelectionModelProperty)
            {
                if (args.OldValue != null)
                {
                    (args.OldValue as SelectionModel).PropertyChanged -= (obj as TreeViewItem).OnselectionModelChanged;
                }

                if (args.NewValue != null)
                {
                    (args.NewValue as SelectionModel).PropertyChanged += (obj as TreeViewItem).OnselectionModelChanged;
                }
            }
        }

        protected override void OnKeyUp(KeyRoutedEventArgs e)
        {
            var indexPath = GetIndexPath();
            Debug.WriteLine("OnKeyUp:" + indexPath.ToString());

            if (SelectionModel != null)
            {
                if (e.Key == VirtualKey.Escape)
                {
                    SelectionModel.ClearSelection();
                }
                else if (e.Key == VirtualKey.Space)
                {
                    SelectionModel.SelectAt(indexPath);
                }
                else if (!SelectionModel.SingleSelect)
                {
                    var isShiftPressed = Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Shift).HasFlag(Windows.UI.Core.CoreVirtualKeyStates.Down);
                    var isCtrlPressed = Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Control).HasFlag(Windows.UI.Core.CoreVirtualKeyStates.Down);
                    if (e.Key == VirtualKey.A && isCtrlPressed)
                    {
                        SelectionModel.SelectAll();
                    }
                    else if (isCtrlPressed && e.Key == VirtualKey.Space)
                    {
                        if(SelectionModel.IsSelectedAt(indexPath).Value)
                        {
                            SelectionModel.DeselectAt(indexPath);
                        }
                        else
                        {
                            SelectionModel.SelectAt(indexPath);
                        }
                    }
                    else if (isShiftPressed)
                    {
                        SelectionModel.SelectRangeFromAnchorTo(GetIndexPath());
                    }
                }
            }

            base.OnKeyUp(e);
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            if (!e.Handled)
            {
                var indexPath = GetIndexPath();
                Debug.WriteLine("OnPointerPressed:" + indexPath.ToString());

                if (SelectionModel != null)
                {
                    if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Shift) && !SelectionModel.SingleSelect)
                    {
                        if(e.KeyModifiers.HasFlag(VirtualKeyModifiers.Control))
                        {
                            SelectionModel.DeselectRangeFromAnchorTo(GetIndexPath());
                        }
                        else
                        {
                            SelectionModel.SelectRangeFromAnchorTo(GetIndexPath());
                        }
                    }
                    else if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Control))
                    {
                        var path = GetIndexPath();
                        if(SelectionModel.IsSelectedAt(path).Value)
                        {
                            SelectionModel.SelectAt(path);
                        }
                        else
                        {
                            SelectionModel.SelectAt(path);
                        }
                    }
                    else
                    {
                        SelectionModel.SelectAt(GetIndexPath());
                        this.Focus(FocusState.Keyboard);
                    }
                }

                e.Handled = true;
                base.OnPointerPressed(e);
            }
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new TreeViewItemAutomationPeer(this);
        }

        private IndexPath GetIndexPath()
        {
            var child = this as FrameworkElement;
            var parent = child.Parent as FrameworkElement;
            List<int> path = new List<int>();

            // TOOD: Hack to know when to stop
            while (!(parent is ItemsRepeater) || (parent as ItemsRepeater).Name != "rootRepeater")
            {
                if (parent is ItemsRepeater)
                {
                    path.Insert(0, (parent as ItemsRepeater).GetElementIndex(child));
                }

                child = parent;
                parent = parent.Parent as FrameworkElement;
            }

            path.Insert(0, (parent as ItemsRepeater).GetElementIndex(child));

            return IndexPath.CreateFromIndices(path);
        }

        private void OnselectionModelChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "SelectedIndices")
            {
                bool? oldValue = IsSelected;
                var indexPath = GetIndexPath();
                bool? newValue = IsRealized(indexPath) ? SelectionModel.IsSelectedAt(indexPath) : false;

                if (oldValue != newValue)
                {
                    IsSelected = newValue;
                    bool oldValueAsBool = oldValue.HasValue && oldValue.Value;
                    bool newValueAsBool = newValue.HasValue && newValue.Value;
                    if (oldValueAsBool != newValueAsBool)
                    {
                        // AutomationEvents.PropertyChanged is used as a value that means dont raise anything 
                        AutomationEvents eventToRaise =
                            oldValueAsBool ?
                                (SelectionModel.SingleSelect ? AutomationEvents.PropertyChanged : AutomationEvents.SelectionItemPatternOnElementRemovedFromSelection) :
                                (SelectionModel.SingleSelect ? AutomationEvents.SelectionItemPatternOnElementSelected : AutomationEvents.SelectionItemPatternOnElementAddedToSelection);

                        if (eventToRaise != AutomationEvents.PropertyChanged && AutomationPeer.ListenerExists(eventToRaise))
                        {
                            var peer = FrameworkElementAutomationPeer.CreatePeerForElement(this);
                            peer.RaiseAutomationEvent(eventToRaise);
                        }
                    }
                }
            }
        }

        private static bool IsRealized(IndexPath indexPath)
        {
            bool isRealized = true;
            for (int i = 0; i < indexPath.GetSize(); i++)
            {
                if (indexPath.GetAt(i) < 0)
                {
                    isRealized = false;
                    break;
                }
            }

            return isRealized;
        }
    }
}
