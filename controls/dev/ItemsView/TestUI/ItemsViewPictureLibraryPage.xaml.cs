// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Private.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class ItemsViewPictureLibraryPage : TestPage
    {
        private List<string> lstEventMessage = new List<string>();
        private PictureLibrary pictureLibrary = null;
        private double[] aspectRatios = null;

        public ItemsViewPictureLibraryPage()
        {
            this.InitializeComponent();

            if (chkLogItemsViewMessages.IsChecked == true || chkLogScrollViewMessages.IsChecked == true || chkLogItemsRepeaterMessages.IsChecked == true)
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
                if (chkLogItemsViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }

            pictureLibrary = PictureLibrary.CreateLarge();
        }

        private void ChkCustomUI_Checked(object sender, RoutedEventArgs e)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Visible;
        }

        private void ChkCustomUI_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Collapsed;
        }

        private void ChkLogs_Checked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Visible;
        }

        private void ChkLogs_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Collapsed;
        }

        private void ItemsView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendEventMessage($"ItemsView.Loaded");
            if (chkLogItemsRepeaterEvents.IsChecked == true)
            {
                LogItemsRepeaterInfo();
            }
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
            LogItemsViewInfo();
        }

        private void ItemsView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendEventMessage($"ItemsView.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            if (chkLogItemsRepeaterEvents.IsChecked == true)
            {
                LogItemsRepeaterInfo();
            }
            if (chkLogScrollViewEvents.IsChecked == true)
            {
                LogScrollViewInfo();
            }
            LogItemsViewInfo();
        }

        private void ItemsView_GettingFocus(UIElement sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendEventMessage($"ItemsView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_LostFocus(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("ItemsView.LostFocus");
        }

        private void ItemsView_LosingFocus(UIElement sender, Microsoft.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendEventMessage($"ItemsView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_GotFocus(object sender, RoutedEventArgs e)
        {
            AppendEventMessage("ItemsView.GotFocus");
        }

        private void ItemsRepeater_Loaded(object sender, RoutedEventArgs e)
        {
            AppendEventMessage($"ItemsRepeater.Loaded");
            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ItemsRepeater_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendEventMessage($"ItemsRepeater.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void LinedFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            AppendEventMessage($"LinedFlowLayout.ItemsInfoRequested ItemsRangeStartIndex={args.ItemsRangeStartIndex}, ItemsRangeRequestedLength={args.ItemsRangeRequestedLength}");

            int itemsRangeStartIndex = args.ItemsRangeStartIndex;

            args.SetDesiredAspectRatios(GetAspectRatios(ref itemsRangeStartIndex, args.ItemsRangeRequestedLength));
            args.ItemsRangeStartIndex = itemsRangeStartIndex;
            args.MinWidth = 86;
        }

        private void ScrollView_Loaded(object sender, RoutedEventArgs e)
        {
            AppendEventMessage($"ScrollView.Loaded");
            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            AppendEventMessage($"ScrollView.SizeChanged Size={itemsView.ActualWidth}, {itemsView.ActualHeight}");
            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            AppendEventMessage("ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}");
        }

        private void ScrollView_StateChanged(ScrollView sender, object args)
        {
            AppendEventMessage($"ScrollView.StateChanged {sender.State.ToString()}");
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            AppendEventMessage($"ScrollView.ViewChanged HorizontalOffset={sender.HorizontalOffset.ToString()}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollView_ScrollAnimationStarting(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ScrollAnimationStarting OffsetsChangeCorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void ScrollView_ZoomAnimationStarting(ScrollView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendEventMessage($"ScrollView.ZoomAnimationStarting ZoomFactorChangeCorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendEventMessage($"ScrollView.ScrollCompleted OffsetsChangeCorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomCompleted(ScrollView sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendEventMessage($"ScrollView.ZoomCompleted ZoomFactorChangeCorrelationId={args.CorrelationId}");
        }

        private void LogItemsRepeaterInfo()
        {
            ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

            AppendEventMessage($"ItemsRepeater Info: ItemsSource={itemsRepeater.ItemsSource}, ItemTemplate={itemsRepeater.ItemTemplate}, Layout={itemsRepeater.Layout}");
        }

        private void LogScrollViewInfo()
        {
            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            AppendEventMessage($"ScrollView Info: HorizontalOffset={scrollView.HorizontalOffset}, VerticalOffset={scrollView.VerticalOffset}, ZoomFactor={scrollView.ZoomFactor}");
            AppendEventMessage($"ScrollView Info: ViewportWidth={scrollView.ViewportWidth}, ExtentHeight={scrollView.ViewportHeight}");
            AppendEventMessage($"ScrollView Info: ExtentWidth={scrollView.ExtentWidth}, ExtentHeight={scrollView.ExtentHeight}");
            AppendEventMessage($"ScrollView Info: ScrollableWidth={scrollView.ScrollableWidth}, ScrollableHeight={scrollView.ScrollableHeight}");
        }

        private void LogItemsViewInfo()
        {
            //AppendEventMessage($"ItemsView Info: ItemsSource={itemsView.ItemsSource}, ItemTemplate={itemsView.ItemTemplate}, Layout={itemsView.Layout}");
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs e)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogItemsRepeaterEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.Loaded += ItemsRepeater_Loaded;
                    itemsRepeater.SizeChanged += ItemsRepeater_SizeChanged;
                }
            }
        }

        private void ChkLogItemsRepeaterEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.Loaded -= ItemsRepeater_Loaded;
                    itemsRepeater.SizeChanged -= ItemsRepeater_SizeChanged;
                }
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
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
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
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
        }

        private void ChkLogItemsViewEvents_Checked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus += ItemsView_GettingFocus;
                itemsView.GotFocus += ItemsView_GotFocus;
                itemsView.LosingFocus += ItemsView_LosingFocus;
                itemsView.LostFocus += ItemsView_LostFocus;
                itemsView.Loaded += ItemsView_Loaded;
                itemsView.SizeChanged += ItemsView_SizeChanged;
            }
        }

        private void ChkLogItemsViewEvents_Unchecked(object sender, RoutedEventArgs e)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus -= ItemsView_GettingFocus;
                itemsView.GotFocus -= ItemsView_GotFocus;
                itemsView.LosingFocus -= ItemsView_LosingFocus;
                itemsView.LostFocus -= ItemsView_LostFocus;
                itemsView.Loaded -= ItemsView_Loaded;
                itemsView.SizeChanged -= ItemsView_SizeChanged;
            }
        }

        private void ChkLogItemsRepeaterMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsRepeaterMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogScrollViewMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Checked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Unchecked(object sender, RoutedEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false && chkLogItemsRepeaterMessages.IsChecked == false)
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

        private double[] GetAspectRatios(ref int itemsRangeStartIndex, int itemsRangeRequestedLength)
        {
            const int bufferedAspectRatiosLength = 500;
            int cappedBufferedAspectRatiosLength = Math.Min(bufferedAspectRatiosLength, pictureLibrary.Count);

            cappedBufferedAspectRatiosLength = Math.Max(cappedBufferedAspectRatiosLength, itemsRangeRequestedLength);

            if (aspectRatios == null || aspectRatios.Length != cappedBufferedAspectRatiosLength)
            {
                aspectRatios = new double[cappedBufferedAspectRatiosLength];
            }

            if (itemsRangeStartIndex > 0 && itemsRangeRequestedLength < cappedBufferedAspectRatiosLength)
            {
                itemsRangeStartIndex -= (cappedBufferedAspectRatiosLength - itemsRangeRequestedLength) / 2;
            }

            if (itemsRangeStartIndex + cappedBufferedAspectRatiosLength - 1 >= pictureLibrary.Count)
            {
                cappedBufferedAspectRatiosLength -= itemsRangeStartIndex + cappedBufferedAspectRatiosLength - pictureLibrary.Count;
            }

            for (int i = 0; i < cappedBufferedAspectRatiosLength; i++)
            {
                aspectRatios[i] = pictureLibrary[itemsRangeStartIndex + i].AspectRatio;
            }

            AppendEventMessage($"GetAspectRatios: itemsRangeStartIndex={itemsRangeStartIndex}, cappedBufferedAspectRatiosLength={cappedBufferedAspectRatiosLength}");

            return aspectRatios;
        }

        private void AppendEventMessage(string eventMessage)
        {
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
            AppendEventMessage();
        }

        private void AppendEventMessage()
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

    public class Picture
    {
        public string Path { get; set; }
        public string Title { get; set; } = string.Empty;
        public int Id { get; set; }

        public double AspectRatio { get; set; } = 1;

        public Location Location { get; set; }
    }

    public enum Location
    {
        Seattle,
        Miami,
        LA
    }

    public class PictureLibrary : ObservableCollection<Picture>
    {
        public string Title { get; set; }

        public PictureLibrary()
        {
        }

        public static PictureLibrary CreateSmall()
        {
            if (s_SmallLibrary == null)
            {
                s_SmallLibrary = new PictureLibrary()
                {
                    new Picture() { Path = "ms-appx:///Images/Picture1.png",  Location = Location.Seattle},
                    new Picture() { Path = "ms-appx:///Images/Picture2.png",  Location = Location.LA},
                    new Picture() { Path = "ms-appx:///Images/Picture3.png",  Location = Location.Miami},
                    new Picture() { Path = "ms-appx:///Images/Picture4.png",  Location = Location.Seattle},
                    new Picture() { Path = "ms-appx:///Images/Picture5.png",  Location = Location.LA},
                    new Picture() { Path = "ms-appx:///Images/Picture6.png",  Location = Location.Miami},
                    new Picture() { Path = "ms-appx:///Images/Picture7.png",  Location = Location.Seattle},
                    new Picture() { Path = "ms-appx:///Images/Picture8.png",  Location = Location.LA},
                    new Picture() { Path = "ms-appx:///Images/Picture9.png",  Location = Location.Miami},
                    new Picture() { Path = "ms-appx:///Images/Picture10.png", Location = Location.Seattle},
                    new Picture() { Path = "ms-appx:///Images/Picture11.png", Location = Location.LA},
                    new Picture() { Path = "ms-appx:///Images/Picture12.png", Location = Location.Miami},
                    new Picture() { Path = "ms-appx:///Images/Picture13.png", Location = Location.Seattle}
                };
            }

            return s_SmallLibrary;
        }

        public static PictureLibrary CreateLarge()
        {
            if (s_LargeLibrary == null)
            {
                var library = new PictureLibrary();

                for (int i = 0; i < 1000000 / 40; i++)
                {
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture1.png",  Title = "Image", Id = i * 40,      Location = Location.Seattle, AspectRatio = 1.497 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture2.png",  Title = "Image", Id = i * 40 + 1,  Location = Location.LA, AspectRatio = 1.529 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture3.png",  Title = "Image", Id = i * 40 + 2,  Location = Location.Miami, AspectRatio = 0.820 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture4.png",  Title = "Image", Id = i * 40 + 3,  Location = Location.Seattle, AspectRatio = 1.504 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture5.png",  Title = "Image", Id = i * 40 + 4,  Location = Location.LA, AspectRatio = 1.267 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture6.png",  Title = "Image", Id = i * 40 + 5,  Location = Location.Miami, AspectRatio = 0.740 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture7.png",  Title = "Image", Id = i * 40 + 6,  Location = Location.Seattle, AspectRatio = 1.488 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture8.png",  Title = "Image", Id = i * 40 + 7,  Location = Location.LA, AspectRatio = 0.727 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture9.png",  Title = "Image", Id = i * 40 + 8,  Location = Location.Miami, AspectRatio = 1.488 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture10.png", Title = "Image", Id = i * 40 + 9,  Location = Location.Seattle, AspectRatio = 0.813 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture11.png", Title = "Image", Id = i * 40 + 10, Location = Location.Seattle, AspectRatio = 0.797 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture12.png", Title = "Image", Id = i * 40 + 11, Location = Location.LA, AspectRatio = 1.496 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture13.png", Title = "Image", Id = i * 40 + 12, Location = Location.Miami, AspectRatio = 0.555 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture14.png", Title = "Image", Id = i * 40 + 13, Location = Location.Seattle, AspectRatio = 1.777 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture15.png", Title = "Image", Id = i * 40 + 14, Location = Location.LA, AspectRatio = 0.858 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture16.png", Title = "Image", Id = i * 40 + 15, Location = Location.Miami, AspectRatio = 1.782 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture17.png", Title = "Image", Id = i * 40 + 16, Location = Location.Seattle, AspectRatio = 1.334 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture18.png", Title = "Image", Id = i * 40 + 17, Location = Location.LA, AspectRatio = 0.998 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture19.png", Title = "Image", Id = i * 40 + 18, Location = Location.Miami, AspectRatio = 1.790 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture20.png", Title = "Image", Id = i * 40 + 19, Location = Location.Seattle, AspectRatio = 0.569 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture21.png", Title = "Image", Id = i * 40 + 20, Location = Location.Seattle, AspectRatio = 1.787 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture22.png", Title = "Image", Id = i * 40 + 21, Location = Location.LA, AspectRatio = 0.742 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture23.png", Title = "Image", Id = i * 40 + 22, Location = Location.Miami, AspectRatio = 0.555 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture24.png", Title = "Image", Id = i * 40 + 23, Location = Location.Seattle, AspectRatio = 1.778 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture25.png", Title = "Image", Id = i * 40 + 24, Location = Location.LA, AspectRatio = 0.998 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture26.png", Title = "Image", Id = i * 40 + 25, Location = Location.Miami, AspectRatio = 1.002 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture27.png", Title = "Image", Id = i * 40 + 26, Location = Location.Seattle, AspectRatio = 0.745 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture28.png", Title = "Image", Id = i * 40 + 27, Location = Location.LA, AspectRatio = 0.748 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture29.png", Title = "Image", Id = i * 40 + 28, Location = Location.Miami, AspectRatio = 1.512 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture30.png", Title = "Image", Id = i * 40 + 29, Location = Location.Seattle, AspectRatio = 0.997 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture31.png", Title = "Image", Id = i * 40 + 30, Location = Location.Seattle, AspectRatio = 0.798 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture32.png", Title = "Image", Id = i * 40 + 31, Location = Location.LA, AspectRatio = 1.501 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture33.png", Title = "Image", Id = i * 40 + 32, Location = Location.Miami, AspectRatio = 1.509 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture34.png", Title = "Image", Id = i * 40 + 33, Location = Location.Seattle, AspectRatio = 1.340 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture35.png", Title = "Image", Id = i * 40 + 34, Location = Location.LA, AspectRatio = 1.327 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture36.png", Title = "Image", Id = i * 40 + 35, Location = Location.Miami, AspectRatio = 1.848 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture37.png", Title = "Image", Id = i * 40 + 36, Location = Location.Seattle, AspectRatio = 1.526 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture38.png", Title = "Image", Id = i * 40 + 37, Location = Location.LA, AspectRatio = 0.743 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture39.png", Title = "Image", Id = i * 40 + 38, Location = Location.Miami, AspectRatio = 0.742 });
                    library.Add(new Picture() { Path = "ms-appx:///Images/Picture40.png", Title = "Image", Id = i * 40 + 39, Location = Location.Seattle, AspectRatio = 0.742 });
                };

                s_LargeLibrary = library;
            }

            return s_LargeLibrary;
        }

        static PictureLibrary s_LargeLibrary = null;
        static PictureLibrary s_SmallLibrary = null;
    }
}
