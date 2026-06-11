// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    [TestClass]
    public class MoCoViewportScenarioTests : MoCoViewportTestBase
    {
        private static string _testDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            _testDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void ValidateChatScenarioHorizontal()
        {
            ValidateChatScenario(Orientation.Horizontal, false);
        }

        [TestMethod]
        public void ValidateChatScenarioVertical()
        {
            ValidateChatScenario(Orientation.Vertical, false);
        }

        [TestMethod]
        [TestProperty("Hosting:Mode", "UAP")]
        public void ValidateChatScenarioHorizontalBottomRightAligned()
        {
            ValidateChatScenario(Orientation.Horizontal, true);
        }

        [TestMethod]
        public void ValidateChatScenarioVerticalBottomRightAligned()
        {
            ValidateChatScenario(Orientation.Vertical, true);
        }

        public void ValidateChatScenario(Orientation orientation, bool setBottomRightAlignmentOnPanel)
        {
            string fileName = Path.Combine(_testDeploymentDir, @"resources/managed/enterprise/moco/MaintainViewport/ChatListView.xaml");
            var list = (ListView)MoCoViewportTestBase.LoadXamlTestPage(fileName);

            ItemsStackPanel panel = null;
            ScrollViewer scrollViewer = null;
            ItemsPresenter itemsPresenter = null;
            FrameworkElement header = null;
            FrameworkElement footer = null;

            UIExecutor.Execute(() =>
            {
                panel = (ItemsStackPanel)list.ItemsPanelRoot;
                scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                itemsPresenter = list.FindElementOfTypeInSubtree<ItemsPresenter>();
                header = (FrameworkElement)list.Header;
                footer = (FrameworkElement)list.Footer;
            });

            foreach (bool includeHeaderAndFooter in new bool[] { true, false })
            {
                bool isHorizontal = (orientation == Orientation.Horizontal);
                var data = new ObservableCollection<ListItem>();

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Preparing list.");

                    panel.Orientation = orientation;
                    panel.HorizontalAlignment = setBottomRightAlignmentOnPanel && isHorizontal ? HorizontalAlignment.Right : HorizontalAlignment.Stretch;
                    panel.VerticalAlignment = setBottomRightAlignmentOnPanel && isHorizontal ? VerticalAlignment.Stretch : VerticalAlignment.Bottom;

                    itemsPresenter.HorizontalAlignment = !setBottomRightAlignmentOnPanel && isHorizontal ? HorizontalAlignment.Right : HorizontalAlignment.Stretch;
                    itemsPresenter.VerticalAlignment = !setBottomRightAlignmentOnPanel && isHorizontal ? VerticalAlignment.Stretch : VerticalAlignment.Bottom;


                    scrollViewer.HorizontalScrollMode = isHorizontal ? ScrollMode.Auto : ScrollMode.Disabled;
                    scrollViewer.VerticalScrollMode = isHorizontal ? ScrollMode.Disabled : ScrollMode.Auto;
                    // We don't want the scroll bar's thumb animation to interfer with us.
                    scrollViewer.HorizontalScrollBarVisibility = ScrollBarVisibility.Hidden;
                    scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;

                    if (includeHeaderAndFooter == false)
                    {
                        list.Header = list.Footer = null;
                    }

                    // We will start with one item in the list.
                    // At this point, the list is not scrollable.
                    data.Add(new ListItem { Text = string.Format("Item #{0}", data.Count + 1) });
                    list.ItemsSource = data;
                });

                TestServices.WindowHelper.WaitForIdle();

                // Let's resize the only item in the list such as the list becomes scrollable.
                // We want to validate edge tracking, in this case, that we follow the bottom edge).
                // We will validate tracking of the top edge a bit later.
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Resizing first (and only) item in the list to make list scrollable.");

                    if (isHorizontal) data[0].Width = (list.Width * 2);
                    else data[0].Height = (list.Height * 2);

                    list.UpdateLayout();

                    Verify.IsTrue(
                        (isHorizontal && scrollViewer.HorizontalOffset == scrollViewer.ExtentWidth - scrollViewer.ViewportWidth) ||
                        (!isHorizontal && scrollViewer.VerticalOffset == scrollViewer.ExtentHeight - scrollViewer.ViewportHeight));

                    data[0].Width = data[0].Height = double.NaN;

                    list.UpdateLayout();

                    Verify.AreEqual(0, scrollViewer.HorizontalOffset);
                    Verify.AreEqual(0, scrollViewer.VerticalOffset);
                });

                TestServices.WindowHelper.WaitForIdle();

                // Add items at the end one-by-one until list is scrollable.
                // This will validate that, when at the very end, we can track the last element.
                {
                    Log.Comment("Adding items at the end until list becomes scrollable.");

                    // Validating storyboards for the first add.
                    {
                        // Unfortunately, the fast mutation logic is hard coded on one second
                        // and if we change the data source before that, transitions won't play.
                        global::System.Threading.Tasks.Task.Delay(1000).Wait();

                        int storyboardCounter = 0;
                        var storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
                        {
                            if (storyboardCounter == 0)
                            {
                                Verify.AreEqual(data[0], ((ListViewItem)target).Content);
                                Verify.AreEqual(2, storyboard.Children.Count);

                                // Validating horizontal animation
                                {
                                    var horizontalAnimation = (DoubleAnimationUsingKeyFrames)storyboard.Children[0];
                                    Verify.AreEqual("(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateX", Storyboard.GetTargetProperty(horizontalAnimation));

                                    Verify.AreEqual(2, horizontalAnimation.KeyFrames.Count);
                                    Verify.AreEqual(isHorizontal ? ((FrameworkElement)list.ContainerFromIndex(0)).ActualWidth : 0, horizontalAnimation.KeyFrames[0].Value);
                                    Verify.AreEqual(0, horizontalAnimation.KeyFrames[1].Value);
                                }

                                // Validating vertical animation
                                {
                                    var verticalAnimation = (DoubleAnimationUsingKeyFrames)storyboard.Children[1];
                                    Verify.AreEqual("(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY", Storyboard.GetTargetProperty(verticalAnimation));

                                    Verify.AreEqual(2, verticalAnimation.KeyFrames.Count);
                                    Verify.AreEqual(isHorizontal ? 0 : ((FrameworkElement)list.ContainerFromIndex(0)).ActualHeight, verticalAnimation.KeyFrames[0].Value);
                                    Verify.AreEqual(0, verticalAnimation.KeyFrames[1].Value);
                                }
                            }
                            else
                            {
                                Verify.AreEqual(data[1], ((ListViewItem)target).Content);
                                // No need to validate the fade in animation. We only care about the translation animation
                                // because that's what we adjust to give the impression the list is growing
                                // from the bottom/right.
                            }

                            ++storyboardCounter;
                        });

                        StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;
                        UIExecutor.Execute(() =>
                        {
                            data.Add(new ListItem { Text = string.Format("Item #{0}", data.Count + 1) });
                        });
                        TestServices.WindowHelper.WaitForIdle();
                        StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;

                        Verify.AreEqual(2, storyboardCounter);
                    }

                    UIExecutor.Execute(() =>
                    {
                        while ((isHorizontal && scrollViewer.ScrollableWidth < 100) ||
                              (!isHorizontal && scrollViewer.ScrollableHeight < 100))
                        {
                            data.Add(new ListItem { Text = string.Format("Item #{0}", data.Count + 1) });
                            list.UpdateLayout();

                            // Validate we are always at the very end.
                            Verify.IsTrue(
                                (isHorizontal && scrollViewer.HorizontalOffset == Math.Max(scrollViewer.ExtentWidth, scrollViewer.ViewportWidth) - scrollViewer.ViewportWidth) ||
                                (!isHorizontal && scrollViewer.VerticalOffset == Math.Max(scrollViewer.ExtentHeight, scrollViewer.ViewportHeight) - scrollViewer.ViewportHeight));
                        }
                    });
                }

                TestServices.WindowHelper.WaitForIdle();

                int trackedElementIndex = data.Count - 1;
                double smallOffsetToTrackedElement = 5;

                // Scroll up/left slightly such as we are no longer at the very end.
                // And then add an item at the very end. Viewport should not change.
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Verify we don't track items added after the last visible element.");

                    scrollViewer.ChangeView(
                        isHorizontal ? scrollViewer.HorizontalOffset - (includeHeaderAndFooter ? footer.Width : 0.0) - smallOffsetToTrackedElement : (double?)null,
                        isHorizontal ? (double?)null : scrollViewer.VerticalOffset - (includeHeaderAndFooter ? footer.Height : 0.0) - smallOffsetToTrackedElement,
                        zoomFactor: null,
                        disableAnimation: true);

                    list.UpdateLayout();

                    double expectedHorizontalOffset = scrollViewer.HorizontalOffset;
                    double expectedVerticalOffset = scrollViewer.VerticalOffset;

                    data.Add(new ListItem { Text = string.Format("Item #{0}", data.Count + 1) });
                    list.UpdateLayout();

                    Verify.AreEqual(expectedHorizontalOffset, scrollViewer.HorizontalOffset);
                    Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);
                });

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Bulk add before the tracked element, verify viewport follows it.");

                    for (int i = 0; i < 100; ++i) data.Insert(trackedElementIndex, new ListItem { Text = string.Format("Item #{0}", data.Count + 1) });
                    trackedElementIndex += 100;

                    list.UpdateLayout();

                    var container = (FrameworkElement)list.ContainerFromIndex(trackedElementIndex);
                    var transform = container.TransformToVisual(scrollViewer);
                    var relativeBounds = transform.TransformBounds(new Rect(0, 0, container.ActualWidth, container.ActualHeight));

                    Verify.IsTrue(
                        (isHorizontal && Math.Round(relativeBounds.Right) == scrollViewer.ViewportWidth + smallOffsetToTrackedElement) ||
                        (!isHorizontal && Math.Round(relativeBounds.Bottom) == scrollViewer.ViewportHeight + smallOffsetToTrackedElement));
                });

                // Resize the tracked element and make sure the viewport doesn't change.
                // This is because we are now tracking the top edge (remember we moved the viewport by smallOffsetToTrackedElement).
                UIExecutor.Execute(() =>
                {
                    Log.Comment("Resizing the tracked element.");

                    double expectedHorizontalOffset = scrollViewer.HorizontalOffset;
                    double expectedVerticalOffset = scrollViewer.VerticalOffset;
                    data[trackedElementIndex].Width = data[trackedElementIndex].Height = 250;

                    list.UpdateLayout();

                    Verify.AreEqual(expectedHorizontalOffset, scrollViewer.HorizontalOffset);
                    Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);
                    data[trackedElementIndex].Width = data[trackedElementIndex].Height = double.NaN;
                });

                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    double sizeDelta = 100.0;

                    Log.Comment("Change the window size and validate we track the last visible element.");
                    {

                        if (isHorizontal) list.Width -= sizeDelta;
                        else list.Height -= sizeDelta;

                        double expectedHorizontalOffset = isHorizontal ? scrollViewer.HorizontalOffset + sizeDelta : scrollViewer.HorizontalOffset;
                        double expectedVerticalOffset = isHorizontal ? scrollViewer.VerticalOffset : scrollViewer.VerticalOffset + sizeDelta;

                        list.UpdateLayout();

                        Verify.AreEqual(expectedHorizontalOffset, scrollViewer.HorizontalOffset);
                        Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);
                    }

                    Log.Comment("Change the window size again but InvalidateMeasure on the panel. And then validate we track the last visible element.");
                    {
                        if (isHorizontal) list.Width += sizeDelta;
                        else list.Height += sizeDelta;

                        double expectedHorizontalOffset = isHorizontal ? scrollViewer.HorizontalOffset - sizeDelta : scrollViewer.HorizontalOffset;
                        double expectedVerticalOffset = isHorizontal ? scrollViewer.VerticalOffset : scrollViewer.VerticalOffset - sizeDelta;

                        list.ItemsPanelRoot.InvalidateMeasure();
                        list.UpdateLayout();

                        Verify.AreEqual(expectedHorizontalOffset, scrollViewer.HorizontalOffset);
                        Verify.AreEqual(expectedVerticalOffset, scrollViewer.VerticalOffset);
                    }
                });
            }
        }

        // In the chat scenario, the panel or ItemsPresenter are bottom/right aligned and we track the last
        // element. In the logging scenario, we still track the last element but the panel and ItemsPresenter are top
        // aligned.
        [TestMethod]
        public void ValidateLoggingScenario()
        {
            ListView list = null;
            ItemsStackPanel panel = null;
            ScrollViewer scrollViewer = null;
            var listLoaded = new AutoResetEvent(false);
            ObservableCollection<string> data = null;

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.Loaded += delegate
                {
                    listLoaded.Set();
                    panel = (ItemsStackPanel)list.ItemsPanelRoot;
                    scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            var prepareList = new Action<Orientation, bool>((o, includeHeadersAndFooters) =>
            {
                panel.Orientation = o;
                panel.ItemsUpdatingScrollMode = ItemsUpdatingScrollMode.KeepLastItemInView;

                list.ItemsSource = data = new ObservableCollection<string>() { string.Format("Item #1") };

                list.Width = (o == Orientation.Horizontal) ? 500 : double.NaN;
                list.Height = (o == Orientation.Vertical) ? 500 : double.NaN;

                scrollViewer.HorizontalScrollMode = (o == Orientation.Horizontal) ? ScrollMode.Auto : ScrollMode.Disabled;
                scrollViewer.VerticalScrollMode = (o == Orientation.Horizontal) ? ScrollMode.Disabled : ScrollMode.Auto;
                scrollViewer.HorizontalScrollBarVisibility = scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;

                list.Header = includeHeadersAndFooters ? new TextBlock { Text = "Header", Height = 60, Width = 70 } : null;
                list.Footer = includeHeadersAndFooters ? new TextBlock { Text = "Footer", Height = 80, Width = 90 } : null;
            });

            foreach (bool includeHeaderAndFooter in new bool[] { false, true })
                foreach (Orientation orientation in Enum.GetValues(typeof(Orientation)))
                {
                    bool isHorizontal = (orientation == Orientation.Horizontal);
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Preparing list.");
                        prepareList(orientation, includeHeaderAndFooter);
                    });

                    TestServices.WindowHelper.WaitForIdle();
                    // Unfortunately, the fast mutation logic is hard coded on one second
                    // and if we change the data source before that, transitions won't play.
                    global::System.Threading.Tasks.Task.Delay(1000).Wait();

                    int storyboardCounter = 0;
                    var storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
                    {
                        if (storyboardCounter == 0) Verify.AreEqual(list.ContainerFromIndex(data.Count - 1), target);
                        ++storyboardCounter;
                    });

                    StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Adding one item at the end of a non-scrollable list to validate storyboards.");
                        data.Add(string.Format("Item #{0}", data.Count + 1));
                    });
                    TestServices.WindowHelper.WaitForIdle();
                    StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;
                    Verify.AreEqual(1, storyboardCounter);
                    storyboardCounter = 0;

                    // Add items at the end one-by-one until list is scrollable.
                    // This will validate that, when at the very end, we can track the last element.
                    {
                        Log.Comment("Adding items at the end until list becomes scrollable.");
                        UIExecutor.Execute(() =>
                        {
                            while ((isHorizontal && scrollViewer.ScrollableWidth < 100) ||
                                  (!isHorizontal && scrollViewer.ScrollableHeight < 100))
                            {
                                data.Add(string.Format("Item #{0}", data.Count + 1));
                                list.UpdateLayout();

                            // Validate we are always at the very end.
                            Verify.IsTrue(
                                    (isHorizontal && scrollViewer.HorizontalOffset == scrollViewer.ExtentWidth - scrollViewer.ViewportWidth) ||
                                    (!isHorizontal && scrollViewer.VerticalOffset == scrollViewer.ExtentHeight - scrollViewer.ViewportHeight));
                            }
                        });
                    }

                    TestServices.WindowHelper.WaitForIdle();
                    // Unfortunately, the fast mutation logic is hard coded on one second
                    // and if we change the data source before that, transitions won't play.
                    global::System.Threading.Tasks.Task.Delay(1000).Wait();
                    storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
                    {
                    // We don't animate containers outside of the visible window (< panel.FirstVisibleIndex)
                    var targetIndex = storyboardCounter + panel.FirstVisibleIndex;
                        Verify.AreEqual(list.ContainerFromIndex(targetIndex), target);
                        if (targetIndex < data.Count - 1)
                        {
                            var verticalAnimation = (DoubleAnimationUsingKeyFrames)storyboard.Children[1];
                            Verify.AreEqual("(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY", Storyboard.GetTargetProperty(verticalAnimation));

                            Verify.AreEqual(2, verticalAnimation.KeyFrames.Count);
                            Verify.AreEqual(isHorizontal ? 0 : ((FrameworkElement)list.ContainerFromIndex(targetIndex)).ActualHeight, verticalAnimation.KeyFrames[0].Value);
                            Verify.AreEqual(0, verticalAnimation.KeyFrames[1].Value);
                        }
                        ++storyboardCounter;
                    });
                    StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;
                    UIExecutor.Execute(() =>
                    {
                        Log.Comment("Adding one item at the end of a scrollable list to validate storyboards.");
                        data.Add(string.Format("Item #{0}", data.Count + 1));
                    });
                    TestServices.WindowHelper.WaitForIdle();
                    StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;
                    UIExecutor.Execute(() =>
                    {
                        Verify.AreEqual(data.Count - panel.FirstVisibleIndex, storyboardCounter);
                    });
                }
        }

        public class ListItem : INotifyPropertyChanged
        {
            private double _width = double.NaN;
            private double _height = double.NaN;

            public string Text { get; set; }
            public double Width { get { return _width; } set { _width = value; OnPropertyChanged("Width"); } }
            public double Height { get { return _height; } set { _height = value; OnPropertyChanged("Height"); } }

            public event PropertyChangedEventHandler PropertyChanged;
            public void OnPropertyChanged(string propertyName)
            {
                if (PropertyChanged != null) PropertyChanged(this, new PropertyChangedEventArgs(propertyName));
            }
        }
    }
}
