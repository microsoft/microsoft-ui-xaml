// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Animation;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Enterprise.Moco.MaintainViewport
{
    [TestClass]
    public class ListViewTransitionsTests : XamlTestsBase
    {
        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        [TestMethod]
        public void CanPlayTransitionsWhenViewportIsCoerced()
        {
            ListView list = null;
            ScrollViewer scrollViewer = null;
            ItemsStackPanel panel = null;
            var listLoaded = new AutoResetEvent(false);
            var data = new ObservableCollection<string>(Enumerable.Range(0, 100).Select(i => "Item #" + i));

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsSource = data;
                list.Height = 500;
                list.Loaded += delegate
                {
                    Log.Comment("ListView loaded.");
                    scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                    panel = list.FindElementOfTypeInSubtree<ItemsStackPanel>();
                    // Prevent us from picking up animations due to the scroll bar.
                    scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to the very end");
                list.ScrollIntoView(data[data.Count - 1]);
            });

            TestServices.WindowHelper.WaitForIdle();

            int storyboardCounter = 0;
            object removedContainer = null;
            var storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
            {
                // The details of the animation are not the goal of this test.
                // We just want to check that they run in this viewport coercion scenario.
                if (storyboardCounter == 0) Verify.AreEqual(removedContainer, target);  // Delete animation
                if (storyboardCounter == 1) Verify.AreEqual(list.ContainerFromIndex(data.Count - 1), target); // Move animation
                ++storyboardCounter;
            });

            StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;
            UIExecutor.Execute(() =>
            {
                Log.Comment("Removing item #98. This will cause the viewport to shift.");
                Log.Comment(panel.FirstCacheIndex.ToString());
                Verify.AreEqual(99, panel.LastVisibleIndex);
                removedContainer = list.ContainerFromIndex(data.Count - 2);
                data.RemoveAt(data.Count - 2);
            });

            TestServices.WindowHelper.WaitForIdle();
            StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;
            Verify.AreEqual(2, storyboardCounter);
        }

        [TestMethod]
        public void ValidateAnimationsAreAdjustedWhenMaintainingViewport()
        {
            ListView list = null;
            var listLoaded = new AutoResetEvent(false);
            var data = new ObservableCollection<string>(Enumerable.Range(0, 3).Select(i => "Item #" + i));

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsSource = data;
                list.Height = 400;
                list.ContainerContentChanging += (s, e) => e.ItemContainer.Height = list.Height / 2;
                list.Loaded += delegate
                {
                    Log.Comment("ListView loaded.");
                    var scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                    // Prevent us from picking up animations due to the scroll bar.
                    scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                Log.Comment("Scrolling to the second item.");
                list.ScrollIntoView(data[1], ScrollIntoViewAlignment.Leading);
            });

            TestServices.WindowHelper.WaitForIdle();

            int storyboardCounter = 0;
            object removedContainer = null;
            var storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
            {
                // The details of the animation are not the goal of this test.
                // We just want to check that they run in this viewport coercion scenario.
                if (storyboardCounter == 0)
                {
                    Verify.AreEqual(removedContainer, target);  // Delete animation
                }
                else if (storyboardCounter == 1)
                {
                    Verify.AreEqual(list.ContainerFromIndex(0), target); // Move animation
                    var verticalAnimation = (DoubleAnimationUsingKeyFrames)storyboard.Children[1];
                    Verify.AreEqual("(UIElement.TransitionTarget).(TransitionTarget.CompositeTransform).TranslateY", Storyboard.GetTargetProperty(verticalAnimation));

                    Verify.AreEqual(3, verticalAnimation.KeyFrames.Count);
                    Verify.AreEqual(-((FrameworkElement)list.ContainerFromIndex(0)).ActualHeight, verticalAnimation.KeyFrames[0].Value);
                    Verify.AreEqual(-((FrameworkElement)list.ContainerFromIndex(0)).ActualHeight, verticalAnimation.KeyFrames[1].Value);
                    Verify.AreEqual(0, verticalAnimation.KeyFrames[2].Value);
                }
                ++storyboardCounter;
            });

            StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;
            UIExecutor.Execute(() =>
            {
                Log.Comment("Removing the second item.");
                // We expect item #3 to stay where it is, from an animation point of view, even though
                // it's going to move up 200px from the layout point of view.
                // Item #2 is going to fade out at offset 0. Item #1 is going to slide down from offset -200px to 0.
                removedContainer = list.ContainerFromIndex(1);
                data.RemoveAt(1);
            });

            TestServices.WindowHelper.WaitForIdle();
            StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;
            Verify.AreEqual(2, storyboardCounter);
        }

        [TestMethod]
        public void SupportMixOfAddsAndRemoves()
        {
            ListView list = null;

            ScrollViewer scrollViewer = null;
            ItemsStackPanel panel = null;
            var listLoaded = new AutoResetEvent(false);
            var data = new ObservableCollection<string>(Enumerable.Range(0, 10).Select(i => "Item #" + i));

            UIExecutor.Execute(() =>
            {
                Log.Comment("Loading list.");
                list = new ListView();
                list.ItemsSource = data;
                list.Height = 500;
                list.Loaded += delegate
                {
                    Log.Comment("ListView loaded.");
                    scrollViewer = list.FindElementOfTypeInSubtree<ScrollViewer>();
                    panel = list.FindElementOfTypeInSubtree<ItemsStackPanel>();
                    // Prevent us from picking up animations due to the scroll bar.
                    scrollViewer.VerticalScrollBarVisibility = ScrollBarVisibility.Hidden;
                    listLoaded.Set();
                };
                TestServices.WindowHelper.WindowContent = list;
            });

            Verify.IsTrue(listLoaded.WaitOne(TimeSpan.FromSeconds(5)), "Could not load list.");
            TestServices.WindowHelper.WaitForIdle();
            
            int storyboardCounter = 0;

            var storyboardStartedHandler = new StoryboardEventHandler((Storyboard storyboard, UIElement target) =>
            {
                // start of animations is not just viewed by going to the begintime of the storyboard.
                // simply because most need to have animations that start at keytime 0 to keep them invisible etc.
                // they have 'initial values' as defined by pvl
                // so we try to look at their complete duration.
                
                ListViewItem lvi = target as ListViewItem;

                TimeSpan longestLocal = TimeSpan.FromMilliseconds(0);
                foreach (var timeline in storyboard.Children)
                {
                    var dak = timeline as DoubleAnimationUsingKeyFrames;
                    if (dak != null)
                    {
                        TimeSpan kt = dak.KeyFrames[dak.KeyFrames.Count - 1].KeyTime.TimeSpan;
                        longestLocal = longestLocal.Milliseconds < kt.Milliseconds ? kt : longestLocal;
                    }
                }

                if (lvi.Content.ToString() == "Item #3")
                {
                    Verify.AreEqual(0, storyboardCounter);
                    // deleted item

                    Verify.AreEqual(longestLocal.TotalMilliseconds, 100);
                }
                if (lvi.Content.ToString() == "Item #4")
                {
                    Verify.AreEqual(1, storyboardCounter);
                    // one of the moving up items
                    Verify.AreEqual(longestLocal.TotalMilliseconds, 553);

                }
                if (lvi.Content.ToString() == "new item")
                {
                    Verify.IsTrue(storyboardCounter > 2);
                    
                    // item being added
                    
                    Verify.AreEqual(longestLocal.TotalMilliseconds, 1099);
                }

                ++storyboardCounter;
            });

            try
            {
                StoryboardMonitor.StoryboardStarted += storyboardStartedHandler;

                UIExecutor.Execute(() =>
                {
                    Log.Comment("Removing item #3. This will cause items above it to move up");
                    Log.Comment("Adding item @6. This will cause items under it to no longer move");
                    data.RemoveAt(3);
                    data.Insert(6, "new item");
                });

                TestServices.WindowHelper.WaitForIdle();
            }
            finally
            {
                StoryboardMonitor.StoryboardStarted -= storyboardStartedHandler;
            }

            Verify.AreEqual(storyboardCounter, 5);
        }

    }
}
