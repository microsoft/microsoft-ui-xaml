// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
using Private.Infrastructure;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Threading;
using WEX.Logging.Interop;
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using Windows.ApplicationModel.DataTransfer;
using Windows.Foundation;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Data;
using Microsoft.UI.Xaml.Input;
using Microsoft.UI.Xaml.Markup;
using Microsoft.UI.Xaml.Media;
using Microsoft.UI.Xaml.Tests.Common;

namespace Microsoft.UI.Xaml.Tests.Controls
{
    [TestClass]
    public class ScrollBarTests : XamlTestsBase
    {
        static string TestDeploymentDir { get; set; }

        [ClassInitialize]
        [TestProperty("BinaryUnderTest", "Microsoft.UI.Xaml.dll")]
        [TestProperty("RunAs", "UAP")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("UAP:Praid", "XamlManagedTAEFTests")]
        [TestProperty("HelixWorkItemCreation", "CreateWorkItemPerTestClass")]
        public static void Setup(TestContext context)
        {
            AssemblySetup.CommonTestClassSetup();
            TestDeploymentDir = context.TestDeploymentDir;
        }

        [ClassCleanup]
        public void ClassCleanup()
        {
            base.CommonClassCleanup();
        }

        public string GetCurrentVisualState(FrameworkElement element, string visualStateGroupName)
        {
            FrameworkElement root = (FrameworkElement)VisualTreeHelper.GetChild(element, 0);
            Log.Comment("Template root: {0}", root.Name);

            var groups = VisualStateManager.GetVisualStateGroups(root);
            Verify.IsNotNull(groups, "Cannot find visual state groups!!!");
            Verify.IsGreaterThan(groups.Count, 0, "Groups count was not greater than 0!!!");

            VisualStateGroup group = null;
            foreach (VisualStateGroup g in groups)
            {
                Log.Comment("Group Name: {0}", g.Name);
                if (g.Name.Equals(visualStateGroupName, StringComparison.OrdinalIgnoreCase))
                {
                    Log.Comment("Found" + visualStateGroupName);
                    group = g;
                    break;
                }
            }

            return group.CurrentState.Name;
        }

        [TestMethod]
        [TestProperty("TestPass:ExcludeOn", "WindowsCore")]
        [TestProperty("Hosting:Mode", "UAP")]
        [TestProperty("Ignore", "True")] // DCPP: Unreliable test: Controls.ScrollBarTests.ScrollBarExpandCollapseWithoutAnimation
        public void ScrollBarExpandCollapseWithoutAnimations()
        {
            ScrollBarExpandCollapseWithoutAnimationsBase(true);
        }

        public void ScrollBarExpandCollapseWithoutAnimationsBase(bool expectWithoutAnimationStates)
        {
            using (RuntimeFeature.Enable(12 /*DisableGlobalAnimations*/))
            {
                ScrollBar scrollBar = null;
                Button button = null;
                ManualResetEvent scrollBarLoaded = new ManualResetEvent(false);

                UIExecutor.Execute(() =>
                {
                    button = new Button() { Content = "Hello" };
                    scrollBar = new ScrollBar()
                    {
                        Orientation = Orientation.Horizontal,
                        Value = 10,
                        Minimum = 0,
                        Maximum = 100,
                        ViewportSize = 10,
                        IndicatorMode = ScrollingIndicatorMode.MouseIndicator
                    };

                    scrollBar.Loaded += (s, e) =>
                    {
                        scrollBarLoaded.Set();
                    };

                    StackPanel sp = new StackPanel();
                    sp.Children.Add(scrollBar);
                    sp.Children.Add(button);

                    TestServices.WindowHelper.WindowContent = sp;

                });

                Verify.IsTrue(scrollBarLoaded.WaitOne(TimeSpan.FromSeconds(10)), "Received loaded event from scrollbar");
                TestServices.WindowHelper.WaitForIdle();

                TestServices.InputHelper.MoveMouse(button);
                TestServices.WindowHelper.WaitForIdle();
                
                TestServices.InputHelper.MoveMouse(scrollBar);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var currentState = GetCurrentVisualState(scrollBar, "ConsciousStates");
                    if (expectWithoutAnimationStates)
                    {
                        Verify.AreEqual("ExpandedWithoutAnimation", currentState);
                    }
                    else
                    {
                        Verify.AreEqual("Expanded", currentState);
                    }
                });

                TestServices.InputHelper.MoveMouse(button);
                TestServices.WindowHelper.WaitForIdle();

                UIExecutor.Execute(() =>
                {
                    var currentState = GetCurrentVisualState(scrollBar, "ConsciousStates");
                    if (expectWithoutAnimationStates)
                    {
                        Verify.AreEqual("CollapsedWithoutAnimation", currentState);
                    }
                    else
                    {
                        Verify.AreEqual("Collapsed", currentState);
                    }
                });
            }
        }
    }
}
