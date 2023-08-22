// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml.Controls;

using Common;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Media;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using ScrollView = Microsoft.UI.Xaml.Controls.ScrollView;
using RatingControl = Microsoft.UI.Xaml.Controls.RatingControl;
using ColorPicker = Microsoft.UI.Xaml.Controls.ColorPicker;
using NavigationView = Microsoft.UI.Xaml.Controls.NavigationView;
using ParallaxView = Microsoft.UI.Xaml.Controls.ParallaxView;
using ScrollPresenter = Microsoft.UI.Xaml.Controls.Primitives.ScrollPresenter;
using TreeView = Microsoft.UI.Xaml.Controls.TreeView;
using TreeViewNode = Microsoft.UI.Xaml.Controls.TreeViewNode;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
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

                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    var scrollView = new ScrollView();
                    objects["ScrollView"] = new WeakReference(scrollView);
                }
            });
            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            RunOnUIThread.Execute(() => CheckLeaks(objects));
        }


        [TestMethod]
        public void VerifyEventCycleScenario()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
            {
                Log.Warning("Reference tracking is only supported on RS2 and greater...skipping.");
                return;
            }

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
            IdleSynchronizer.Wait();

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

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

            IdleSynchronizer.Wait();
            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            RunOnUIThread.Execute(() => CheckLeaks(objects));
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
