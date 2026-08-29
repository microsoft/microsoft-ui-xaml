// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using ItemsViewIntegrationApp.Samples.Model;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using Windows.ApplicationModel.DataTransfer;
using Windows.Storage;
using ElementFactoryGetArgs = Microsoft.UI.Xaml.ElementFactoryGetArgs;

namespace ItemsViewIntegrationApp
{
    public sealed partial class MainWindow : Window
    {
        private ObservableCollection<Photo> _colPhotos = null;
        private ScrollView _scrollView = null;
        private BringIntoViewOptions _bringIntoViewOptions = null;
        private int[] _yearPhotoCounts = null;
        private int[,] _monthPhotoCounts = null;
        private bool _populateAnnotatedScrollBar = true;
        private int _requestedSubLabelYearIndex = 0;
        private int _requestedSubLabelPhotoInYearIndex = 0;
        private int _populateAnnotatedScrollBarPending = 0;
        private int _populateHeaderPending = 0;
        private int _yearCount = 16;
        private int _averageMonthCount = 50;
        private string[] _months = new string[12] { "December", "November", "October", "September", "August", "July", "June", "May", "April", "March", "February", "January" };
        private double _labelSize = 0.0;

        public MainWindow()
        {
            this.InitializeComponent();

            rootGrid.Loaded += ItemsViewIntegrationPage_Loaded;
            PopulatePhotosCollection();
        }

        private void ItemsViewIntegrationPage_Loaded(object sender, RoutedEventArgs args)
        {
            _scrollView = itemsView.GetValue(ItemsView.ScrollViewProperty) as ScrollView;
            _scrollView.HorizontalScrollBarVisibility = ScrollingScrollBarVisibility.Auto;
            _scrollView.VerticalScrollBarVisibility = ScrollingScrollBarVisibility.Hidden;
            _scrollView.ExtentChanged += ScrollView_ExtentChanged;
            _scrollView.ViewChanged += ScrollView_ViewChanged;
            _scrollView.ScrollCompleted += ScrollView_ScrollCompleted;
            _scrollView.SizeChanged += ScrollView_SizeChanged;

            IScrollController scrollController = annotatedScrollBar as IScrollController;
            ScrollPresenter scrollPresenter = _scrollView.GetValue(ScrollView.ScrollPresenterProperty) as ScrollPresenter;

            scrollPresenter.VerticalScrollController = scrollController;

            itemsView.ItemsSource = _colPhotos;

            riverFlowLayout.ItemsInfoRequested += RiverFlowLayout_ItemsInfoRequested;
            riverFlowLayout.ItemsUnlocked += RiverFlowLayout_ItemsUnlocked;

            annotatedScrollBar.Scrolling += AnnotatedScrollBar_Scroll;
            annotatedScrollBar.DetailLabelRequested += AnnotatedScrollBar_SubLabelRequested;
            annotatedScrollBar.SizeChanged += AnnotatedScrollBar_SizeChanged;
        }

        // Gets the item index from the most recent SubLabelRequested event, using
        // the cached _requestedSubLabelYearIndex & _requestedSubLabelPhotoInYearIndex values.
        private int GetItemIndexFromLastSubLabelRequest()
        {
            int cumulatedPhotos = 0;

            for (int yearIndex = 0; yearIndex < _requestedSubLabelYearIndex; yearIndex++)
            {
                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                cumulatedPhotos += _yearPhotoCounts[yearIndex];
            }

            return cumulatedPhotos + _requestedSubLabelPhotoInYearIndex;
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
                // Workaround for AnnotatedScrollBarSubLabelRequestedEventArgs.Value exceeding AnnotatedScrollBar.Maximum + AnnotatedScrollBar.ViewportSize
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

            if (annotatedScrollBar.Labels.Count > 0)
            {
                annotatedScrollBar.Labels.Clear();
            }

            if (_yearPhotoCounts == null || _monthPhotoCounts == null)
            {
                return;
            }

            double labelSize = GetLabelSize();

            double lineAndSpacingSize = riverFlowLayout.ActualLineHeight + riverFlowLayout.LineSpacing;
            int cumulatedPhotos = _yearPhotoCounts[0];
            DateTime today = DateTime.Now.Date;

            AnnotatedScrollBarLabel annotatedScrollBarLabel = new AnnotatedScrollBarLabel(today.Year.ToString(), 0.0);

            annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);

