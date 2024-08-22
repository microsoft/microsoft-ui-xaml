// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Navigation;
using MUXControlsTestApp.Utilities;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using Windows.UI;

using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;

namespace MUXControlsTestApp.Samples
{
    public sealed partial class ObjectModelTestPage : TestPage
    {
        private List<int> _collapsedElements = null;
        private ObservableCollection<string> _colStrings = null;

        public ObjectModelTestPage()
        {
            this.InitializeComponent();

            if (chkLogScrollViewMessages.IsChecked == true || chkLogItemsRepeaterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogItemsRepeaterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }

            if (itemsRepeater != null)
            {
                itemsRepeater.ElementPrepared += ItemsRepeater_ElementPrepared;
                itemsRepeater.ElementClearing += ItemsRepeater_ElementClearing;
            }
        }

        ~ObjectModelTestPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            if (itemsRepeater != null)
            {
                itemsRepeater.ElementPrepared -= ItemsRepeater_ElementPrepared;
                itemsRepeater.ElementClearing -= ItemsRepeater_ElementClearing;
            }

            base.OnNavigatedFrom(e);
        }

        private void CmbItemTemplate_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    if (cmbItemTemplate.SelectedIndex == 0)
                    {
                        itemsRepeater.ItemTemplate = null;
                    }
                    else
                    {
                        itemsRepeater.ItemTemplate = Resources["template" + cmbItemTemplate.SelectedIndex.ToString()] as DataTemplate;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbItemsSource_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    switch (cmbItemsSource.SelectedIndex)
                    {
                        case 0:
                            if (itemsRepeater.ItemsSource != null)
                            {
                                itemsRepeater.ItemsSource = null;
                                _collapsedElements = null;
                                btnChangeEvenItemsVisibility.Content = "Collapse even realized items";
                                btnChangeOddItemsVisibility.Content = "Collapse odd realized items";
                            }
                            break;
                        case 1:
                            if (_colStrings == null)
                            {
                                _colStrings = new ObservableCollection<string>();

                                for (int itemIndex = 0; itemIndex < 200; itemIndex++)
                                {
                                    _colStrings.Add("Item" + itemIndex);
                                }
                            }
                            itemsRepeater.ItemsSource = _colStrings;
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void CmbLayout_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null)
                {
                    switch (cmbLayout.SelectedIndex)
                    {
                        case 0:
                            itemsRepeater.Layout = null;
                            break;
                        case 1:
                            itemsRepeater.Layout = new StackLayout();
                            break;
                        case 2:
                            itemsRepeater.Layout = new UniformGridLayout();
                            break;
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnChangeEvenItemsVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null && _colStrings != null)
                {
                    Visibility newVisibility = Visibility.Collapsed;

                    if (btnChangeEvenItemsVisibility.Content as string == "Collapse even realized items")
                    {
                        btnChangeEvenItemsVisibility.Content = "Show even realized items";
                    }
                    else
                    {
                        btnChangeEvenItemsVisibility.Content = "Collapse even realized items";
                        newVisibility = Visibility.Visible;
                    }

                    if (_collapsedElements == null)
                    {
                        _collapsedElements = new List<int>();
                    }

                    for (int i = 0; i < (_colStrings.Count + 1) / 2; i++)
                    {
                        UIElement element = itemsRepeater.TryGetElement(2 * i);
                        if (element != null)
                        {
                            element.Visibility = newVisibility;

                            if (newVisibility == Visibility.Visible)
                            {
                                if (_collapsedElements.Contains(2 * i))
                                {
                                    _collapsedElements.Remove(2 * i);
                                }
                            }
                            else
                            {
                                if (!_collapsedElements.Contains(2 * i))
                                {
                                    _collapsedElements.Add(2 * i);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void BtnChangeOddItemsVisibility_Click(object sender, RoutedEventArgs e)
        {
            try
            {
                if (itemsRepeater != null && _colStrings != null)
                {
                    Visibility newVisibility = Visibility.Collapsed;

                    if (btnChangeOddItemsVisibility.Content as string == "Collapse odd realized items")
                    {
                        btnChangeOddItemsVisibility.Content = "Show odd realized items";
                    }
                    else
                    {
                        btnChangeOddItemsVisibility.Content = "Collapse odd realized items";
                        newVisibility = Visibility.Visible;
                    }

                    if (_collapsedElements == null)
                    {
                        _collapsedElements = new List<int>();
                    }

                    for (int i = 0; i < _colStrings.Count / 2; i++)
                    {
                        UIElement element = itemsRepeater.TryGetElement(2 * i + 1);
                        if (element != null)
                        {
                            element.Visibility = newVisibility;

                            if (newVisibility == Visibility.Visible)
                            {
                                if (_collapsedElements.Contains(2 * i + 1))
                                {
                                    _collapsedElements.Remove(2 * i + 1);
                                }
                            }
                            else
                            {
                                if (!_collapsedElements.Contains(2 * i + 1))
                                {
                                    _collapsedElements.Add(2 * i + 1);
                                }
                            }
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ChkItemsRepeaterProperties_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (grdItemsRepeaterProperties != null && chkItemsRepeaterProperties != null)
                grdItemsRepeaterProperties.Visibility = chkItemsRepeaterProperties.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        }

        private void ChkLogs_IsCheckedChanged(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null && chkLogs != null)
                grdLogs.Visibility = chkLogs.IsChecked == true ? Visibility.Visible : Visibility.Collapsed;
        }

        private void ScrollView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendEventMessage($"ScrollView.Loaded");
            LogScrollViewInfo();
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendEventMessage($"ScrollView.SizeChanged Size={scrollView.ActualWidth}, {scrollView.ActualHeight}");
            LogScrollViewInfo();
        }

        private void ScrollView_GettingFocus(UIElement sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendEventMessage($"ScrollView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("ScrollView.LostFocus");
        }

        private void ScrollView_LosingFocus(UIElement sender, Microsoft.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendEventMessage($"ScrollView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ScrollView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("ScrollView.GotFocus");
        }

        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            AppendEventMessage($"ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollView_StateChanged(ScrollView sender, object args)
        {
            AppendEventMessage($"ScrollView.StateChanged {sender.State.ToString()}");
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendEventMessage($"ScrollView.ViewChanged HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollView_ScrollAnimationStarting(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ScrollAnimationStarting OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomAnimationStarting(ScrollView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ZoomAnimationStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}");
        }

        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendEventMessage($"ScrollView.ScrollCompleted OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomCompleted(ScrollView sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendEventMessage($"ScrollView.ZoomCompleted ZoomFactorChangeCorrelationId={args.CorrelationId}");
        }

        private void ItemsRepeater_Loaded(object sender, RoutedEventArgs e)
        {
            AppendEventMessage($"ItemsRepeater.Loaded");
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
        }

        private void ItemsRepeater_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendEventMessage($"ItemsRepeater.SizeChanged Size={scrollView.ActualWidth}, {scrollView.ActualHeight}");
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
        }

        private void ItemsRepeater_ElementPrepared(ItemsRepeater sender, ItemsRepeaterElementPreparedEventArgs args)
        {
            try
            {                 
                if (chkLogItemsRepeaterEvents.IsChecked == true)
                {
                    AppendEventMessage($"ItemsRepeater.ElementPrepared Index={args.Index}, Element={args.Element}, Visibility={args.Element.Visibility}");
                }

                if (_collapsedElements != null && _collapsedElements.Contains(args.Index))
                {
                    if (args.Element.Visibility == Visibility.Visible)
                    {
                        args.Element.Visibility = Visibility.Collapsed;
                    }
                }
                else if (args.Element.Visibility == Visibility.Collapsed)
                {
                    args.Element.Visibility = Visibility.Visible;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void ItemsRepeater_ElementClearing(ItemsRepeater sender, ItemsRepeaterElementClearingEventArgs args)
        {
            try
            {
                if (chkLogItemsRepeaterEvents.IsChecked == true)
                {
                    AppendEventMessage($"ItemsRepeater.ElementClearing Element={args.Element}, Visibility={args.Element.Visibility}");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendEventMessage(ex.ToString());
            }
        }

        private void LogScrollViewInfo()
        {
            AppendEventMessage($"ScrollView Info: HorizontalOffset={scrollView.HorizontalOffset}, VerticalOffset={scrollView.VerticalOffset}, ZoomFactor={scrollView.ZoomFactor}");
            AppendEventMessage($"ScrollView Info: ViewportWidth={scrollView.ViewportWidth}, ExtentHeight={scrollView.ViewportHeight}");
            AppendEventMessage($"ScrollView Info: ExtentWidth={scrollView.ExtentWidth}, ExtentHeight={scrollView.ExtentHeight}");
            AppendEventMessage($"ScrollView Info: ScrollableWidth={scrollView.ScrollableWidth}, ScrollableHeight={scrollView.ScrollableHeight}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogItemsRepeaterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsRepeater != null)
            {
                itemsRepeater.Loaded += ItemsRepeater_Loaded;
                itemsRepeater.SizeChanged += ItemsRepeater_SizeChanged;
            }
        }

        private void ChkLogItemsRepeaterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsRepeater != null)
            {
                itemsRepeater.Loaded -= ItemsRepeater_Loaded;
                itemsRepeater.SizeChanged -= ItemsRepeater_SizeChanged;
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                scrollView.GettingFocus += ScrollView_GettingFocus;
                scrollView.GotFocus += ScrollView_GotFocus;
                scrollView.LosingFocus += ScrollView_LosingFocus;
                scrollView.LostFocus += ScrollView_LostFocus;
                scrollView.Loaded += ScrollView_Loaded;
                scrollView.SizeChanged += ScrollView_SizeChanged;
                scrollView.ExtentChanged += ScrollView_ExtentChanged;
                scrollView.StateChanged += ScrollView_StateChanged;
                scrollView.ViewChanged += ScrollView_ViewChanged;
                scrollView.ScrollAnimationStarting += ScrollView_ScrollAnimationStarting;
                scrollView.ZoomAnimationStarting += ScrollView_ZoomAnimationStarting;
                scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
                scrollView.ZoomCompleted += ScrollView_ZoomCompleted;
            }
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (scrollView != null)
            {
                scrollView.GettingFocus -= ScrollView_GettingFocus;
                scrollView.GotFocus -= ScrollView_GotFocus;
                scrollView.LosingFocus -= ScrollView_LosingFocus;
                scrollView.LostFocus -= ScrollView_LostFocus;
                scrollView.Loaded -= ScrollView_Loaded;
                scrollView.SizeChanged -= ScrollView_SizeChanged;
                scrollView.ExtentChanged -= ScrollView_ExtentChanged;
                scrollView.StateChanged -= ScrollView_StateChanged;
                scrollView.ViewChanged -= ScrollView_ViewChanged;
                scrollView.ScrollAnimationStarting -= ScrollView_ScrollAnimationStarting;
                scrollView.ZoomAnimationStarting -= ScrollView_ZoomAnimationStarting;
                scrollView.ScrollCompleted -= ScrollView_ScrollCompleted;
                scrollView.ZoomCompleted -= ScrollView_ZoomCompleted;
            }
        }

        private void ChkLogItemsRepeaterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsRepeaterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string eventMessage = string.Empty;
            string senderName = string.Empty;

            try
            {
                FrameworkElement fe = sender as FrameworkElement;

                if (fe != null)
                {
                    senderName = "s:" + fe.Name + ", ";
                }
            }
            catch
            {
            }

            if (args.IsVerboseLevel)
            {
                eventMessage = "Verbose: " + senderName + "m:" + msg;
            }
            else
            {
                eventMessage = "Info: " + senderName + "m:" + msg;
            }

            AppendEventMessage(eventMessage);
        }

        private void AppendEventMessage(string eventMessage)
        {
            List<string> lstEventMessage = new List<string>();

            while (eventMessage.Length > 0)
            {
                string msgHead = eventMessage;

                if (eventMessage.Length > 110)
                {
                    int commaIndex = eventMessage.IndexOf(',', 110);
                    if (commaIndex != -1)
                    {
                        msgHead = eventMessage.Substring(0, commaIndex);
                        eventMessage = eventMessage.Substring(commaIndex + 1);
                    }
                    else
                    {
                        eventMessage = string.Empty;
                    }
                }
                else
                {
                    eventMessage = string.Empty;
                }

                lstEventMessage.Add(msgHead);
            }
            AppendEventMessage(lstEventMessage);
        }

        private void AppendEventMessage(List<string> lstEventMessage)
        {
            foreach (string eventMessage in lstEventMessage)
            {
                lstLogs.Items.Add(eventMessage);
            }
            lstEventMessage.Clear();
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs e)
        {
            txtExceptionReport.Text = string.Empty;
        }
    }
}
