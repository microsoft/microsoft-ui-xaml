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
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Media;
using Windows.UI;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using RatingControl = Microsoft.UI.Xaml.Controls.RatingControl;
using PersonPicture = Microsoft.UI.Xaml.Controls.PersonPicture;

namespace Windows.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class ThemeResourcesTests : ApiTestBase
    {
        [ClassInitialize]
        [TestProperty("Classification", "Integration")]
        public static void Setup(TestContext context) { }

        [TestMethod]
        // Isolate this test because other tests might have run in this context and either loaded the brushes 
        // already (thereby invalidating what we're trying to test) or our changes will adversly affect other tests.
        // Because of the test isolation we cannot use the base classes Content property as it may not be initialized.
        [TestProperty("IsolationLevel", "Method")]
        public void VerifyOverrides()
        {
            RatingControl ratingControl = null;
            PersonPicture personPicture = null;
            Slider slider = null;
            Grid root = null;

            RunOnUIThread.Execute(() =>
            {
                var appResources = Application.Current.Resources;
                // 1) Override WinUI defined brush in App.Resources.
                appResources["RatingControlCaptionForeground"] = new SolidColorBrush(Colors.Orange);

                // 2) Override system brush used by WinUI ThemeResource.

                ((ResourceDictionary)appResources.ThemeDictionaries["Light"])["SystemAltHighColor"] = Colors.Green;
                ((ResourceDictionary)appResources.ThemeDictionaries["Default"])["SystemAltHighColor"] = Colors.Green;
                ((ResourceDictionary)appResources.ThemeDictionaries["HighContrast"])["SystemColorButtonTextColor"] = Colors.Green;

                // 3) Override brush name used by a system control
                appResources["SliderTrackValueFill"] = new SolidColorBrush(Colors.Purple);

                root = new Grid {
                    Background = new SolidColorBrush(Colors.AntiqueWhite),
                };

                StackPanel panel = new StackPanel { Orientation = Orientation.Vertical };
                panel.Children.Add(slider = new Slider());

                panel.Children.Add(ratingControl = new RatingControl() { Value = 2 });
                panel.Children.Add(personPicture = new PersonPicture());

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

                // 2) Verify that overriding a system color used by a WinUI control works.
                Verify.AreEqual(Colors.Green, ((SolidColorBrush)personPicture.Foreground).Color,
                    "Verify PersonPictureForegroundThemeBrush (which uses SystemAltHighColor) overridden in Application.Resources gets picked up by WinUI control");

                // 3) Verify that overriding a system brush used by a system control works.
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

                Log.Comment("Setting Window.Current.Content = null");
                MUXControlsTestApp.App.TestContentRoot = null;
            });
            IdleSynchronizer.TryWait();
        }
    }
}