            double previousLabelBottom = labelSize;
            for (int yearIndex = 1; yearIndex < _yearCount; yearIndex++)
            {
                int year = today.Year - yearIndex;
                int lineIndex = riverFlowLayout.LockItemToLine(cumulatedPhotos);
                double value = lineIndex * lineAndSpacingSize;

                double currentLabelTop = value * annotatedScrollBar.ScrollOffsetToLabelOffsetFactor;
                bool doesLabelCollide = currentLabelTop <= previousLabelBottom;
                if (!doesLabelCollide)
                {
                    annotatedScrollBarLabel = new AnnotatedScrollBarLabel(year.ToString(), value);

                    annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);

                    previousLabelBottom = currentLabelTop + labelSize;
                }
                else if (yearIndex == _yearCount - 1)
                {
                    // We always want the last label to be shown.
                    annotatedScrollBar.Labels.RemoveAt(annotatedScrollBar.Labels.Count - 1);

                    annotatedScrollBarLabel = new AnnotatedScrollBarLabel(year.ToString(), value);

                    annotatedScrollBar.Labels.Add(annotatedScrollBarLabel);
                }

                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                cumulatedPhotos += _yearPhotoCounts[yearIndex];
            }
        }

        // Retrieves the first and last relevant ItemContainer instances from the ItemsView and populates the tblHeader TextBlock with the range.
        // 'relevant' means at least 16px are displayed on screen vertically.
        // 'FirstMonth FirstYear - LastMonth LastYear' when range exceeds one month, and
        // 'Month Year' when the range falls within a single month.
        private void PopulateHeader()
        {
            Debug.Assert(_populateHeaderPending > 0);
            _populateHeaderPending--;

            if (_populateHeaderPending > 0)
            {
                return;
            }

            const double requiredDisplayedHeight = 16.0;

            int firstRelevantItemContainerIndex = 0;
            itemsView.TryGetItemIndex(horizontalViewportRatio: 0.0, verticalViewportRatio: ((riverFlowLayout.LineSpacing - riverFlowLayout.LineHeight) / 2.0 + requiredDisplayedHeight) / _scrollView.ViewportHeight, out firstRelevantItemContainerIndex);

            int lastRelevantItemContainerIndex = 0;
            itemsView.TryGetItemIndex(horizontalViewportRatio: 1.0, verticalViewportRatio: 1.0 + ((riverFlowLayout.LineHeight + riverFlowLayout.LineSpacing) / 2.0 - requiredDisplayedHeight) / _scrollView.ViewportHeight, out lastRelevantItemContainerIndex);

            string headerText = string.Empty;

            if (firstRelevantItemContainerIndex != -1 && lastRelevantItemContainerIndex != -1)
            {
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

            tblHeader.Text = headerText;
        }

        // Creates an observable collection of photos.
        private void PopulatePhotosCollection()
        {
            _yearPhotoCounts = new int[_yearCount];
            _monthPhotoCounts = new int[_yearCount, 12];

            if (_colPhotos != null)
            {
                _colPhotos.CollectionChanged -= Photos_CollectionChanged;
            }

            _colPhotos = new ObservableCollection<Photo>();

            _colPhotos.CollectionChanged += Photos_CollectionChanged;

            var rnd = new Random();

            for (int yearIndex = 0; yearIndex < _yearCount; yearIndex++)
            {
                int yearCount = 0;

                for (int monthIndex = 0; monthIndex < 12; monthIndex++)
                {
                    int monthCount = rnd.Next(_averageMonthCount / 10, (19 * _averageMonthCount) / 10 + 1);

                    if (monthIndex == 11 && yearCount == 0 && monthCount == 0)
                    {
                        monthCount = 1;
                    }

                    _monthPhotoCounts[yearIndex, monthIndex] = monthCount;
                    yearCount += monthCount;

                    for (int photoIndex = 0; photoIndex < monthCount; photoIndex++)
                    {
                        _colPhotos.Add(new Photo()
                        {
                            ImageUri = new Uri(string.Format("ms-appx:///Images/vette{0}.jpg", rnd.Next(1, 127)))
                        });
                    }
                }

                Debug.Assert(yearIndex < _yearPhotoCounts.Length);
                _yearPhotoCounts[yearIndex] = yearCount;
            }
        }

        // Asynchronously re-populating the AnnotatedScrollBar and header because not all collection changes trigger a ScrollView event.
        private void Photos_CollectionChanged(object sender, System.Collections.Specialized.NotifyCollectionChangedEventArgs e)
        {
            _populateAnnotatedScrollBarPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateAnnotatedScrollBar);

            _populateHeaderPending++;
            ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateHeader);
        }

        private void AnnotatedScrollBar_Scroll(AnnotatedScrollBar sender, AnnotatedScrollBarScrollingEventArgs args)
        {
            if (args.ScrollingEventKind == AnnotatedScrollBarScrollingEventKind.Click)
            {
                // The user clicked the labels area to scroll to a particular offset.
                int targetItemIndex = GetItemIndexFromLastSubLabelRequest();

                // Overwrite the automatic scrolling the AnnotatedScrollBar launched with a bring-into-view operation to the target item index.
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

        // Handler for providing a tooltip value when the user hovers over the AnnotatedScrollBar.
        private void AnnotatedScrollBar_SubLabelRequested(AnnotatedScrollBar sender, AnnotatedScrollBarDetailLabelRequestedEventArgs args)
        {
            Debug.Assert(_yearPhotoCounts != null);
            Debug.Assert(_monthPhotoCounts != null);


            double lineAndSpacingSize = riverFlowLayout.ActualLineHeight + riverFlowLayout.LineSpacing;
            double value = args.ScrollOffset;
            double previousLabelValue = 0.0;
            double labelValue = 0.0;
            int cumulatedPhotos = _yearPhotoCounts[0];
            int yearIndex;

            // Phase 1: Find the Label values before and after the provided args.Value.
            for (yearIndex = 1; yearIndex < _yearCount; yearIndex++)
            {
                int lineIndex = riverFlowLayout.LockItemToLine(cumulatedPhotos);
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
                labelValue = itemsView.ScrollView.ScrollableHeight + itemsView.ScrollView.ViewportHeight;
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

            // Caching information for the case an AnnotatedScrollBar_Scroll event is raised for this args.Value.
            _requestedSubLabelYearIndex = yearIndex - 1;
            _requestedSubLabelPhotoInYearIndex = approximatePhotoInYear;

            Debug.WriteLine($"AnnotatedScrollBar_SubLabelRequested args.ScrollOffset={args.ScrollOffset}, _requestedSubLabelYearIndex={_requestedSubLabelYearIndex}, _requestedSubLabelPhotoInYearIndex={_requestedSubLabelPhotoInYearIndex}");
        }

        private void AnnotatedScrollBar_SizeChanged(object sender, object args)
        {
            _populateAnnotatedScrollBarPending++;
            PopulateAnnotatedScrollBar();
        }

        // Raised to give the application code a chance to provide items sizing information to minimize the number of UI elements being realized.
        // - args.ItemsRangeStartIndex: indicates the index of the first item in the source collection for which sizing information is requested.
        // - args.ItemsRangeRequestedCount: indicates the number of items in the source collection for which sizing information is requested.
        //   For example, the numbers may be ItemsRangeStartIndex=50 and ItemsRangeRequestedCount=80, indicating that information is requested for the [50, 129] item range.
        // - args.SetDesiredAspectRatios: if the app code decides to provide information at all, it must call this method. It allows to provide an array of aspect ratios. 
        // - args.SetVariedMinSizes: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide heterogeneous minimum sizing information for the same items.
        // - args.SetVariedMaxSizes: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide heterogeneous maximum sizing information for the same items.
        // - args.UniformMinSize: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide homogeneous minimum sizing information for the same items. 
        // - args.UniformMaxSize: when aspect ratios are provided via SetDesiredAspectRatios, the app code can also provide homogeneous maximum sizing information for the same items.
        //   Note that it does not make sense to use SetVariedMinSizes and UniformMinSize. The items either have a uniform min size or not. Using UniformMinSize is more efficient than using SetVariedMinSizes.
        //   Likewise, it does not make sense to use SetVariedMaxSizes and UniformMaxSize. The items either have a uniform max size or not. Using UniformMaxSize is more efficient than using SetVariedMaxSizes.
        //   Not using SetVariedMaxSizes and UniformMaxSize is best because it gives more freedom to the LinedFlowLayout.
        // A few points:
        // - providing sizing information for fewer items than the requested range would be pointless because it would be unused.
        // - providing minimum or maximun sizing information without also providing aspect ratios would be pointless because it would be unused.
        // - app code can:
        //   * not provide any sizing information at all (Do not hook up to this event at all then).
        //   * provide sizing information for the requested range only.
        //   * provide sizing information for more items than the requested range. In order to do that, app code can set args.ItemsRangeStartIndex to a smaller number
        //     and/or provide aspect ratios for more items than args.ItemsRangeRequestedCount.
        // - the more items are covered in an ItemsInfoRequested response, the less likely the event is going to be raised again to retrieve more information.
        // - the information provided by the ItemsInfoRequested handler must reflect reality. It must not mislead the RiverFlowLayout. 
        //   If an item's natural aspect ratio is 1.75, the handler must return that value.
        //   If an item's MinWidth is 48px, the handler must return that value.
        //   If an item's MaxWidth is 480px, the handler must return that value. It is preferable to not use max widths at all though and thus not use SetVariedMaxSizes/UniformMaxSize.
        // - app code can dynamically switch between not providing information, providing the requested information, and providing more information, each time the event is raised.
        // - when providing sizing information for the entire source collection, the RiverFlowLayout uses a simpler/faster layout algorithm. That simpler algorithm lays out the entire collection
        //   in a single path. It uses more memory than the regular layout though because it caches the resulting width of all items and item count of all lines.
        //   As the source collection grows, the simpler algo becomes less efficient than the regular one, but currently it is unknown where the transition point is. A perf analysis is pending.
        //   In the meantime, in this sample and for illustrative purposes, the switch to the regular layout is made when the source collection exceeds 16384 items. 
        private void RiverFlowLayout_ItemsInfoRequested(LinedFlowLayout sender, LinedFlowLayoutItemsInfoRequestedEventArgs args)
        {
            int itemsRangeReturnedStartIndex, itemsRangeReturnedCount;

            if (_colPhotos.Count <= 16384)
            {
                args.ItemsRangeStartIndex = itemsRangeReturnedStartIndex = 0;
                itemsRangeReturnedCount = _colPhotos.Count;
            }
            else
            {
                itemsRangeReturnedStartIndex = args.ItemsRangeStartIndex;
                itemsRangeReturnedCount = args.ItemsRangeRequestedLength;
            }

            double[] desiredAspectRatios = new double[itemsRangeReturnedCount];

            for (int index = 0; index < itemsRangeReturnedCount; index++)
            {
                Photo photo = _colPhotos[itemsRangeReturnedStartIndex + index];

                desiredAspectRatios[index] = photo.AspectRatio;
            }

            args.SetDesiredAspectRatios(desiredAspectRatios);
            args.MinWidth = 100.0;  // Comes from <ItemContainer MinWidth="100" MinHeight="150" Background="LightGray"> in MainWindow.xaml
        }

        // Raised when the average-items-per-line value changed and the AnnotatedScrollBar's Labels collection needs to be re-populated.
        private void RiverFlowLayout_ItemsUnlocked(LinedFlowLayout sender, object args)
        {
            // Declare the re-population need for when the ScrollView extent was changed based on the new average-items-per-line value.
            // Note: Not expecting this to be the final way to re-populate the AnnotatedScrollBar. Looking into a more reliable/easy way
            // because average-items-per-line changes may not result in extent changes.
            // At the moment, LockItem cannot be called within this handler.
            _populateAnnotatedScrollBar = true;
        }

        // Queues up a refresh of the displayed range caption,
        // Re-populates the AnnotatedScrollBar when required.
        private void ScrollView_ExtentChanged(ScrollView sender, object args)
        {
            _populateHeaderPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateHeader);

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
            _populateHeaderPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateHeader);
        }

        // Queues up a refresh of the displayed range caption
        private void ScrollView_ScrollCompleted(ScrollView sender, ScrollingScrollCompletedEventArgs args)
        {
            _populateHeaderPending++;
            var ignored = this.DispatcherQueue.TryEnqueue(Microsoft.UI.Dispatching.DispatcherQueuePriority.Low, PopulateHeader);
        }

        private void SelectionModeComboBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
        { 
            if (itemsView != null && 
                SelectionModeComboBox != null && 
                itemsView.SelectionMode != (ItemsViewSelectionMode)SelectionModeComboBox.SelectedIndex)
            {
                itemsView.SelectionMode = (ItemsViewSelectionMode)SelectionModeComboBox.SelectedIndex;
            }
        }
        private async void ItemContainer_DragStarting(UIElement sender, DragStartingEventArgs args)
        {
            var draggedElement = sender as ItemContainer;
            Photo item = draggedElement.DataContext as Photo;
            var storageItem = await StorageFile.GetFileFromApplicationUriAsync(item.ImageUri);
            args.Data.SetStorageItems(new List<IStorageItem>() { storageItem });
            args.AllowedOperations = DataPackageOperation.Copy;
        }

        private void ItemsView_DragOver(object sender, DragEventArgs e)
        {
            e.AcceptedOperation = DataPackageOperation.Move;
        }

        private async void ItemsView_Drop(object sender, DragEventArgs e)
        {
            if (e.DataView.Contains(StandardDataFormats.StorageItems))
            {
                var storageItems = await e.DataView.GetStorageItemsAsync();
                if (storageItems.Count > 0)
                {
                    var storageItem = storageItems[0];
                    var storageItemPath = storageItem.Path;
                    _colPhotos.Insert(0, new Photo()
                    {
                        ImageUri = new Uri(string.Format(storageItemPath))
                    });
                }
            }
        }
    }
}

