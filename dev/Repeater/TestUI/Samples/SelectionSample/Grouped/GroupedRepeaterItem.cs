// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;

namespace MUXControlsTestApp.Samples.Selection
{
    class GroupedRepeaterItem : ContentControl
    {
        public GroupedRepeaterItem()
        {
            IsTabStop = true;
            UseSystemFocusVisuals = true;
            Margin = new Thickness(3);
        }

        public SelectionModel SelectionModel
        {
            get { return (SelectionModel)GetValue(SelectionModelProperty); }
            set { SetValue(SelectionModelProperty, value); }
        }

        public static readonly DependencyProperty SelectionModelProperty =
            DependencyProperty.Register("SelectionModel", typeof(SelectionModel), typeof(GroupedRepeaterItem), new PropertyMetadata(null, new PropertyChangedCallback(OnPropertyChanged)));

        public int RepeatedIndex
        {
            get { return (int)GetValue(RepeatedIndexProperty); }
            set { SetValue(RepeatedIndexProperty, value); }
        }

        public static readonly DependencyProperty RepeatedIndexProperty =
            DependencyProperty.Register("RepeatedIndex", typeof(int), typeof(GroupedRepeaterItem), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool), typeof(GroupedRepeaterItem), new PropertyMetadata(false));

        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            if (args.Property == RepeatedIndexProperty)
            {
                var item = obj as GroupedRepeaterItem;
                if (item.SelectionModel != null)
                {
                    int groupIndex = item.GetGroupIndex();
                    item.IsSelected = groupIndex >= 0 ? item.SelectionModel.IsSelected(groupIndex, item.RepeatedIndex).Value : false;
                }
            }
            else if (args.Property == SelectionModelProperty)
            {
                if (args.OldValue != null)
                {
                    (args.OldValue as SelectionModel).PropertyChanged -= (obj as GroupedRepeaterItem).OnselectionModelChanged;
                }

                if (args.NewValue != null)
                {
                    (args.NewValue as SelectionModel).PropertyChanged += (obj as GroupedRepeaterItem).OnselectionModelChanged;
                }
            }
        }

        public void Select()
        {
            SelectionModel.Select(GetGroupIndex(), RepeatedIndex);
        }

        public void Deselect()
        {
            SelectionModel.Deselect(GetGroupIndex(), RepeatedIndex);
        }

        protected override void OnKeyUp(KeyRoutedEventArgs e)
        {
            if (SelectionModel != null)
            {
                if (e.Key == VirtualKey.Escape)
                {
                    SelectionModel.ClearSelection();
                }
                else if (e.Key == VirtualKey.Space)
                {
                    Select();
                }
                else if (!SelectionModel.SingleSelect)
                {
                    var isShiftPressed = Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Shift).HasFlag(Windows.UI.Core.CoreVirtualKeyStates.Down);
                    var isCtrlPressed = Window.Current.CoreWindow.GetAsyncKeyState(VirtualKey.Control).HasFlag(Windows.UI.Core.CoreVirtualKeyStates.Down);
                    if (e.Key == VirtualKey.A && isCtrlPressed)
                    {
                        SelectionModel.SelectAll();
                    }
                    else if (isShiftPressed)
                    {
                        SelectionModel.SelectRangeFromAnchor(GetGroupIndex(), RepeatedIndex);
                    }
                }
            }

            base.OnKeyUp(e);
        }

        protected override void OnPointerPressed(PointerRoutedEventArgs e)
        {
            if (SelectionModel != null)
            {
                if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Shift) && !SelectionModel.SingleSelect)
                {
                    if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Control))
                    {
                        SelectionModel.DeselectRangeFromAnchor(GetGroupIndex(), RepeatedIndex);
                    }
                    else
                    {
                        SelectionModel.SelectRangeFromAnchor(GetGroupIndex(), RepeatedIndex);
                    }
                }
                else if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Control))
                {
                    var groupIndex = GetGroupIndex();
                    if (SelectionModel.IsSelected(groupIndex, RepeatedIndex).Value)
                    {
                        Deselect();
                    }
                    else
                    {
                        Select();
                    }
                }
                else
                {
                    Select();
                    this.Focus(FocusState.Keyboard);
                }
            }

            base.OnPointerPressed(e);
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new GroupedRepeaterItemAutomationPeer(this);
        }

        private int GetGroupIndex()
        {
            var child = this.Parent as FrameworkElement;
            var parent = child.Parent as FrameworkElement;

            while (!(parent is ItemsRepeater))
            {
                child = parent;
                parent = parent.Parent as FrameworkElement;
            }

            var groupIndex = (parent as ItemsRepeater).GetElementIndex(child);

            return groupIndex;
        }

        private void OnselectionModelChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "SelectedIndices")
            {
                bool oldValue = IsSelected;
                var groupIndex = GetGroupIndex();
                bool newValue = groupIndex >= 0 ? SelectionModel.IsSelected(groupIndex, RepeatedIndex).Value : false;

                if (oldValue != newValue)
                {
                    IsSelected = newValue;

                    // AutomationEvents.PropertyChanged is used as a value that means dont raise anything 
                    AutomationEvents eventToRaise =
                        oldValue ?
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
}
