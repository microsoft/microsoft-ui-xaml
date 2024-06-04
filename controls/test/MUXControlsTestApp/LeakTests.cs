// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

using MUXControlsTestApp.Utilities;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;

using Common;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Media;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
using System.Linq;
using Microsoft.UI.Xaml.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class LeakTests : ApiTestBase
    {
        void CheckLeaks(Dictionary<string, WeakReference> objects)
        {
            foreach (var pair in objects)
            {
                Log.Comment("Checking if object {0} has been collected", pair.Key);
                object target = pair.Value.Target;
                if (target != null)
                {
                    Verify.DisableVerifyFailureExceptions = true; // throwing exceptions makes running this in TDP harder
                    Verify.Fail(String.Format("Object {0} is still alive when it should not be.", target));
                    Verify.DisableVerifyFailureExceptions = false;
                }
            }
            objects.Clear();
        }

        [TestMethod]
        public void VerifySimpleCollectionScenario()
        {
            // This test caught bug 6578691 in the past, but we didn't notice for a while because the repro rate was relatively
            // low and it often was marked as "pass" after re-runs.  So, we run 100 iterations to help us catch an issue like this
            // more reliably in the future.
            for (int i = 0; i < 100; ++i)
            {
                Log.Comment("Iteration {0}...", i);

                var objects = new Dictionary<string, WeakReference>();

                RunOnUIThread.Execute(() =>
                {
                    var rating = new RatingControl();
                    objects["Rating"] = new WeakReference(rating);

                    var colorPicker = new ColorPicker();
                    objects["ColorPicker"] = new WeakReference(colorPicker);

                    var navigationView = new NavigationView();
                    objects["NavigationView"] = new WeakReference(navigationView);

                    var parallaxView = new ParallaxView();
                    objects["ParallaxView"] = new WeakReference(parallaxView);

                    var scrollPresenter = new ScrollPresenter();
                    objects["ScrollPresenter"] = new WeakReference(scrollPresenter);

                    var scrollView = new ScrollView();
                    objects["ScrollView"] = new WeakReference(scrollView);
                });

                WaitAndGC();
                RunOnUIThread.Execute(() => CheckLeaks(objects));
                IdleSynchronizer.Wait();
            }
        }


        [TestMethod]
        [TestProperty("Ignore", "True")]    // Task 27843113: DCPP: LeakTests.VerifyEventCycleScenario and ValidateCollectionApisWithResourceDictionary are unreliable
        public void VerifyEventCycleScenario()
        {
            var objects = new Dictionary<string, WeakReference>();

            // Create EventCycle but don't add it to the tree. Demonstrates cycle with Events.
            Log.Comment("Create EventCycle but don't add it to the tree. Demonstrates cycle with Events");
            RunOnUIThread.Execute(() =>
            {
                var eventCycle = new MUXControlsTestApp.EventCycleTest(false);
                objects["EventCycle"] = new WeakReference(eventCycle);
            });
            IdleSynchronizer.Wait();

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            Log.Comment("Verify that EventCycleTest object got collected.");
            // Verify that EventCycle got collected.
            RunOnUIThread.Execute(() => CheckLeaks(objects));
            IdleSynchronizer.Wait();

            // Now make another EventCycle object but this time add it to the tree.
            Log.Comment("Now make another EventCycle object but this time add it to the tree.");
            RunOnUIThread.Execute(() =>
            {
                var eventCycle = new MUXControlsTestApp.EventCycleTest();
                Content = eventCycle;
                objects["EventCycle"] = new WeakReference(eventCycle);
                Content.UpdateLayout();
                // After templates have been expanded, now remove it. After the dust has settled we expect that things have been GC'd.
                Log.Comment("After templates have been expanded, now remove it. After the dust has settled we expect that things have been GC'd.");
                Content = null;
            });

            WaitAndGC();
            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        [TestMethod]
        public void VerifyTreeViewHasNoLeak()
        {
            var objects = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() =>
            {
                var tree = new TreeView();
                objects["Tree"] = new WeakReference(tree);

                for (int i = 1; i <= 3; i++)
                {
                    var parentName = "Node " + i;
                    var parentNode = new TreeViewNode() { Content = parentName };
                    objects[parentName] = new WeakReference(parentNode);

                    var childName = "Node " + i + ".1";
                    var childNode = new TreeViewNode() { Content = childName };
                    objects[childName] = new WeakReference(childNode);

                    parentNode.Children.Add(childNode);
                    tree.RootNodes.Add(parentNode);
                }

                Content = tree;
                Content.UpdateLayout();
                objects["TreeViewList"] = new WeakReference(FindVisualChildByName(tree, "ListControl"));
                tree = null;
                Content = null;
            });

            WaitAndGC();
            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        [TestMethod]
        public void VerifyListViewIsLeakFree()
        {
            var objects = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() =>
            {
                var listView = new ListView();
                objects["ListView"] = new WeakReference(listView);

                listView.ItemsSource = Enumerable.Range(0, 1000);

                Content = listView;
                Content.UpdateLayout();
                listView = null;
                Content = null;
            });

            WaitAndGC();
            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // This is still leaking. Bug 41475047 (likely)
        public void VerifyListViewWithContainerAsRootOfTemplateIsLeakFree()
        {
            var objects = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() =>
            { 
                var listView = new ListView();
                objects["ListView"] = new WeakReference(listView);

                listView.ItemsSource = Enumerable.Range(0, 1000);
                listView.ItemTemplate = (DataTemplate)XamlReader.Load(
                       @"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>
                           <ListViewItem>
                            <TextBlock Text='{Binding}'/>
                           </ListViewItem>
                        </DataTemplate>");

                Content = listView;
                Content.UpdateLayout();
                listView = null;
                Content = null;
            });

            WaitAndGC();
            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // This is still leaking.Bug 39415078
        public void VerifyTreeViewIsLeakFree()
        {
            var objects = new Dictionary<string, WeakReference>();

            RunOnUIThread.Execute(() =>
            {
                var treeView = new TreeView();
                objects["TreeView"] = new WeakReference(treeView);

                treeView.ItemsSource = Enumerable.Range(0, 1000);

                Content = treeView;
                Content.UpdateLayout();
                treeView = null;
                Content = null;
            });

            WaitAndGC();
            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }

        private void WaitAndGC()
        {
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }

        public static DependencyObject FindVisualChildByName(FrameworkElement parent, string name)
        {
            if (parent.Name == name)
            {
                return parent;
            }

            int childrenCount = VisualTreeHelper.GetChildrenCount(parent);

            for (int i = 0; i < childrenCount; i++)
            {
                FrameworkElement childAsFE = VisualTreeHelper.GetChild(parent, i) as FrameworkElement;

                if (childAsFE != null)
                {
                    DependencyObject result = FindVisualChildByName(childAsFE, name);

                    if (result != null)
                    {
                        return result;
                    }
                }
            }

            return null;
        }
    }
}
