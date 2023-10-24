// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using Microsoft.UI;
using Microsoft.UI.Private.Controls;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Media.Imaging;
using Microsoft.UI.Xaml.Navigation;
using MUXControlsTestApp.Samples.Model;

using ItemsRepeater = Microsoft.UI.Xaml.Controls.ItemsRepeater;
using ScrollView = Microsoft.UI.Xaml.Controls.ScrollView;
using ItemsView = Microsoft.UI.Xaml.Controls.ItemsView;
using ScrollingScrollAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollAnimationStartingEventArgs;
using ScrollingZoomAnimationStartingEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomAnimationStartingEventArgs;
using ScrollingScrollCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingScrollCompletedEventArgs;
using ScrollingZoomCompletedEventArgs = Microsoft.UI.Xaml.Controls.ScrollingZoomCompletedEventArgs;
using MUXControlsTestHooks = Microsoft.UI.Private.Controls.MUXControlsTestHooks;
using MUXControlsTestHooksLoggingMessageEventArgs = Microsoft.UI.Private.Controls.MUXControlsTestHooksLoggingMessageEventArgs;
using ItemsViewTestHooks = Microsoft.UI.Private.Controls.ItemsViewTestHooks;

namespace MUXControlsTestApp
{
    public sealed partial class ItemsViewIntegrationPage : TestPage
    {
        private object _asyncEventReportingLock = new object();
        private List<string> _lstAsyncEventMessage = new List<string>();
        private List<string> _fullLogs = new List<string>();
        private ObservableCollection<Recipe> _colPhotos = null;
        private DataTemplate[] _photoTemplates = new DataTemplate[8];
        private SolidColorBrush _redBrush = new SolidColorBrush(Colors.Red);
        private SolidColorBrush _whiteBrush = new SolidColorBrush(Colors.White);
        private ScrollView _scrollView = null;
        private BringIntoViewOptions _bringIntoViewOptions = null;
        private int[] _yearPhotoCounts = null;
        private int[,] _monthPhotoCounts = null;
        private bool _populateAnnotatedScrollBar = true;
        private int _requestedDetailLabelYearIndex = 0;
        private int _requestedDetailLabelPhotoInYearIndex = 0;
        private int _populateAnnotatedScrollBarPending = 0;
        private int _populateHeaderPending = 0;
        private int _yearCount = 2; // 16;    TODO: Restore 16 after bug "43218932 - Out of memory error when loading images" was fixed.
        private int _averageMonthCount = 2; // 50;    TODO: Restore 50 after bug "43218932 - Out of memory error when loading images" was fixed.
        private string[] _months = new string[12] { "December", "November", "October", "September", "August", "July", "June", "May", "April", "March", "February", "January" };
        private double _labelSize = 0.0;

