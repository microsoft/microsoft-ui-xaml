// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Threading;

using MUXControlsTestApp.Utilities;

using Microsoft.UI.Xaml.Controls;
using Microsoft.UI.Xaml.Media.Imaging;

using WEX.TestExecution;
using WEX.TestExecution.Markup;

namespace Microsoft.UI.Xaml.Tests.MUXControls.ApiTests
{
    [TestClass]
    public class RatingControlTests : ApiTestBase
    {
        [TestMethod]
        public void VerifyDefaultsAndBasicSetting()
        {
            RatingControl ratingControl = null;
            RunOnUIThread.Execute(() =>
            {
                ratingControl = new RatingControl();
                Verify.IsNotNull(ratingControl);

                Verify.AreEqual(ratingControl.Caption, "");
                Verify.AreEqual(ratingControl.InitialSetValue, 1);
                Verify.AreEqual(ratingControl.IsClearEnabled, true);
                Verify.AreEqual(ratingControl.IsReadOnly, false);
                Verify.AreEqual(ratingControl.MaxRating, 5);
                Verify.AreEqual(ratingControl.PlaceholderValue, -1);
                Verify.AreEqual(ratingControl.Value, -1);

                // Disabled due to 12359255 reliability issue
                // Verify.IsTrue(ratingControl.ItemInfo is RatingItemFontInfo, "Verify default type of ItemInfo");
                // Verify.IsTrue((ratingControl.ItemInfo as RatingItemFontInfo).Glyph.Equals("\uE735"));

                // Now verify basic setting the value, and reaccessing it:

                ratingControl.Caption = "Rating API Test Caption";
                ratingControl.InitialSetValue = 2;
                ratingControl.IsClearEnabled = false;
                ratingControl.IsReadOnly = true;
                ratingControl.MaxRating = 10;
                ratingControl.PlaceholderValue = 3.0;
                ratingControl.Value = 2.0;

                var imageInfo = new RatingItemImageInfo();
                imageInfo.Image = new BitmapImage(new Uri("ms-appx:/Assets/rating_set.png"));
                ratingControl.ItemInfo = imageInfo;
            });

            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Verify.AreEqual(ratingControl.Caption, "Rating API Test Caption");
                Verify.AreEqual(ratingControl.InitialSetValue, 2);
                Verify.AreEqual(ratingControl.IsClearEnabled, false);
                Verify.AreEqual(ratingControl.IsReadOnly, true);
                Verify.AreEqual(ratingControl.MaxRating, 10);
                Verify.AreEqual(ratingControl.PlaceholderValue, 3.0);
                Verify.AreEqual(ratingControl.Value, 2.0);
                Verify.IsTrue(ratingControl.ItemInfo is RatingItemImageInfo, "Verify default type of ItemInfo [2]");
                Verify.IsTrue(((ratingControl.ItemInfo as RatingItemImageInfo).Image as BitmapImage).UriSource.Equals("ms-appx:/Assets/rating_set.png"));
            });
        }

        // Setting the value on a collapsed control can cause it to try and
        // interact with a non-existent AutomationPeer, causing a crash.
        [TestMethod]
        public void VerifyDontCrashWhenCollapsedAndValueSet()
        {
            RunOnUIThread.Execute(() =>
            {
                RatingControl ratingControl = new RatingControl();
                ratingControl.Visibility = Visibility.Collapsed;
                ratingControl.Value = 3.3;
            });
        }

        // Test just verifies the API contraaaact
        [TestMethod]
        public void VerifyValuesCoercion()
        {
            RunOnUIThread.Execute(() =>
            {
                RatingControl ratingControl = new RatingControl();
                Verify.IsNotNull(ratingControl);
                Verify.AreEqual(-1, ratingControl.PlaceholderValue);
                Verify.AreEqual(-1, ratingControl.Value);

                ratingControl.PlaceholderValue = 0.1;
                ratingControl.Value = 0.1;
                Verify.AreEqual(0.1, ratingControl.PlaceholderValue, "PlaceholderValue is just a display hint, so small fractional values should be preserved, not coerced to 1.0");
                Verify.AreEqual(1.0, ratingControl.Value, "Should coerce small Value values to 1.0");

                ratingControl.PlaceholderValue = 0.5;
                Verify.AreEqual(0.5, ratingControl.PlaceholderValue, "PlaceholderValue of 0.5 should be preserved, not coerced up to 1.0");

                ratingControl.PlaceholderValue = 0.0;
                Verify.AreEqual(0.0, ratingControl.PlaceholderValue, "PlaceholderValue of 0 should be preserved, not coerced up to 1.0");

                ratingControl.PlaceholderValue = -0.5;
                Verify.AreEqual(-1.0, ratingControl.PlaceholderValue, "Negative PlaceholderValue should be coerced to the 'unset' sentinel value");

                ratingControl.PlaceholderValue = 6.0;
                ratingControl.Value = 6.0;
                Verify.AreEqual(5.0, ratingControl.PlaceholderValue, "Should coerce PlaceholderValue above MaxRating back to MaxRating");
                Verify.AreEqual(5.0, ratingControl.Value, "Should coerce Value above MaxRating back to MaxRating");

                ratingControl.MaxRating = -2;
                Verify.AreEqual(1, ratingControl.MaxRating, "Should coerce MaxRating below 1 back up to 1.");

                Verify.AreEqual(1.0, ratingControl.PlaceholderValue, "Should auto-coerce now outdated PlaceholderValue above MaxRating back to MaxRating [2]");
                Verify.AreEqual(1.0, ratingControl.Value, "Should auto-coerce now outdated Value above MaxRating back to MaxRating [2]");

                ratingControl.PlaceholderValue = 6.0;
                ratingControl.Value = 6.0;
                Verify.AreEqual(1.0, ratingControl.PlaceholderValue, "Should coerce set PlaceholderValue above MaxRating back to MaxRating");
                Verify.AreEqual(1.0, ratingControl.Value, "Should coerce set Value above MaxRating back to MaxRating");
            });
        }

        // Regression test: setting MaxRating to an invalid value (e.g. below 1) while the
        // control is loaded used to be able to produce a transient invalid/negative width
        // during layout, because Value/PlaceholderValue were re-coerced against the
        // still-invalid MaxRating before it was normalized. This verifies that no exception
        // occurs and that MaxRating, Value, and PlaceholderValue all settle on the
        // corrected value of 1.
        [TestMethod]
        public void VerifyMaxRatingCoercionWhileLoadedDoesNotCrash()
        {
            ManualResetEvent loadedEvent = new(false);
            RatingControl ratingControl = null;

            RunOnUIThread.Execute(() =>
            {
                ratingControl = new RatingControl();
                ratingControl.Loaded += (sender, e) => loadedEvent.Set();
                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                ratingControl.Value = 6.0;
                ratingControl.PlaceholderValue = 6.0;
                ratingControl.UpdateLayout();

                ratingControl.MaxRating = -2;
                ratingControl.UpdateLayout();

                Verify.AreEqual(1, ratingControl.MaxRating, "Should coerce MaxRating below 1 back up to 1.");
                Verify.AreEqual(1.0, ratingControl.Value, "Should coerce Value down to the corrected MaxRating (1).");
                Verify.AreEqual(1.0, ratingControl.PlaceholderValue, "Should coerce PlaceholderValue down to the corrected MaxRating (1), not the unset sentinel.");
            });

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            IdleSynchronizer.Wait();
        }

        [TestMethod]
        [TestProperty("IsolationLevel", "Method")] // This test alters the application resources, so it's isolated from other tests.
        public void VerifySizeIsChangeableFromResource()
        {
            ManualResetEvent loadedEvent = new(false);
            ManualResetEvent unloadedEvent = new(false);
            double originalWidth = 0;
            double previousWidth = 0;

            RunOnUIThread.Execute(() =>
            {
                var ratingControl = new RatingControl();

                ratingControl.Loaded += (sender, e) =>
                {
                    originalWidth = ratingControl.ActualWidth;
                    previousWidth = originalWidth;
                    loadedEvent.Set();
                };

                ratingControl.Unloaded += (sender, e) => unloadedEvent.Set();

                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            unloadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Application.Current.Resources["RatingControlFontSizeForRendering"] = 20.0;

                var ratingControl = new RatingControl();

                ratingControl.Loaded += (sender, e) =>
                {
                    Verify.IsLessThan(ratingControl.ActualWidth, previousWidth);
                    previousWidth = ratingControl.ActualWidth;
                    loadedEvent.Set();
                };

                ratingControl.Unloaded += (sender, e) => unloadedEvent.Set();

                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            unloadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Application.Current.Resources["RatingControlItemSpacing"] = 20.0;

                var ratingControl = new RatingControl();

                ratingControl.Loaded += (sender, e) =>
                {
                    Verify.IsGreaterThan(ratingControl.ActualWidth, previousWidth);
                    previousWidth = ratingControl.ActualWidth;
                    loadedEvent.Set();
                };

                ratingControl.Unloaded += (sender, e) => unloadedEvent.Set();

                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            unloadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Application.Current.Resources["RatingControlFontSizeForRendering"] = 48.0;
                Application.Current.Resources.Remove("RatingControlItemSpacing");

                var ratingControl = new RatingControl();

                ratingControl.Loaded += (sender, e) =>
                {
                    Verify.IsGreaterThan(ratingControl.ActualWidth, originalWidth);
                    Verify.IsGreaterThan(ratingControl.ActualWidth, previousWidth);
                    previousWidth = ratingControl.ActualWidth;
                    loadedEvent.Set();
                };

                ratingControl.Unloaded += (sender, e) => unloadedEvent.Set();

                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            unloadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Application.Current.Resources["RatingControlItemSpacing"] = 2.0;

                var ratingControl = new RatingControl();

                ratingControl.Loaded += (sender, e) =>
                {
                    Verify.IsLessThan(ratingControl.ActualWidth, previousWidth);
                    loadedEvent.Set();
                };

                ratingControl.Unloaded += (sender, e) => unloadedEvent.Set();

                Content = ratingControl;
            });

            loadedEvent.WaitOne();
            IdleSynchronizer.Wait();

            RunOnUIThread.Execute(() =>
            {
                Content = null;
            });

            unloadedEvent.WaitOne();
            IdleSynchronizer.Wait();
        }
    }
}
