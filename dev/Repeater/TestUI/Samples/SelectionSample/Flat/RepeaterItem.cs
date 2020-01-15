// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.System;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Input;
using SelectionModel = Microsoft.UI.Xaml.Controls.SelectionModel;

namespace MUXControlsTestApp.Samples.Selection
{
    class RepeaterItem : ContentControl
    {
        public RepeaterItem()
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
            DependencyProperty.Register("SelectionModel", typeof(SelectionModel), typeof(RepeaterItem), new PropertyMetadata(null, new PropertyChangedCallback(OnPropertyChanged)));

        public int RepeatedIndex
        {
            get { return (int)GetValue(RepeatedIndexProperty); }
            set { SetValue(RepeatedIndexProperty, value); }
        }

        public static readonly DependencyProperty RepeatedIndexProperty =
            DependencyProperty.Register("RepeatedIndex", typeof(int), typeof(RepeaterItem), new PropertyMetadata(0, new PropertyChangedCallback(OnPropertyChanged)));

        public bool IsSelected
        {
            get { return (bool)GetValue(IsSelectedProperty); }
            set { SetValue(IsSelectedProperty, value); }
        }

        public static readonly DependencyProperty IsSelectedProperty =
            DependencyProperty.Register("IsSelected", typeof(bool), typeof(RepeaterItem), new PropertyMetadata(false));

        private static void OnPropertyChanged(DependencyObject obj, DependencyPropertyChangedEventArgs args)
        {
            if (args.Property == RepeatedIndexProperty)
            {
                var item = obj as RepeaterItem;
                if (item.SelectionModel != null)
                {
                    item.IsSelected = item.SelectionModel.IsSelected(item.RepeatedIndex).Value;
                }
            }
            else if (args.Property == SelectionModelProperty)
            {
                if (args.OldValue != null)
                {
                    (args.OldValue as SelectionModel).PropertyChanged -= (obj as RepeaterItem).OnselectionModelChanged;
                }

                if (args.NewValue != null)
                {
                    (args.NewValue as SelectionModel).PropertyChanged += (obj as RepeaterItem).OnselectionModelChanged;
                }
            }
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
                    SelectionModel.Select(RepeatedIndex);
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
                        SelectionModel.SelectRangeFromAnchor(RepeatedIndex);
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
                        SelectionModel.DeselectRangeFromAnchor(RepeatedIndex);
                    }
                    else
                    {
                        SelectionModel.SelectRangeFromAnchor(RepeatedIndex);
                    }
                }
                else if (e.KeyModifiers.HasFlag(VirtualKeyModifiers.Control))
                {
                    if (SelectionModel.IsSelected(RepeatedIndex).Value)
                    {
                        SelectionModel.Deselect(RepeatedIndex);
                    }
                    else
                    {
                        SelectionModel.Select(RepeatedIndex);
                    }
                }
                else
                {
                    SelectionModel.Select(RepeatedIndex);
                    this.Focus(FocusState.Keyboard);
                }
            }

            base.OnPointerPressed(e);
        }

        protected override AutomationPeer OnCreateAutomationPeer()
        {
            return new RepeaterItemAutomationPeer(this);
        }

        private void OnselectionModelChanged(object sender, System.ComponentModel.PropertyChangedEventArgs e)
        {
            if (e.PropertyName == "SelectedIndices")
            {
                bool oldValue = IsSelected;
                bool newValue = SelectionModel.IsSelected(RepeatedIndex).Value;
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