        public ItemsViewIntegrationPage()
        {
            this.InitializeComponent();

            if (chkLogAnnotatedScrollBarMessages.IsChecked == true ||
                chkLogLinedFlowLayoutMessages.IsChecked == true ||
                chkLogItemsViewMessages.IsChecked == true ||
                chkLogScrollPresenterMessages.IsChecked == true ||
                chkLogScrollViewMessages.IsChecked == true ||
                chkLogItemsRepeaterMessages.IsChecked == true)
            {
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;

                if (chkLogItemsRepeaterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollPresenterMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogScrollViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogItemsViewMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogLinedFlowLayoutMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
                if (chkLogAnnotatedScrollBarMessages.IsChecked == true)
                {
                    MUXControlsTestHooks.SetLoggingLevelForType("AnnotatedScrollBar", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
                }
            }

            this.Loaded += ItemsViewIntegrationPage_Loaded;
            this.KeyDown += ItemsViewIntegrationPage_KeyDown;
            PopulatePhotosCollection();
        }

        ~ItemsViewIntegrationPage()
        {
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("AnnotatedScrollBar", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);

            MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;

            UnhookLinedFlowLayoutEvents(justItemsInfoRequested: false, exceptItemsInfoRequested: false);
            UnhookScrollViewEvents();
            UnhookAnnotatedScrollBarEvents();
            UnhookPhotosCollectionChanged();

            ChkLogLinedFlowLayoutEvents_Unchecked(null, null);
            ChkLogScrollPresenterEvents_Unchecked(null, null);
            ChkLogScrollViewEvents_Unchecked(null, null);
            ChkLogItemsViewEvents_Unchecked(null, null);
            ChkLogItemsRepeaterEvents_Unchecked(null, null);
            ChkLogAnnotatedScrollBarEvents_Unchecked(null, null);

            base.OnNavigatedFrom(e);
        }

        private void ItemsViewIntegrationPage_Loaded(object sender, RoutedEventArgs args)
        {
            _scrollView = itemsView.GetValue(ItemsView.ScrollViewProperty) as ScrollView;

            HookScrollViewEvents();

            itemsView.ItemsSource = _colPhotos;

            UpdateItemsInfoRequestedHandler();
            HookLinedFlowLayoutEvents(justItemsInfoRequested: false, exceptItemsInfoRequested: true);
            HookAnnotatedScrollBarEvents();

            BtnGetAverageMonthCount_Click(null, null);
            BtnGetYearCount_Click(null, null);
        }

        private void ItemsViewIntegrationPage_KeyDown(object sender, KeyRoutedEventArgs e)
        {
            if (e.Key == Windows.System.VirtualKey.G)
            {
                GetFullLog();
            }
            else if (e.Key == Windows.System.VirtualKey.C)
            {
                ClearFullLog();
            }
        }

        private void HookAnnotatedScrollBarEvents()
        {
            annotatedScrollBar.Scrolling += AnnotatedScrollBar_Scrolling;
            annotatedScrollBar.DetailLabelRequested += AnnotatedScrollBar_DetailLabelRequested;
            annotatedScrollBar.SizeChanged += AnnotatedScrollBar_SizeChanged;
        }

        private void UnhookAnnotatedScrollBarEvents()
        {
            annotatedScrollBar.Scrolling -= AnnotatedScrollBar_Scrolling;
            annotatedScrollBar.DetailLabelRequested -= AnnotatedScrollBar_DetailLabelRequested;
            annotatedScrollBar.SizeChanged -= AnnotatedScrollBar_SizeChanged;
        }

        private void HookLinedFlowLayoutEvents(bool justItemsInfoRequested, bool exceptItemsInfoRequested)
        {
            if (!exceptItemsInfoRequested)
            {
                linedFlowLayout.ItemsInfoRequested += LinedFlowLayout_ItemsInfoRequested;
            }
            if (justItemsInfoRequested)
            {
                return;
            }
            linedFlowLayout.ItemsUnlocked += LinedFlowLayout_ItemsUnlocked;
        }

        private void HookScrollViewEvents()
        {
            if (_scrollView != null)
            {
                _scrollView.ExtentChanged += ScrollView_ExtentChanged;
                _scrollView.ViewChanged += ScrollView_ViewChanged;
                _scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
                _scrollView.SizeChanged += ScrollView_SizeChanged;
            }
        }

        private void UnhookScrollViewEvents()
        {
            if (_scrollView != null)
            {
                _scrollView.ExtentChanged -= ScrollView_ExtentChanged;
                _scrollView.ViewChanged -= ScrollView_ViewChanged;
                _scrollView.ScrollCompleted -= ScrollView_ScrollCompleted;
                _scrollView.SizeChanged -= ScrollView_SizeChanged;
            }
        }

        private void UnhookLinedFlowLayoutEvents(bool justItemsInfoRequested, bool exceptItemsInfoRequested)
        {
            if (!exceptItemsInfoRequested)
            {
                linedFlowLayout.ItemsInfoRequested -= LinedFlowLayout_ItemsInfoRequested;
            }
            if (justItemsInfoRequested)
            {
                return;
            }
            linedFlowLayout.ItemsUnlocked -= LinedFlowLayout_ItemsUnlocked;
        }

        private void HookPhotosCollectionChanged()
        {
            if (_colPhotos != null)
            {
                _colPhotos.CollectionChanged += Photos_CollectionChanged;
            }
        }

        private void UnhookPhotosCollectionChanged()
        {
            if (_colPhotos != null)
            {
                _colPhotos.CollectionChanged -= Photos_CollectionChanged;
            }
        }

        // Gets the item index from the most recent DetailLabelRequested event, using
        // the cached _requestedDetailLabelYearIndex & _requestedDetailLabelPhotoInYearIndex values.
        private int GetItemIndexFromLastDetailLabelRequest()
        {
            int cumulatedPhotos = 0;

            for (int yearIndex = 0; yearIndex < _requestedDetailLabelYearIndex; yearIndex++)
            {
                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                cumulatedPhotos += _yearPhotoCounts[yearIndex];
            }

            return cumulatedPhotos + _requestedDetailLabelPhotoInYearIndex;
        }

        private double GetLabelSize()
        {
            // This value needs to be recalculated when OS text size changes
            if (_labelSize != 0.0)
            {
                return _labelSize;
            }

            DateTime today = DateTime.Now.Date;
            AnnotatedScrollBarLabel annotatedScrollBarTestLabel = new AnnotatedScrollBarLabel(today.Year.ToString(), 0.0);

            ElementFactoryGetArgs args = new ElementFactoryGetArgs();
            args.Data = annotatedScrollBarTestLabel;

            DataTemplate labelTemplate = (DataTemplate)annotatedScrollBar.LabelTemplate;
            UIElement testLabel = labelTemplate.GetElement(args);
            testLabel.Measure(new Windows.Foundation.Size(double.PositiveInfinity, double.PositiveInfinity));

            _labelSize = testLabel.DesiredSize.Height;
            return _labelSize;
        }

        private string GetMonthNameFromIndex(int monthIndex)
        {
            if (monthIndex == 12)
            {
                // Workaround for AnnotatedScrollBarDetailLabelRequestedEventArgs.Value exceeding AnnotatedScrollBar.Maximum + AnnotatedScrollBar.ViewportSize
                monthIndex = 11;
            }

            return _months[monthIndex];
        }

        // Populates the AnnotatedScrollBar's Label collection based on the indexes of the last photo in each year (chronologically speaking).
        private void PopulateAnnotatedScrollBar()
        {
            Debug.Assert(_populateAnnotatedScrollBarPending > 0);
            _populateAnnotatedScrollBarPending--;

            if (_populateAnnotatedScrollBarPending > 0)
            {
                return;
            }

            if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage($"PopulateAnnotatedScrollBar");
            }

            if (annotatedScrollBar.Labels.Count > 0)
            {
                AppendAsyncEventMessage($"PopulateAnnotatedScrollBar - Clearing AnnotatedScrollBar labels");
                annotatedScrollBar.Labels.Clear();
            }

            if (itemsView.ItemsSource == null || _yearPhotoCounts == null || _monthPhotoCounts == null)
            {
                return;
            }

            double labelSize = GetLabelSize();
            double lineAndSpacingSize = linedFlowLayout.ActualLineHeight + linedFlowLayout.LineSpacing;
            int cumulatedPhotos = _yearPhotoCounts[0];
            DateTime today = DateTime.Now.Date;

            AnnotatedScrollBarLabel annotatedScrollBarLabel = new AnnotatedScrollBarLabel(today.Year.ToString(), 0.0);

            annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);
            AppendAsyncEventMessage($"PopulateAnnotatedScrollBar - Adding AnnotatedScrollBarLabel with Content={annotatedScrollBarLabel.Content}, ScrollOffset={annotatedScrollBarLabel.ScrollOffset}");

            double previousLabelBottom = labelSize;
            for (int yearIndex = 1; yearIndex < _yearCount; yearIndex++)
            {
                int year = today.Year - yearIndex;
                int lineIndex = linedFlowLayout.LockItemToLine(cumulatedPhotos);
                double value = lineIndex * lineAndSpacingSize;
                double currentLabelTop = value * annotatedScrollBar.ScrollOffsetToLabelOffsetFactor;
                bool doesLabelCollide = currentLabelTop <= previousLabelBottom;
                if (!doesLabelCollide)
                {
                    annotatedScrollBarLabel = new AnnotatedScrollBarLabel(year.ToString(), value);

                    annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);
                    AppendAsyncEventMessage($"PopulateAnnotatedScrollBar - Adding AnnotatedScrollBarLabel with Content={annotatedScrollBarLabel.Content}, ScrollOffset={annotatedScrollBarLabel.ScrollOffset}");

                    previousLabelBottom = currentLabelTop + labelSize;
                }
                else if (yearIndex == _yearCount - 1)
                {
                    // We always want the last label to be shown.
                    AppendAsyncEventMessage($"PopulateAnnotatedScrollBar - Removing AnnotatedScrollBarLabel with Content={annotatedScrollBar.Labels[annotatedScrollBar.Labels.Count - 1].Content}, ScrollOffset={annotatedScrollBar.Labels[annotatedScrollBar.Labels.Count - 1].ScrollOffset}");
                    annotatedScrollBar.Labels.RemoveAt(annotatedScrollBar.Labels.Count - 1);

                    annotatedScrollBarLabel = new AnnotatedScrollBarLabel(year.ToString(), value);

                    annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);
                    AppendAsyncEventMessage($"PopulateAnnotatedScrollBar - Adding AnnotatedScrollBarLabel with Content={annotatedScrollBarLabel.Content}, ScrollOffset={annotatedScrollBarLabel.ScrollOffset}");
                }

                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                cumulatedPhotos += _yearPhotoCounts[yearIndex];
            }
        }

        private void PopulateHeaderAsync()
        {
            _populateHeaderPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateHeader);
        }

        // Retrieves the first and last relevant ItemContainer instances from the ItemsView and populates the tblHeader TextBlock with the range.
        // 'relevant' means at least 16px are displayed on screen vertically.
        // 'FirstMonth FirstYear - LastMonth LastYear' when range exceeds one month, and
        // 'Month Year' when the range falls within a single month.
        private void PopulateHeader()
        {
            Debug.Assert(_populateHeaderPending > 0);
            _populateHeaderPending--;

            if (_populateHeaderPending > 0 || _scrollView == null)
            {
                return;
            }

            const double requiredDisplayedHeight = 16.0;
            int firstRelevantItemContainerIndex;
            int lastRelevantItemContainerIndex; 

            itemsView.TryGetItemIndex(
                        horizontalViewportRatio: 0.0, 
                        verticalViewportRatio: ((linedFlowLayout.LineSpacing - linedFlowLayout.LineHeight) / 2.0 + requiredDisplayedHeight) / _scrollView.ViewportHeight, 
                        index: out firstRelevantItemContainerIndex);
            itemsView.TryGetItemIndex(
                        horizontalViewportRatio: 1.0, 
                        verticalViewportRatio: 1.0 + ((linedFlowLayout.LineHeight + linedFlowLayout.LineSpacing) / 2.0 - requiredDisplayedHeight) / _scrollView.ViewportHeight, 
                        index: out lastRelevantItemContainerIndex);

            string headerText = string.Empty;

            if (firstRelevantItemContainerIndex != -1 && lastRelevantItemContainerIndex != -1)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    if (chkGeneralInfoLogs.IsChecked == true)
                    {
                        Recipe firstRelevantPhoto = _colPhotos[firstRelevantItemContainerIndex];
                        Recipe lastRelevantPhoto = _colPhotos[lastRelevantItemContainerIndex];

                        AppendAsyncEventMessage($"PopulateHeader firstRelevantPhoto={firstRelevantPhoto.Description}, lastRelevantPhoto={lastRelevantPhoto.Description}");
                    }

                    int cumulatedPhotos = 0;
                    int firstYearIndex = -1, lastYearIndex = -1;
                    int firstMonthIndex = -1, lastMonthIndex = -1;

                    for (int yearIndex = 0; yearIndex < _yearCount; yearIndex++)
                    {
                        int monthIndex;

                        for (monthIndex = 0; monthIndex < 12; monthIndex++)
                        {
                            cumulatedPhotos += _monthPhotoCounts[yearIndex, monthIndex];

                            if (cumulatedPhotos > firstRelevantItemContainerIndex && firstYearIndex == -1)
                            {
                                firstYearIndex = yearIndex;
                                firstMonthIndex = monthIndex;
                            }

                            if (cumulatedPhotos > lastRelevantItemContainerIndex)
                            {
                                break;
                            }
                        }

                        if (cumulatedPhotos > lastRelevantItemContainerIndex)
                        {
                            lastYearIndex = yearIndex;
                            lastMonthIndex = monthIndex;
                            break;
                        }
                    }

                    DateTime today = DateTime.Now.Date;
                    int year = today.Year - firstYearIndex;
                    string monthName = GetMonthNameFromIndex(firstMonthIndex);
                    headerText = monthName + " " + year.ToString();

                    if (lastMonthIndex != firstMonthIndex || lastYearIndex != firstYearIndex)
                    {
                        year = today.Year - lastYearIndex;
                        monthName = GetMonthNameFromIndex(lastMonthIndex);
                        headerText += " - " + monthName + " " + year.ToString();
                    }
                }
            }
            else if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage($"PopulateHeader firstRelevantItemContainerIndex={firstRelevantItemContainerIndex}, lastRelevantItemContainerIndex={lastRelevantItemContainerIndex}");
            }

            tblHeader.Text = headerText;
        }

        // Creates an observable collection of photos. Each photo is associated with a Description string 'Month Year - #Number'.
        private void PopulatePhotosCollection()
        {
            _yearPhotoCounts = new int[_yearCount];
            _monthPhotoCounts = new int[_yearCount, 12];

            if (_colPhotos != null)
            {
                UnhookPhotosCollectionChanged();
            }

            _colPhotos = new ObservableCollection<Recipe>();

            var rnd = chkUseConstantMonthCount.IsChecked == true ? null : new Random();
            DateTime today = DateTime.Now.Date;

            for (int yearIndex = 0; yearIndex < _yearCount; yearIndex++)
            {
                int year = today.Year - yearIndex;
                int yearCount = 0;

                for (int monthIndex = 0; monthIndex < 12; monthIndex++)
                {
                    int monthCount;

                    if (chkUseConstantMonthCount.IsChecked == true)
                    {
                        monthCount = Math.Max(1, _averageMonthCount);
                    }
                    else
                    {
                        monthCount = rnd.Next(_averageMonthCount / 10, (19 * _averageMonthCount) / 10 + 1);

                        if (monthIndex == 11 && yearCount == 0 && monthCount == 0)
                        {
                            monthCount = 1;
                        }
                    }

                    _monthPhotoCounts[yearIndex, monthIndex] = monthCount;
                    yearCount += monthCount;

                    for (int photoIndex = 0; photoIndex < monthCount; photoIndex++)
                    {
                        _colPhotos.Add(new Recipe()
                        {
                            ImageUri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", chkUseConstantMonthCount.IsChecked == true ? (yearIndex * 50 + monthIndex * 5 + photoIndex) % 126 + 1 : rnd.Next(1, 127))),
                            Description = GetMonthNameFromIndex(monthIndex) + " " + year + " - #" + photoIndex
                        });
                    }
                }

                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                _yearPhotoCounts[yearIndex] = yearCount;
            }

            PopulateHeaderAsync();
            HookPhotosCollectionChanged();
        }

        // Asynchronously re-populating the AnnotatedScrollBar because ScrollView_ExtentChanged may not be called after items get unlocked.
        private void Photos_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            _populateAnnotatedScrollBarPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateAnnotatedScrollBar);

            PopulateHeaderAsync();
        }

        private void AnnotatedScrollBar_Scrolling(AnnotatedScrollBar sender, AnnotatedScrollBarScrollingEventArgs args)
        {
            try
            { 
                if (itemsView != null && args.ScrollingEventKind == AnnotatedScrollBarScrollingEventKind.Click)
                {
                    // Tell AnnotatedScrollBar to not raise a scroll request
                    args.Cancel = true;

                    // The user clicked the labels area to scroll to a particular offset.
                    int targetItemIndex = GetItemIndexFromLastDetailLabelRequest();

                    // Launch a bring-into-view operation to the target item index.
                    if (_bringIntoViewOptions == null)
                    {
                        _bringIntoViewOptions = new BringIntoViewOptions()
                        {
                            AnimationDesired = false,
                            TargetRect = new Windows.Foundation.Rect(0, 0, 1, _scrollView.ViewportHeight)
                        };
                    }

                    itemsView.StartBringItemIntoView(targetItemIndex, _bringIntoViewOptions);
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        // Handler for providing a tooltip value when the user hovers over the AnnotatedScrollBar.
        private void AnnotatedScrollBar_DetailLabelRequested(AnnotatedScrollBar sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            try
            {
                if (itemsView == null || itemsView.ItemsSource == null)
                {
                    return;
                }

                Debug.Assert(_yearPhotoCounts != null);
                Debug.Assert(_monthPhotoCounts != null);

                double lineAndSpacingSize = linedFlowLayout.ActualLineHeight + linedFlowLayout.LineSpacing;
                double value = args.ScrollOffset;
                double previousLabelValue = 0.0;
                double labelValue = 0.0;
                int cumulatedPhotos = _yearPhotoCounts[0];
                int yearIndex;

                // Phase 1: Find the Label values before and after the provided args.Value.
                for (yearIndex = 1; yearIndex < _yearCount; yearIndex++)
                {
                    int lineIndex = linedFlowLayout.LockItemToLine(cumulatedPhotos);
                    labelValue = lineIndex * lineAndSpacingSize;

                    if (labelValue > value)
                    {
                        break;
                    }

                    previousLabelValue = labelValue;

                    Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                    cumulatedPhotos += _yearPhotoCounts[yearIndex];
                }

                if (yearIndex == _yearCount)
                {
                    labelValue = _scrollView.ExtentHeight;
                }

                Debug.Assert(value >= previousLabelValue);
                Debug.Assert(value <= labelValue);

                // Phase 2: Find the approximate photo index in that resulting year.
                DateTime today = DateTime.Now.Date;
                int yearPhotoIndex = 0;
                int monthIndex;
                int year = today.Year - yearIndex + 1;

                Debug.Assert(yearIndex - 1 < _yearPhotoCounts.Length);
                int yearPhotoCount = _yearPhotoCounts[yearIndex - 1];

                double yearValueSpan = labelValue - previousLabelValue;
                int approximatePhotoInYear = (int)(yearPhotoCount * (value - previousLabelValue) / yearValueSpan);

                Debug.Assert(approximatePhotoInYear >= 0);

                // Phase3: Find the month index for the resulting approximate photo index.
                for (monthIndex = 0; monthIndex < 12; monthIndex++)
                {
                    yearPhotoIndex += _monthPhotoCounts[yearIndex - 1, monthIndex];

                    if (yearPhotoIndex > approximatePhotoInYear)
                    {
                        break;
                    }
                }

                // Phase 4: Get the month name from the resulting month index.
                string monthName = GetMonthNameFromIndex(monthIndex);

                // Phase 5: Provide a sub-label string.
                args.Content = monthName + " " + year.ToString();
                // Caching information for the case an AnnotatedScrollBar_Scrolling event is raised for this args.Value.
                _requestedDetailLabelYearIndex = yearIndex - 1;
                _requestedDetailLabelPhotoInYearIndex = approximatePhotoInYear;

                if (chkLogAnnotatedScrollBarEvents.IsChecked == true)
                {
                    AppendAsyncEventMessage($"AnnotatedScrollBar_DetailLabelRequested args.ScrollOffset={args.ScrollOffset}, _requestedDetailLabelYearIndex={_requestedDetailLabelYearIndex}, _requestedDetailLabelPhotoInYearIndex={_requestedDetailLabelPhotoInYearIndex}");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void AnnotatedScrollBar_SizeChanged(object sender, object args)
        {
            try
            {
                _populateAnnotatedScrollBarPending++;
                PopulateAnnotatedScrollBar();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        // Raised to give the application code a chance to provide items sizing information to minimize the number of UI elements being realized.
        // - args.ItemsRangeStartIndex: indicates the index of the first item in the source collection for which sizing information is requested.
        // - args.ItemsRangeRequestedLength: indicates the number of items in the source collection for which sizing information is requested.
        //   For example, the numbers may be ItemsRangeStartIndex=50 and ItemsRangeRequestedLength=80, indicating that information is requested for the [50, 129] item range.
        // - args.SetDesiredAspectRatios: if the app code decides to provide information at all, it must call this method. It allows to provide an array of aspect ratios. 
        // - args.SetMinWidths: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide heterogeneous minimum sizing information for the same items.
        // - args.SetMaxWidths: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide heterogeneous maximum sizing information for the same items.
        // - args.MinWidth: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide homogeneous minimum sizing information for the same items. 
        // - args.MaxWidth: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide homogeneous maximum sizing information for the same items.
        //   Using MinWidth is more efficient than using SetMinWidths, but both SetMinWidths and MinWidth can be used at the same time. The greatest of the two values is used in that
        //   case. Likewise, SetMaxWidths and MaxWidth can be used at the same time. Not using SetMaxWidths and MaxWidth is best because it gives more freedom to the LinedFlowLayout.
        // A few points:
        // - providing sizing information for fewer items than the requested range would be pointless because it would be unused.
        // - providing minimum or maximun sizing information without also providing aspect ratios would be pointless because it would be unused.
        // - app code can:
        //   * not provide any sizing information at all (Do not hook up to this event at all then).
        //   * provide sizing information for the requested range only.
        //   * provide sizing information for more items than the requested range. In order to do that, app code can set args.ItemsRangeStartIndex to a smaller number
        //     and/or provide aspect ratios for more items than args.ItemsRangeRequestedLength.
        // - the more items are covered in an ItemsInfoRequested response, the less likely the event is going to be raised again to retrieve more information.
        // - the information provided by the ItemsInfoRequested handler must reflect reality. It must not mislead the LinedFlowLayout. 
        //   If an item's natural aspect ratio is 1.75, the handler must return that value.
        //   If an item's MinWidth is 48px, the handler must return that value.
        //   If an item's MaxWidth is 480px, the handler must return that value. It is preferable to not use max widths at all though and thus not use SetMaxWidths/MaxWidth.
        // - app code can dynamically switch between not providing information, providing the requested information, and providing more information, each time the event is raised.
        // - when providing sizing information for the entire source collection, the LinedFlowLayout uses a simpler/faster layout algorithm. That simpler algorithm lays out the entire collection
        //   in a single path. It uses more memory than the regular layout though because it caches the resulting width of all items and item count of all lines.
        //   As the source collection grows, the simpler algo becomes less efficient than the regular one, but currently it is unknown where the transition point is. A perf analysis is pending.
        private void LinedFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage($"LinedFlowLayout_ItemsInfoRequested - args.ItemsRangeStartIndex={args.ItemsRangeStartIndex}, args.ItemsRangeRequestedLength={args.ItemsRangeRequestedLength}");
            }

            double minWidth = 0.0;

            switch (cmbItemTemplate.SelectedIndex)
            {
                case 0:
                    minWidth = 48.0;
                    break;
                case 2:
                case 5:
                case 6:
                case 7:
                    minWidth = 72.0;
                    break;
                case 1:
                    minWidth = 76.0;
                    break;
                case 3:
                    minWidth = 96.0;
                    break;
                case 4:
                    minWidth = 146.0;
                    break;
            }

            args.MinWidth = minWidth;

            double[] desiredAspectRatios = new double[chkUseFastPath.IsChecked == true ? _colPhotos.Count : args.ItemsRangeRequestedLength];

            if (chkUseFastPath.IsChecked == true)
            {
                args.ItemsRangeStartIndex = 0;

                for (int index = 0; index < _colPhotos.Count; index++)
                {
                    desiredAspectRatios[index] = _colPhotos[index].AspectRatio;
                }
            }
            else
            {
                for (int index = 0; index < args.ItemsRangeRequestedLength; index++)
                {
                    desiredAspectRatios[index] = _colPhotos[args.ItemsRangeStartIndex + index].AspectRatio;
                }
            }

            args.SetDesiredAspectRatios(desiredAspectRatios);
        }

        // Raised when the average-items-per-line value changed and the AnnotatedScrollBar's Labels collection needs to be re-populated.
        private void LinedFlowLayout_ItemsUnlocked(LinedFlowLayout sender, object args)
        {
            if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage("LinedFlowLayout_ItemsUnlocked");
            }

            // Declare the re-population need for when the ScrollView extent was changed based on the new average-items-per-line value.
            // Note: Not expecting this to be the final way to re-populate the AnnotatedScrollBar. Looking into a more reliable/easy way
            // because average-items-per-line changes may not result in extent changes.
            // At the moment, LockItemToLine cannot be called within this handler.
            _populateAnnotatedScrollBar = true;
        }

        // Queues up a refresh of the displayed range caption,
        // Re-populates the AnnotatedScrollBar when required.
        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollView_ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}, ScrollableWidth={sender.ScrollableWidth}, ScrollableHeight={sender.ScrollableHeight}");
            }

            PopulateHeaderAsync();

            if (_populateAnnotatedScrollBar)
            {
                _populateAnnotatedScrollBar = false;
                _populateAnnotatedScrollBarPending++;
                PopulateAnnotatedScrollBar();
            }
        }

        private void ScrollView_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            if (_bringIntoViewOptions != null)
            {
                // Sizing the TargetRect in order to bring the target item to the top of the viewport.
                _bringIntoViewOptions.TargetRect = new Windows.Foundation.Rect(0, 0, 1, _scrollView.ViewportHeight);
            }
        }

        private void ScrollView_ViewChanged(ScrollView sender, object args)
        {
            PopulateHeaderAsync();
        }

        // Queues up a refresh of the displayed range caption
        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            if (chkGeneralInfoLogs.IsChecked == true)
            {
                AppendAsyncEventMessage($"ScrollView_ScrollCompleted CorrelationId={args.CorrelationId}");
            }

            PopulateHeaderAsync();
        }

        private void RepopulateAndApplyPhotosCollection()
        {
            try
            {
                bool isItemsSourceSet = itemsView.ItemsSource == _colPhotos;

                PopulatePhotosCollection();

                if (isItemsSourceSet)
                {
                    itemsView.ItemsSource = _colPhotos;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void UpdateItemsInfoRequestedHandler()
        {
            UnhookLinedFlowLayoutEvents(justItemsInfoRequested: true, exceptItemsInfoRequested: false);

            if (chkHandleItemsInfoRequested.IsChecked == true || chkUseFastPath.IsChecked == true)
            {
                HookLinedFlowLayoutEvents(justItemsInfoRequested: true, exceptItemsInfoRequested: false);
            }
        }

        private void ChkCustomUI_Checked(object sender, RoutedEventArgs args)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Visible;
        }

        private void ChkCustomUI_Unchecked(object sender, RoutedEventArgs args)
        {
            if (grdCustomUI != null)
                grdCustomUI.Visibility = Visibility.Collapsed;
        }

        private void ChkPageMethods_Checked(object sender, RoutedEventArgs e)
        {
            if (grdPageMethods != null)
                grdPageMethods.Visibility = Visibility.Visible;
        }

        private void ChkPageMethods_Unchecked(object sender, RoutedEventArgs e)
        {
            if (grdPageMethods != null)
                grdPageMethods.Visibility = Visibility.Collapsed;
        }

        private void ChkLogs_Checked(object sender, RoutedEventArgs args)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Visible;
        }

        private void ChkLogs_Unchecked(object sender, RoutedEventArgs args)
        {
            if (grdLogs != null)
                grdLogs.Visibility = Visibility.Collapsed;
        }

        private void AnnotatedScrollBar_ScrollingLog(AnnotatedScrollBar sender, AnnotatedScrollBarScrollingEventArgs args)
        {
            AppendAsyncEventMessage($"AnnotatedScrollBar.Scrolling ScrollingEventKind={args.ScrollingEventKind}, ScrollOffset={args.ScrollOffset}");
        }

        private void AnnotatedScrollBar_DetailLabelRequestedLog(AnnotatedScrollBar sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            AppendAsyncEventMessage($"AnnotatedScrollBar.DetailLabelRequested ScrollOffset={args.ScrollOffset}");
        }

        private void AnnotatedScrollBar_ValueFromScrollOffsetRequestedLog(AnnotatedScrollBar sender, AnnotatedScrollBarValueFromScrollOffsetRequestedEventArgs args)
        {
            AppendAsyncEventMessage($"AnnotatedScrollBar.ValueFromScrollOffsetRequested ScrollOffset={args.ScrollOffset}");
        }

        private void Image_ImageOpened(object sender, RoutedEventArgs e)
        {
            // When the Image has a TextBlock sibling, set its Foreground to White to indicate the image loading success.
            TextBlock textBlock = null;
            Image image = sender as Image;

            if (image != null)
            {
                Panel itemPanel = image.Parent as Panel;

                if (itemPanel != null && itemPanel.Name == "itemPanel" && itemPanel.Children.Count >= 2)
                {
                    textBlock = itemPanel.Children[1] as TextBlock;

                    if (textBlock != null)
                    {
                        textBlock.Foreground = _whiteBrush;
                    }
                }
            }

            // This event is too chatty so its reporting is turned off by default.
            //if (chkLogImageEvents.IsChecked == true)
            //{
            //    AppendAsyncEventMessage("Image_ImageOpened, Item Id={textBlock?.Text}");
            //}
        }

        private void Image_ImageFailed(object sender, ExceptionRoutedEventArgs e)
        {
            // When the Image has a TextBlock sibling, set its Foreground to Red to indicate the image loading failure.
            TextBlock textBlock = null;
            Image image = sender as Image;

            if (image != null)
            {
                Panel itemPanel = image.Parent as Panel;

                if (itemPanel != null && itemPanel.Name == "itemPanel" && itemPanel.Children.Count >= 2)
                {
                    textBlock = itemPanel.Children[1] as TextBlock;

                    if (textBlock != null)
                    {
                        textBlock.Foreground = _redBrush;
                    }
                }
            }

            if (chkLogImageEvents.IsChecked == true)
            {
                string uri = string.Empty;

                if (image != null)
                {
                    BitmapImage bitmapImage = image.Source as BitmapImage;

                    if (bitmapImage != null)
                    {
                        Uri uriSource = bitmapImage.UriSource;

                        if (uriSource != null)
                        {
                            uri = uriSource.ToString();
                        }
                    }
                }

                AppendAsyncEventMessage($"Image_ImageFailed ErrorMessage={e.ErrorMessage}, Item Id={textBlock?.Text}, Item Uri={uri}");
            }
        }

        private void ItemContainer_DragStartingLog(UIElement sender, DragStartingEventArgs args)
        {
            AppendAsyncEventMessage("ItemContainer.DragStarting");
        }

        private void ItemsView_LoadedLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ItemsView.Loaded");
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

        private void ItemsView_SizeChangedLog(object sender, SizeChangedEventArgs args)
        {
            AppendAsyncEventMessage($"ItemsView.SizeChanged PreviousSize={args.PreviousSize}, NewSize={args.NewSize}, ActualSize={itemsView.ActualWidth} x {itemsView.ActualHeight}");

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

        private void ItemsView_GettingFocusLog(UIElement sender, Microsoft.UI.Xaml.Input.GettingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ItemsView.GettingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_LostFocusLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ItemsView.LostFocus");
        }

        private void ItemsView_LosingFocusLog(UIElement sender, Microsoft.UI.Xaml.Input.LosingFocusEventArgs args)
        {
            FrameworkElement oldFE = args.OldFocusedElement as FrameworkElement;
            string oldFEName = (oldFE == null) ? "null" : oldFE.Name;
            FrameworkElement newFE = args.NewFocusedElement as FrameworkElement;
            string newFEName = (newFE == null) ? "null" : newFE.Name;

            AppendAsyncEventMessage($"ItemsView.LosingFocus FocusState={args.FocusState}, Direction={args.Direction}, InputDevice={args.InputDevice}, oldFE={oldFEName}, newFE={newFEName}");
        }

        private void ItemsView_GotFocusLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ItemsView.GotFocus");
        }

        private void ItemsRepeater_LayoutUpdatedLog(object sender, object args)
        {
            if (chkGeneralVerboseLogs.IsChecked == true)
            {
                AppendAsyncEventMessage("ItemsRepeater.LayoutUpdated");
            }
        }

        private void ItemsRepeater_LoadedLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ItemsRepeater.Loaded");
            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ItemsRepeater_SizeChangedLog(object sender, SizeChangedEventArgs args)
        {
            AppendAsyncEventMessage($"ItemsRepeater.SizeChanged PreviousSize={args.PreviousSize}, NewSize={args.NewSize}, ActualSize={(sender as FrameworkElement).ActualWidth} x {(sender as FrameworkElement).ActualHeight}");

            LogItemsRepeaterInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void LinedFlowLayout_ItemsUnlockedLog(LinedFlowLayout sender, object args)
        {
            AppendAsyncEventMessage("LinedFlowLayout.ItemsUnlocked");
        }

        private void LinedFlowLayout_ArrangeInvalidatedLog(Layout sender, object args)
        {
            AppendAsyncEventMessage("LinedFlowLayout.ArrangeInvalidated");
        }

        private void LinedFlowLayout_MeasureInvalidatedLog(Layout sender, object args)
        {
            AppendAsyncEventMessage("LinedFlowLayout.MeasureInvalidated");
        }

        private void ScrollController_AddScrollVelocityRequestedLog(IScrollController sender, ScrollControllerAddScrollVelocityRequestedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollController.AddScrollVelocityRequested CorrelationId={args.CorrelationId}, OffsetVelocity={args.OffsetVelocity}");
        }

        private void ScrollController_ScrollToRequestedLog(IScrollController sender, ScrollControllerScrollToRequestedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollController.ScrollToRequested CorrelationId={args.CorrelationId}, Offset={args.Offset}, AnimationMode={args.Options.AnimationMode}");
        }

        private void ScrollController_ScrollByRequestedLog(IScrollController sender, ScrollControllerScrollByRequestedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollController.ScrollByRequested CorrelationId={args.CorrelationId}, Offset={args.OffsetDelta}, AnimationMode={args.Options.AnimationMode}");
        }

        private void ScrollPresenter_LoadedLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ScrollPresenter.Loaded");
        }

        private void ScrollPresenter_SizeChangedLog(object sender, SizeChangedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.SizeChanged PreviousSize={args.PreviousSize}, NewSize={args.NewSize}, ActualSize={(sender as FrameworkElement).ActualWidth} x {(sender as FrameworkElement).ActualHeight}");
        }

        private void ScrollPresenter_ExtentChangedLog(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}, ScrollableWidth={sender.ScrollableWidth}, ScrollableHeight={sender.ScrollableHeight}");
        }

        private void ScrollPresenter_StateChangedLog(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.StateChanged {sender.State}");
        }

        private void ScrollPresenter_ViewChangedLog(ScrollPresenter sender, object args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ViewChanged HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollPresenter_ScrollAnimationStartingLog(ScrollPresenter sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ScrollAnimationStarting CorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void ScrollPresenter_ZoomAnimationStartingLog(ScrollPresenter sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ZoomAnimationStarting CorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollPresenter_ScrollCompletedLog(ScrollPresenter sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ScrollCompleted CorrelationId={args.CorrelationId}");
        }

        private void ScrollPresenter_ZoomCompletedLog(ScrollPresenter sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollPresenter.ZoomCompleted CorrelationId={args.CorrelationId}");
        }

        private void ScrollView_LoadedLog(object sender, RoutedEventArgs args)
        {
            AppendAsyncEventMessage("ScrollView.Loaded");
            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_SizeChangedLog(object sender, SizeChangedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.SizeChanged PreviousSize={args.PreviousSize}, NewSize={args.NewSize}, ActualSize={itemsView.ActualWidth} x {itemsView.ActualHeight}");

            LogScrollViewInfo();
            if (chkLogItemsViewEvents.IsChecked == true)
            {
                LogItemsViewInfo();
            }
        }

        private void ScrollView_ExtentChangedLog(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ExtentChanged ExtentWidth={sender.ExtentWidth}, ExtentHeight={sender.ExtentHeight}, ScrollableWidth={sender.ScrollableWidth}, ScrollableHeight={sender.ScrollableHeight}");
        }

        private void ScrollView_StateChangedLog(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.StateChanged {sender.State}");
        }

        private void ScrollView_ViewChangedLog(ScrollView sender, object args)
        {
            AppendAsyncEventMessage($"ScrollView.ViewChanged HorizontalOffset={sender.HorizontalOffset}, VerticalOffset={sender.VerticalOffset}, ZoomFactor={sender.ZoomFactor}");
        }

        private void ScrollView_ScrollAnimationStartingLog(ScrollView sender, ScrollingScrollAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ScrollAnimationStarting CorrelationId={args.CorrelationId}, SP=({args.StartPosition.X}, {args.StartPosition.Y}), EP=({args.EndPosition.X}, {args.EndPosition.Y})");
        }

        private void ScrollView_ZoomAnimationStartingLog(ScrollView sender, ScrollingZoomAnimationStartingEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomAnimationStarting CorrelationId={args.CorrelationId}, CenterPoint={args.CenterPoint}, SZF={args.StartZoomFactor}, EZF={args.EndZoomFactor}");
        }

        private void ScrollView_ScrollCompletedLog(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ScrollCompleted CorrelationId={args.CorrelationId}");
        }

        private void ScrollView_ZoomCompletedLog(ScrollView sender, ScrollingZoomCompletedEventArgs args)
        {
            AppendAsyncEventMessage($"ScrollView.ZoomCompleted CorrelationId={args.CorrelationId}");
        }

        private void LogItemsRepeaterInfo()
        {
            ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

            AppendAsyncEventMessage($"ItemsRepeater Info: ItemsSource={itemsRepeater.ItemsSource}, ItemTemplate={itemsRepeater.ItemTemplate}, Layout={itemsRepeater.Layout}");
        }

        private void LogScrollViewInfo()
        {
            ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

            AppendAsyncEventMessage($"ScrollView Info: HorizontalOffset={scrollView.HorizontalOffset}, VerticalOffset={scrollView.VerticalOffset}, ZoomFactor={scrollView.ZoomFactor}");
            AppendAsyncEventMessage($"ScrollView Info: ViewportWidth={scrollView.ViewportWidth}, ExtentHeight={scrollView.ViewportHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ExtentWidth={scrollView.ExtentWidth}, ExtentHeight={scrollView.ExtentHeight}");
            AppendAsyncEventMessage($"ScrollView Info: ScrollableWidth={scrollView.ScrollableWidth}, ScrollableHeight={scrollView.ScrollableHeight}");
        }

        private void LogItemsViewInfo()
        {
            //AppendAsyncEventMessage($"ItemsView Info: ItemsSource={itemsView.ItemsSource}, ItemTemplate={itemsView.ItemTemplate}, Layout={itemsView.Layout}");
        }

        private void BtnGetAverageMonthCount_Click(object sender, RoutedEventArgs args)
        {
            txtAverageMonthCount.Text = _averageMonthCount.ToString();
        }

        private void BtnSetAverageMonthCount_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                _averageMonthCount = Math.Max(1, Convert.ToInt32(txtAverageMonthCount.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnApplyAverageMonthCount_Click(object sender, RoutedEventArgs args)
        {
            RepopulateAndApplyPhotosCollection();
        }

        private void ChkUseConstantMonthCount_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            try
            {
                PopulatePhotosCollection();
                itemsView.ItemsSource = _colPhotos;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void ChkHandleItemsInfoRequested_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            UpdateItemsInfoRequestedHandler();
        }

        private void ChkUseFastPath_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            UpdateItemsInfoRequestedHandler();
        }

        private void ChkAnnotatedScrollBarIsEnabled_IsCheckedChanged(object sender, RoutedEventArgs args)
        {
            annotatedScrollBar.IsEnabled = (bool)chkAnnotatedScrollBarIsEnabled.IsChecked;
        }

        private void BtnGetFullLog_Click(object sender, RoutedEventArgs e)
        {
            GetFullLog();
        }

        private void BtnClearFullLog_Click(object sender, RoutedEventArgs e)
        {
            ClearFullLog();
        }

        private void BtnGetScrollViewState_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                txtScrollViewState.Text = _scrollView.State.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }


        private void BtnGetScrollViewVerticalOffset_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                txtScrollViewVerticalOffset.Text = _scrollView.VerticalOffset.ToString();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnSetScrollViewVerticalOffset_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                double verticalOffset = Convert.ToDouble(txtScrollViewVerticalOffset.Text);
                int correlationId = _scrollView.ScrollTo(_scrollView.HorizontalOffset, verticalOffset);

                if (chkGeneralInfoLogs.IsChecked == true)
                {
                    AppendAsyncEventMessage($"BtnSetScrollViewVerticalOffset_Click ScrollView.ScrollTo({verticalOffset}), CorrelationId={correlationId}");
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnGetYearCount_Click(object sender, RoutedEventArgs args)
        {
            txtYearCount.Text = _yearCount.ToString();
        }

        private void BtnSetYearCount_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                _yearCount = Math.Max(1, Convert.ToInt32(txtYearCount.Text));
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnApplyYearCount_Click(object sender, RoutedEventArgs args)
        {
            RepopulateAndApplyPhotosCollection();
        }

        private void BtnForcePopulateHeader_Click(object sender, RoutedEventArgs args)
        {
            _populateHeaderPending++;
            PopulateHeader();
        }

        private void BtnSetItemsSource_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                itemsView.ItemsSource = _colPhotos;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnResetItemsSource_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                itemsView.ItemsSource = null;
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnClearItemsSource_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                _yearCount = 0;
                _yearPhotoCounts = null;
                _monthPhotoCounts = null;
                _colPhotos.Clear();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnAddItemsSourceItem_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                int monthIndex = 0;

                if (_colPhotos == null || _yearPhotoCounts == null || _monthPhotoCounts == null)
                {
                    // A collection with a single photo is instantiated.
                    _yearCount = 1;
                    _yearPhotoCounts = new int[_yearCount];
                    _monthPhotoCounts = new int[_yearCount, 12];
                    if (_colPhotos == null)
                    {
                        _colPhotos = new ObservableCollection<Recipe>();

                        HookPhotosCollectionChanged();
                    }
                }
                else
                {
                    int lastYearIndex = _yearCount - 1;
                    int lastPopulatedMonthIndex = 11;
                    int lastPopulatedMonthCount = _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex];

                    while (lastPopulatedMonthCount == 0)
                    {
                        lastPopulatedMonthIndex--;
                        lastPopulatedMonthCount = _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex];
                    }

                    if (lastPopulatedMonthCount >= _averageMonthCount)
                    {
                        if (lastPopulatedMonthIndex == 11)
                        {
                            // Add a new year without photos.
                            int[] yearPhotoCounts = new int[_yearCount + 1];
                            int[,] monthPhotoCounts = new int[_yearCount + 1, 12];

                            for (int yearIndexTmp = 0; yearIndexTmp < _yearCount; yearIndexTmp++)
                            {
                                yearPhotoCounts[yearIndexTmp] = _yearPhotoCounts[yearIndexTmp];
                                
                                for (int monthIndexTmp = 0; monthIndexTmp < 12; monthIndexTmp++)
                                {
                                    monthPhotoCounts[yearIndexTmp, monthIndexTmp] = _monthPhotoCounts[yearIndexTmp, monthIndexTmp];
                                }
                            }

                            _yearCount++;
                            _yearPhotoCounts = yearPhotoCounts;
                            _monthPhotoCounts = monthPhotoCounts;
                        }
                        else
                        {
                            monthIndex = lastPopulatedMonthIndex + 1;
                        }
                    }
                }

                // Adding one photo to December of the last year.
                DateTime today = DateTime.Now.Date;
                int yearIndex = _yearCount - 1;
                int year = today.Year - yearIndex;
                int photoIndex = _monthPhotoCounts[yearIndex, monthIndex];

                _yearPhotoCounts[yearIndex] = _yearPhotoCounts[yearIndex] + 1;
                _monthPhotoCounts[yearIndex, monthIndex] = photoIndex + 1;

                Random rnd = new Random();

                _colPhotos.Add(new Recipe()
                {
                    ImageUri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", rnd.Next(1, 127))),
                    Description = GetMonthNameFromIndex(monthIndex) + " " + year + " - #" + photoIndex
                });
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnRemoveLastItemsSourceItem_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                if (_colPhotos == null || _yearPhotoCounts == null || _monthPhotoCounts == null)
                {
                    return;
                }

                int lastYearIndex = _yearCount - 1;
                int lastPopulatedMonthIndex = 11;
                int lastPopulatedMonthCount = _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex];

                while (lastPopulatedMonthCount == 0)
                {
                    lastPopulatedMonthIndex--;
                    lastPopulatedMonthCount = _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex];
                }

                if (lastPopulatedMonthCount > 1)
                {
                    _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex] = lastPopulatedMonthCount - 1;
                }
                else
                {
                    if (lastPopulatedMonthIndex == 0)
                    {
                        // Remove last year as it is now empty.
                        _yearCount--;

                        if (_yearCount == 0)
                        {
                            _yearPhotoCounts = null;
                            _monthPhotoCounts = null;
                        }
                        else
                        {
                            int[] yearPhotoCounts = new int[_yearCount];
                            int[,] monthPhotoCounts = new int[_yearCount, 12];

                            for (int yearIndexTmp = 0; yearIndexTmp < _yearCount; yearIndexTmp++)
                            {
                                yearPhotoCounts[yearIndexTmp] = _yearPhotoCounts[yearIndexTmp];

                                for (int monthIndexTmp = 0; monthIndexTmp < 12; monthIndexTmp++)
                                {
                                    monthPhotoCounts[yearIndexTmp, monthIndexTmp] = _monthPhotoCounts[yearIndexTmp, monthIndexTmp];
                                }
                            }

                            _yearPhotoCounts = yearPhotoCounts;
                            _monthPhotoCounts = monthPhotoCounts;
                        }
                    }
                    else
                    {
                        _monthPhotoCounts[lastYearIndex, lastPopulatedMonthIndex] = 0;
                    }
                }

                _colPhotos.RemoveAt(_colPhotos.Count - 1);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnParentItemsView_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                gridParent.Children.Insert(0, itemsView);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnUnparentItemsView_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                gridParent.Children.Remove(itemsView);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnUnparentReparentItemsView_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                gridParent.Children.Remove(itemsView);
                gridParent.Children.Insert(0, itemsView);
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnUnparentDiscardItemsView_Click(object sender, RoutedEventArgs args)
        {
            try
            {
                gridParent.Children.Remove(itemsView);
                itemsView = null;

                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void CmbItemTemplate_SelectionChanged(object sender, SelectionChangedEventArgs args)
        {
            try
            {
                if (itemsView != null)
                {
                    int templateIndex = cmbItemTemplate.SelectedIndex;

                    if (_photoTemplates[templateIndex] == null)
                    {
                        _photoTemplates[templateIndex] = Resources["photoTemplate" + (templateIndex + 1).ToString()] as DataTemplate;
                    }

                    switch (templateIndex)
                    {
                        case 0:
                        case 2:
                        case 5:
                        case 6:
                        case 7:
                            linedFlowLayout.LineHeight = 96.0;
                            break;

                        case 1:
                        case 3:
                            linedFlowLayout.LineHeight = 146.0;
                            break;

                        case 4:
                            linedFlowLayout.LineHeight = 296.0;
                            break;
                    }

                    itemsView.ItemTemplate = _photoTemplates[templateIndex];
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void CmbSelectionMode_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            try
            {
                if (itemsView != null && cmbSelectionMode != null && itemsView.SelectionMode != (ItemsViewSelectionMode)cmbSelectionMode.SelectedIndex)
                {
                    itemsView.SelectionMode = (ItemsViewSelectionMode)cmbSelectionMode.SelectedIndex;
                }
            }
            catch (Exception ex)
            {
                txtExceptionReport.Text = ex.ToString();
                AppendAsyncEventMessage(ex.ToString());
            }
        }

        private void BtnClearLogs_Click(object sender, RoutedEventArgs args)
        {
            lstLogs.Items.Clear();
        }

        private void ChkLogAnnotatedScrollBarEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (annotatedScrollBar != null)
            {
                annotatedScrollBar.Scrolling += AnnotatedScrollBar_ScrollingLog;
                annotatedScrollBar.DetailLabelRequested += AnnotatedScrollBar_DetailLabelRequestedLog;
                annotatedScrollBar.ValueFromScrollOffsetRequested += AnnotatedScrollBar_ValueFromScrollOffsetRequestedLog;

                IScrollController scrollController = annotatedScrollBar as IScrollController;

                scrollController.ScrollByRequested += ScrollController_ScrollByRequestedLog;
                scrollController.ScrollToRequested += ScrollController_ScrollToRequestedLog;
                scrollController.AddScrollVelocityRequested += ScrollController_AddScrollVelocityRequestedLog;
            }
        }

        private void ChkLogAnnotatedScrollBarEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (annotatedScrollBar != null)
            {
                annotatedScrollBar.Scrolling -= AnnotatedScrollBar_ScrollingLog;
                annotatedScrollBar.DetailLabelRequested -= AnnotatedScrollBar_DetailLabelRequestedLog;
                annotatedScrollBar.ValueFromScrollOffsetRequested -= AnnotatedScrollBar_ValueFromScrollOffsetRequestedLog;

                IScrollController scrollController = annotatedScrollBar as IScrollController;

                scrollController.ScrollByRequested -= ScrollController_ScrollByRequestedLog;
                scrollController.ScrollToRequested -= ScrollController_ScrollToRequestedLog;
                scrollController.AddScrollVelocityRequested -= ScrollController_AddScrollVelocityRequestedLog;
            }
        }

        private void ChkLogItemsRepeaterEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.LayoutUpdated += ItemsRepeater_LayoutUpdatedLog;
                    itemsRepeater.Loaded += ItemsRepeater_LoadedLog;
                    itemsRepeater.SizeChanged += ItemsRepeater_SizeChangedLog;
                }
            }
        }

        private void ChkLogItemsRepeaterEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ItemsRepeater itemsRepeater = ItemsViewTestHooks.GetItemsRepeaterPart(itemsView);

                if (itemsRepeater != null)
                {
                    itemsRepeater.LayoutUpdated -= ItemsRepeater_LayoutUpdatedLog;
                    itemsRepeater.Loaded -= ItemsRepeater_LoadedLog;
                    itemsRepeater.SizeChanged -= ItemsRepeater_SizeChangedLog;
                }
            }
        }

        private void ChkLogLinedFlowLayoutEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (linedFlowLayout != null)
            {
                linedFlowLayout.ItemsUnlocked += LinedFlowLayout_ItemsUnlockedLog;
                linedFlowLayout.ArrangeInvalidated += LinedFlowLayout_ArrangeInvalidatedLog;
                linedFlowLayout.MeasureInvalidated += LinedFlowLayout_MeasureInvalidatedLog;
            }
        }

        private void ChkLogLinedFlowLayoutEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (linedFlowLayout != null)
            {
                linedFlowLayout.ItemsUnlocked -= LinedFlowLayout_ItemsUnlockedLog;
                linedFlowLayout.ArrangeInvalidated -= LinedFlowLayout_ArrangeInvalidatedLog;
                linedFlowLayout.MeasureInvalidated -= LinedFlowLayout_MeasureInvalidatedLog;
            }
        }

        private void ChkLogScrollPresenterEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);

                    if (scrollPresenter != null)
                    {
                        scrollPresenter.Loaded += ScrollPresenter_LoadedLog;
                        scrollPresenter.SizeChanged += ScrollPresenter_SizeChangedLog;
                        scrollPresenter.ExtentChanged += ScrollPresenter_ExtentChangedLog;
                        scrollPresenter.StateChanged += ScrollPresenter_StateChangedLog;
                        scrollPresenter.ViewChanged += ScrollPresenter_ViewChangedLog;
                        scrollPresenter.ScrollAnimationStarting += ScrollPresenter_ScrollAnimationStartingLog;
                        scrollPresenter.ZoomAnimationStarting += ScrollPresenter_ZoomAnimationStartingLog;
                        scrollPresenter.ScrollCompleted += ScrollPresenter_ScrollCompletedLog;
                        scrollPresenter.ZoomCompleted += ScrollPresenter_ZoomCompletedLog;
                    }
                }
            }
        }

        private void ChkLogScrollPresenterEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    ScrollPresenter scrollPresenter = ScrollViewTestHooks.GetScrollPresenterPart(scrollView);

                    if (scrollPresenter != null)
                    {
                        scrollPresenter.Loaded -= ScrollPresenter_LoadedLog;
                        scrollPresenter.SizeChanged -= ScrollPresenter_SizeChangedLog;
                        scrollPresenter.ExtentChanged -= ScrollPresenter_ExtentChangedLog;
                        scrollPresenter.StateChanged -= ScrollPresenter_StateChangedLog;
                        scrollPresenter.ViewChanged -= ScrollPresenter_ViewChangedLog;
                        scrollPresenter.ScrollAnimationStarting -= ScrollPresenter_ScrollAnimationStartingLog;
                        scrollPresenter.ZoomAnimationStarting -= ScrollPresenter_ZoomAnimationStartingLog;
                        scrollPresenter.ScrollCompleted -= ScrollPresenter_ScrollCompletedLog;
                        scrollPresenter.ZoomCompleted -= ScrollPresenter_ZoomCompletedLog;
                    }
                }
            }
        }

        private void ChkLogScrollViewEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    scrollView.Loaded += ScrollView_LoadedLog;
                    scrollView.SizeChanged += ScrollView_SizeChangedLog;
                    scrollView.ExtentChanged += ScrollView_ExtentChangedLog;
                    scrollView.StateChanged += ScrollView_StateChangedLog;
                    scrollView.ViewChanged += ScrollView_ViewChangedLog;
                    scrollView.ScrollAnimationStarting += ScrollView_ScrollAnimationStartingLog;
                    scrollView.ZoomAnimationStarting += ScrollView_ZoomAnimationStartingLog;
                    scrollView.ScrollCompleted += ScrollView_ScrollCompletedLog;
                    scrollView.ZoomCompleted += ScrollView_ZoomCompletedLog;
                }
            }
        }

        private void ChkLogScrollViewEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                ScrollView scrollView = ItemsViewTestHooks.GetScrollViewPart(itemsView);

                if (scrollView != null)
                {
                    scrollView.Loaded -= ScrollView_LoadedLog;
                    scrollView.SizeChanged -= ScrollView_SizeChangedLog;
                    scrollView.ExtentChanged -= ScrollView_ExtentChangedLog;
                    scrollView.StateChanged -= ScrollView_StateChangedLog;
                    scrollView.ViewChanged -= ScrollView_ViewChangedLog;
                    scrollView.ScrollAnimationStarting -= ScrollView_ScrollAnimationStartingLog;
                    scrollView.ZoomAnimationStarting -= ScrollView_ZoomAnimationStartingLog;
                    scrollView.ScrollCompleted -= ScrollView_ScrollCompletedLog;
                    scrollView.ZoomCompleted -= ScrollView_ZoomCompletedLog;
                }
            }
        }

        private void ChkLogItemsViewEvents_Checked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus += ItemsView_GettingFocusLog;
                itemsView.GotFocus += ItemsView_GotFocusLog;
                itemsView.LosingFocus += ItemsView_LosingFocusLog;
                itemsView.LostFocus += ItemsView_LostFocusLog;
                itemsView.Loaded += ItemsView_LoadedLog;
                itemsView.SizeChanged += ItemsView_SizeChangedLog;
            }
        }

        private void ChkLogItemsViewEvents_Unchecked(object sender, RoutedEventArgs args)
        {
            if (itemsView != null)
            {
                itemsView.GettingFocus -= ItemsView_GettingFocusLog;
                itemsView.GotFocus -= ItemsView_GotFocusLog;
                itemsView.LosingFocus -= ItemsView_LosingFocusLog;
                itemsView.LostFocus -= ItemsView_LostFocusLog;
                itemsView.Loaded -= ItemsView_LoadedLog;
                itemsView.SizeChanged -= ItemsView_SizeChangedLog;
            }
        }

        private void ChkLogItemsRepeaterMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsRepeaterMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsRepeater", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogScrollViewMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollPresenterMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollPresenter", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogScrollViewMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogScrollViewMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ScrollView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogItemsViewMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogItemsViewMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("ItemsView", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogLinedFlowLayoutMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogItemsViewMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogLinedFlowLayoutMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("LinedFlowLayout", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogItemsViewMessages.IsChecked == false &&
                chkLogAnnotatedScrollBarMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogAnnotatedScrollBarMessages_Checked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("AnnotatedScrollBar", isLoggingInfoLevel: true, isLoggingVerboseLevel: true);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogItemsViewMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage += MUXControlsTestHooks_LoggingMessage;
        }

        private void ChkLogAnnotatedScrollBarMessages_Unchecked(object sender, RoutedEventArgs args)
        {
            MUXControlsTestHooks.SetLoggingLevelForType("AnnotatedScrollBar", isLoggingInfoLevel: false, isLoggingVerboseLevel: false);
            if (chkLogScrollViewMessages.IsChecked == false &&
                chkLogScrollPresenterMessages.IsChecked == false &&
                chkLogItemsRepeaterMessages.IsChecked == false &&
                chkLogItemsViewMessages.IsChecked == false &&
                chkLogLinedFlowLayoutMessages.IsChecked == false)
                MUXControlsTestHooks.LoggingMessage -= MUXControlsTestHooks_LoggingMessage;
        }

        private void MUXControlsTestHooks_LoggingMessage(object sender, MUXControlsTestHooksLoggingMessageEventArgs args)
        {
            // Cut off the terminating new line.
            string msg = args.Message.Substring(0, args.Message.Length - 1);
            string asyncEventMessage;
            string senderName = string.Empty;

            while (msg.Contains("0x"))
            {
                int index = msg.IndexOf("0x");

                string toReplace = msg.Substring(index, 10);

                msg = msg.Replace(toReplace, "PTR");
            }

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
                asyncEventMessage = "Verbose: " + senderName + "m:" + msg;
            }
            else
            {
                asyncEventMessage = "Info: " + senderName + "m:" + msg;
            }

            AppendAsyncEventMessage(asyncEventMessage);
        }

        private void AppendAsyncEventMessage(string asyncEventMessage)
        {
            lock (_asyncEventReportingLock)
            {
                while (asyncEventMessage.Length > 0)
                {
                    string msgHead = asyncEventMessage;

                    if (asyncEventMessage.Length > 110)
                    {
                        int commaIndex = asyncEventMessage.IndexOf(',', 110);
                        if (commaIndex != -1)
                        {
                            msgHead = asyncEventMessage.Substring(0, commaIndex);
                            asyncEventMessage = asyncEventMessage.Substring(commaIndex + 1);
                        }
                        else
                        {
                            asyncEventMessage = string.Empty;
                        }
                    }
                    else
                    {
                        asyncEventMessage = string.Empty;
                    }

                    _lstAsyncEventMessage.Add(msgHead);
                }

                var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Normal, AppendAsyncEventMessage);
            }
        }

        private void AppendAsyncEventMessage()
        {
            lock (_asyncEventReportingLock)
            {
                foreach (string asyncEventMessage in _lstAsyncEventMessage)
                {
                    lstLogs.Items.Add(asyncEventMessage);
                    _fullLogs.Add(asyncEventMessage);
                }
                _lstAsyncEventMessage.Clear();
            }
        }

        private void BtnClearExceptionReport_Click(object sender, RoutedEventArgs args)
        {
            txtExceptionReport.Text = string.Empty;
        }

        private void GetFullLog()
        {
            this.txtStatus.Text = "GetFullLog. Populating cmbFullLog...";
            chkLogCleared.IsChecked = false;
            foreach (string log in _fullLogs)
            {
                this.cmbFullLog.Items.Add(log);
            }
            chkLogUpdated.IsChecked = true;
            this.txtStatus.Text = "GetFullLog. Done.";
        }

        private void ClearFullLog()
        {
            this.txtStatus.Text = "ClearFullLog. Clearing cmbFullLog & fullLogs...";
            chkLogUpdated.IsChecked = false;
            _fullLogs.Clear();
            this.cmbFullLog.Items.Clear();
            chkLogCleared.IsChecked = true;
            this.txtStatus.Text = "ClearFullLog. Done.";
        }
    }
}
