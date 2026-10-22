// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using Common;
using MUXControlsTestApp.Utilities;

using Windows.Foundation.Metadata;
using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Controls.Primitives;
using Microsoft.UI.Xaml.Media;
using Windows.UI;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ThemeResourcesTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) { }

        [TestMethod]
        public void VerifyActualThemeOneWayXBindUpdates()
        {
            MUXControlsTestApp.ActualThemeBindingPage page = null;
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    page = new MUXControlsTestApp.ActualThemeBindingPage();
                    MUXControlsTestApp.App.TestContentRoot = page;
                });
                IdleSynchronizer.TryWait();

                RunOnUIThread.Execute(() =>
                {
                    Verify.AreEqual(ElementTheme.Light, page.BoundTheme.RequestedTheme);
                    page.ThemeSource.RequestedTheme = ElementTheme.Dark;
                    Verify.AreEqual(ElementTheme.Dark, page.BoundTheme.RequestedTheme,
                        "OneWay x:Bind must observe a change to ActualTheme");
                    page.ThemeSource.RequestedTheme = ElementTheme.Light;
                    Verify.AreEqual(ElementTheme.Light, page.BoundTheme.RequestedTheme);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() => MUXControlsTestApp.App.TestContentRoot = null);
                IdleSynchronizer.TryWait();
            }
        }

        [TestMethod]
        public void VerifyActualThemePropertyChangedCallback()
        {
            Grid root = null;
            Grid inheritingChild = null;
            Grid overridingChild = null;
            long inheritedToken = 0;
            long overriddenToken = 0;
            var observedThemes = new List<ElementTheme>();
            int overriddenNotifications = 0;

            try
            {
                RunOnUIThread.Execute(() =>
                {
                    root = new Grid { RequestedTheme = ElementTheme.Light };
                    inheritingChild = new Grid();
                    overridingChild = new Grid { RequestedTheme = ElementTheme.Dark };
                    root.Children.Add(inheritingChild);
                    root.Children.Add(overridingChild);
                    MUXControlsTestApp.App.TestContentRoot = root;
                });
                IdleSynchronizer.TryWait();

                RunOnUIThread.Execute(() =>
                {
                    inheritedToken = inheritingChild.RegisterPropertyChangedCallback(
                        FrameworkElement.ActualThemeProperty,
                        (sender, property) => observedThemes.Add(((FrameworkElement)sender).ActualTheme));
                    overriddenToken = overridingChild.RegisterPropertyChangedCallback(
                        FrameworkElement.ActualThemeProperty,
                        (sender, property) => overriddenNotifications++);

                    root.RequestedTheme = ElementTheme.Dark;
                    root.RequestedTheme = ElementTheme.Light;
                    inheritingChild.RequestedTheme = ElementTheme.Dark;
                    inheritingChild.RequestedTheme = ElementTheme.Default;
                    root.RequestedTheme = ElementTheme.Light;

                    Verify.AreEqual(4, observedThemes.Count,
                        "Notify once per effective base-theme change, including clearing an override");
                    Verify.AreEqual(ElementTheme.Dark, observedThemes[0], "The getter must return the new theme inside the callback");
                    Verify.AreEqual(ElementTheme.Light, observedThemes[1]);
                    Verify.AreEqual(ElementTheme.Dark, observedThemes[2]);
                    Verify.AreEqual(ElementTheme.Light, observedThemes[3]);
                    Verify.AreEqual(0, overriddenNotifications, "An unchanged effective theme must not notify");
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    inheritingChild?.UnregisterPropertyChangedCallback(FrameworkElement.ActualThemeProperty, inheritedToken);
                    overridingChild?.UnregisterPropertyChangedCallback(FrameworkElement.ActualThemeProperty, overriddenToken);
                    MUXControlsTestApp.App.TestContentRoot = null;
                });
                IdleSynchronizer.TryWait();
            }
        }

        [TestMethod]
        public void VerifyActualThemeCallbackCanChangeRequestedTheme()
        {
            Grid root = null;
            long token = 0;
            var observedThemes = new List<ElementTheme>();
            try
            {
                RunOnUIThread.Execute(() =>
                {
                    root = new Grid { RequestedTheme = ElementTheme.Light };
                    MUXControlsTestApp.App.TestContentRoot = root;
                });
                IdleSynchronizer.TryWait();

                RunOnUIThread.Execute(() =>
                {
                    token = root.RegisterPropertyChangedCallback(FrameworkElement.ActualThemeProperty, (sender, property) =>
                    {
                        observedThemes.Add(root.ActualTheme);
                        if (root.ActualTheme == ElementTheme.Dark)
                        {
                            root.RequestedTheme = ElementTheme.Light;
                        }
                    });

                    root.RequestedTheme = ElementTheme.Dark;
                    Verify.AreEqual(ElementTheme.Light, root.ActualTheme,
                        "The callback's theme change must not be suppressed by the previous theme walk");
                    Verify.AreEqual(2, observedThemes.Count);
                    Verify.AreEqual(ElementTheme.Dark, observedThemes[0]);
                    Verify.AreEqual(ElementTheme.Light, observedThemes[1]);
                });
            }
            finally
            {
                RunOnUIThread.Execute(() =>
                {
                    root?.UnregisterPropertyChangedCallback(FrameworkElement.ActualThemeProperty, token);
                    MUXControlsTestApp.App.TestContentRoot = null;
                });
                IdleSynchronizer.TryWait();
            }
        }

        [TestMethod]
        // Isolate this test because other tests might have run in this context and either loaded the brushes 
        // already (thereby invalidating what we're trying to test) or our changes will adversly affect other tests.
        // Because of the test isolation we cannot use the base classes Content property as it may not be initialized.
        [TestProperty("IsolationLevel", "Method")]
        public void VerifyOverrides()
        {
            RatingControl ratingControl = null;
            Slider slider = null;
            Grid root = null;

            RunOnUIThread.Execute(() =>
            {
                var appResources = Application.Current.Resources;
                // 1) Override WinUI defined brush in App.Resources.
                appResources["RatingControlCaptionForeground"] = new SolidColorBrush(Colors.Orange);

                // 2) Override brush name used by a system control
                appResources["SliderTrackValueFill"] = new SolidColorBrush(Colors.Purple);

                root = new Grid {
                    Background = new SolidColorBrush(Colors.AntiqueWhite),
                };

                StackPanel panel = new StackPanel { Orientation = Orientation.Vertical };
                panel.Children.Add(slider = new Slider());

                panel.Children.Add(ratingControl = new RatingControl() { Value = 2 });

                root.Children.Add(panel);
                // Add an element over top to prevent stray mouse input from interfering.
                root.Children.Add(new Button {
                    Background = new SolidColorBrush(Color.FromArgb(30, 0, 255, 0)),
                    HorizontalAlignment = HorizontalAlignment.Stretch,
                    VerticalAlignment = VerticalAlignment.Stretch
                });

                MUXControlsTestApp.App.TestContentRoot = root;
            });
            IdleSynchronizer.TryWait();

            //System.Threading.Tasks.Task.Delay(TimeSpan.FromSeconds(20)).Wait();

            RunOnUIThread.Execute(() =>
            {
                // 1) Verify that overriding WinUI defined brushes in App.Resources works.
                Verify.AreEqual(Colors.Orange, ((SolidColorBrush)ratingControl.Foreground).Color,
                    "Verify RatingControlCaptionForeground override in Application.Resources gets picked up by WinUI control");

                // 2) Verify that overriding a system brush used by a system control works.
                if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone1))
                {
                    // Below code is comment because of bug 19180323 and we expect the code to be enabled again after test case is moved to nuget testapp

                    //Verify.AreEqual(Colors.Purple, ((SolidColorBrush)slider.Foreground).Color,
                    //    "Verify Slider (which uses SliderTrackValueFill as its .Foreground) overridden in Application.Resources gets picked up by Slider control");
                }
                else
                {
                    // Before RS1, we used to reference system brushes directly.
                }

                Log.Comment("Setting App.TestContentRoot = null");
                MUXControlsTestApp.App.TestContentRoot = null;
            });
            IdleSynchronizer.TryWait();
        }
    }
}
