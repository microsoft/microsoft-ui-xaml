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
        public void ScrollBarInitializesTrackBrushesOnPointerExpansion()
        {
            ScrollBar scrollBar = null;
            Button button = null;
            Style applicationStyle = null;
            UIExecutor.Execute(() =>
            {
                scrollBar = new ScrollBar
                {
                    Orientation = Orientation.Vertical,
                    Width = 12,
                    Height = 200,
                    Maximum = 100,
                    ViewportSize = 20,
                    IndicatorMode = ScrollingIndicatorMode.MouseIndicator,
                    IsEnabled = false,
                    Style = (Style)new XamlControlsResources()["DefaultScrollBarStyle"]
                };
                button = new Button { Content = "Pointer parking", Width = 150, Height = 100 };
                var panel = new StackPanel { Orientation = Orientation.Horizontal };
                panel.Children.Add(scrollBar);
                panel.Children.Add(button);
                TestServices.WindowHelper.WindowContent = panel;
            });
            TestServices.WindowHelper.WaitForIdle();
            TestServices.InputHelper.MoveMouse(button);
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                applicationStyle = new Style { TargetType = typeof(Microsoft.UI.Xaml.Shapes.Rectangle) };
                applicationStyle.Setters.Add(new Setter(FrameworkElement.TagProperty, "Application style"));
                var root = (FrameworkElement)VisualTreeHelper.GetChild(scrollBar, 0);
                foreach (var name in new[] { "HorizontalTrackRect", "VerticalTrackRect" })
                {
                    ((Microsoft.UI.Xaml.Shapes.Rectangle)root.FindName(name)).Style = applicationStyle;
                }
                scrollBar.IsEnabled = true;
            });
            TestServices.InputHelper.MoveMouse(scrollBar);
            TestServices.WindowHelper.WaitForIdle();

            UIExecutor.Execute(() =>
            {
                var root = (FrameworkElement)VisualTreeHelper.GetChild(scrollBar, 0);
                var track = (Microsoft.UI.Xaml.Shapes.Rectangle)root.FindName("VerticalTrackRect");
                Verify.IsNotNull(track);
                Verify.AreEqual(applicationStyle, track.Style, "Pointer expansion must preserve the application's Style.");
                Verify.AreEqual("Application style", track.Tag);
                Verify.AreNotEqual(DependencyProperty.UnsetValue, track.ReadLocalValue(Microsoft.UI.Xaml.Shapes.Shape.FillProperty));
                Verify.AreNotEqual(DependencyProperty.UnsetValue, track.ReadLocalValue(Microsoft.UI.Xaml.Shapes.Shape.StrokeProperty));
                Verify.IsNotNull(track.Fill);
                Verify.IsNotNull(track.Stroke);
                scrollBar.IsEnabled = false;
            });
            TestServices.WindowHelper.WaitForIdle();
            UIExecutor.Execute(() =>
            {
                var root = (FrameworkElement)VisualTreeHelper.GetChild(scrollBar, 0);
                var track = (Microsoft.UI.Xaml.Shapes.Rectangle)root.FindName("VerticalTrackRect");
                Verify.IsNotNull(track.Fill, "Disabling an expanded bar must not lose its base fill.");
                Verify.IsNotNull(track.Stroke, "Disabling an expanded bar must not lose its base stroke.");
                Verify.AreEqual(applicationStyle, track.Style);
                track.ClearValue(Microsoft.UI.Xaml.Shapes.Shape.FillProperty);
                track.ClearValue(Microsoft.UI.Xaml.Shapes.Shape.StrokeProperty);
                scrollBar.IsEnabled = true;
                Verify.IsTrue(VisualStateManager.GoToState(scrollBar, "Collapsed", false));
                Verify.AreEqual(DependencyProperty.UnsetValue, track.ReadLocalValue(Microsoft.UI.Xaml.Shapes.Shape.FillProperty),
                    "Later state updates must not undo the application's ClearValue.");
                Verify.AreEqual(DependencyProperty.UnsetValue, track.ReadLocalValue(Microsoft.UI.Xaml.Shapes.Shape.StrokeProperty));
                Verify.IsNull(track.Fill);
                Verify.IsNull(track.Stroke);
            });
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
