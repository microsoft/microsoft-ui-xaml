// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Point = System.Drawing.Point;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ScrollPresenterTestsWithInputHelper : ScrollPresenterTestsBase
    {
        private enum ScrollSnapPointsAlignment
        {
            Near = 0,
            Center = 1,
            Far = 2
        }

        // Mouse wheel delta amount required per initial velocity unit
        // 120 matches the built-in InteractionTracker zooming behavior introduced in RS5.
        const int mouseWheelDeltaForVelocityUnit = 120;

        // The longest observed animated view change took 5.4 seconds, so 9 seconds is picked
        // as the default timeout so there is a reasonable margin for reliability.
        const double defaultAnimatedViewChangeTimeout = 9000;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        [TestProperty("Description", "Pans a Rectangle in a ScrollPresenter, with railing.")]
        public void PanWithRailing()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            const double minVerticalScrollPercent = 50.0;

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    SetOutputDebugStringLevel("Verbose");

                    GoToSimpleContentsPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving cmbShowScrollPresenter");
                    ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                    Log.Comment("Changing ScrollPresenter selection to scrollPresenter11");
                    cmbShowScrollPresenter.SelectItemByName("scrollPresenter11");
                    Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                    Log.Comment("Retrieving ScrollPresenter");
                    UIObject scrollPresenter11UIObject = FindElement.ByName("ScrollPresenter11");
                    Verify.IsNotNull(scrollPresenter11UIObject, "Verifying that ScrollPresenter was found");

                    WaitForScrollPresenterFinalSize(scrollPresenter11UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                    // Tapping button before attempting pan operation to guarantee effective touch input
                    TapResetViewsButton();
                    Log.Comment("Panning ScrollPresenter almost vertically");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter11");

                    InputHelper.Pan(
                        scrollPresenter11UIObject,
                        new Point(scrollPresenter11UIObject.BoundingRectangle.Left + 100, scrollPresenter11UIObject.BoundingRectangle.Top + 150),
                        new Point(scrollPresenter11UIObject.BoundingRectangle.Left + 95, scrollPresenter11UIObject.BoundingRectangle.Top + 50));

                    Log.Comment("Waiting for scrollPresenter11 pan completion");
                    bool success = WaitForManipulationEnd("scrollPresenter11", "txtScrollPresenterState", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    ScrollPresenter scrollPresenter11 = new ScrollPresenter(scrollPresenter11UIObject);

                    Log.Comment("scrollPresenter11.HorizontalScrollPercent={0}", scrollPresenter11.HorizontalScrollPercent);
                    Log.Comment("scrollPresenter11.VerticalScrollPercent={0}", scrollPresenter11.VerticalScrollPercent);

                    if (!success ||
                        scrollPresenter11.HorizontalScrollPercent != 0.0 ||
                        scrollPresenter11.VerticalScrollPercent <= minVerticalScrollPercent)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.AreEqual(scrollPresenter11.HorizontalScrollPercent, 0.0, "Verifying scrollPresenter11 HorizontalScrollPercent is 0%");
                        Verify.IsTrue(scrollPresenter11.VerticalScrollPercent > minVerticalScrollPercent, "Verifying scrollPresenter11 VerticalScrollPercent is greater than " + minVerticalScrollPercent + "%");
                    }

                    // scrollPresenter11's Content height is 1000px.
                    double horizontalOffset;
                    double verticalOffset;
                    double minVerticalOffset = 1000.0 * (1.0 - scrollPresenter11.VerticalViewSize / 100.0) * minVerticalScrollPercent / 100.0;
                    float zoomFactor;

                    GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                    Log.Comment("horizontalOffset={0}", horizontalOffset);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Log.Comment("zoomFactor={0}", zoomFactor);

                    if (additionalAttempts == 0)
                    {
                        Verify.AreEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is 0.0");
                        Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                        Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                    // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
                }
            }
            while (additionalAttempts > 0);
        }

        [TestMethod]
        [TestProperty("Description", "Pans an Image in a ScrollPresenter, without railing.")]
        public void PanWithoutRailing()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            const double minHorizontalScrollPercent = 35.0;
            const double minVerticalScrollPercent = 35.0;

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    SetOutputDebugStringLevel("Verbose");

                    GoToSimpleContentsPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving cmbShowScrollPresenter");
                    ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                    Log.Comment("Changing ScrollPresenter selection to scrollPresenter51");
                    cmbShowScrollPresenter.SelectItemByName("scrollPresenter51");
                    Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                    Log.Comment("Retrieving ScrollPresenter");
                    UIObject scrollPresenter51UIObject = FindElement.ByName("ScrollPresenter51");
                    Verify.IsNotNull(scrollPresenter51UIObject, "Verifying that ScrollPresenter was found");

                    WaitForScrollPresenterFinalSize(scrollPresenter51UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                    // Tapping button before attempting pan operation to guarantee effective touch input
                    TapResetViewsButton();

                    Log.Comment("Panning ScrollPresenter in diagonal");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter51");

                    InputHelper.Pan(
                        scrollPresenter51UIObject,
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 25, scrollPresenter51UIObject.BoundingRectangle.Top + 25),
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left - 25, scrollPresenter51UIObject.BoundingRectangle.Top - 25));

                    Log.Comment("Waiting for scrollPresenter51 pan completion");
                    bool success = WaitForManipulationEnd("scrollPresenter51", "txtScrollPresenterState", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    ScrollPresenter scrollPresenter51 = new ScrollPresenter(scrollPresenter51UIObject);

                    Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                    Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                    if (!success ||
                        scrollPresenter51.HorizontalScrollPercent <= minHorizontalScrollPercent ||
                        scrollPresenter51.VerticalScrollPercent <= minVerticalScrollPercent)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.IsTrue(scrollPresenter51.HorizontalScrollPercent > minHorizontalScrollPercent, "Verifying scrollPresenter51 HorizontalScrollPercent is greater than " + minHorizontalScrollPercent + "%");
                        Verify.IsTrue(scrollPresenter51.VerticalScrollPercent > minVerticalScrollPercent, "Verifying scrollPresenter51 VerticalScrollPercent is greater than " + minVerticalScrollPercent + "%");
                    }

                    // scrollPresenter51's Content size is 800x800px.
                    double horizontalOffset;
                    double verticalOffset;
                    double minHorizontalOffset = 800.0 * (1.0 - scrollPresenter51.HorizontalViewSize / 100.0) * minHorizontalScrollPercent / 100.0;
                    double minVerticalOffset = 800.0 * (1.0 - scrollPresenter51.VerticalViewSize / 100.0) * minVerticalScrollPercent / 100.0;
                    float zoomFactor;

                    GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                    Log.Comment("horizontalOffset={0}", horizontalOffset);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Log.Comment("zoomFactor={0}", zoomFactor);

                    if (additionalAttempts == 0)
                    {
                        Verify.IsTrue(horizontalOffset > minHorizontalOffset, "Verifying horizontalOffset is greater than " + minHorizontalOffset);
                        Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                        Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                    // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
                }
            }
            while (additionalAttempts > 0);
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls a Rectangle in a ScrollPresenter, with the mouse wheel.")]
        public void ScrollWithMouseWheel()
        {
            ScrollWithMouseWheel(useCustomMouseWheelScrollLines: false);
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls a Rectangle in a ScrollPresenter, with the mouse wheel, using an increased WheelScrollLines OS setting.")]
        public void ScrollWithMouseWheelUsingCustomScrollLines()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                Log.Warning("This test is skipped starting with RS5 where the InteractionTracker does its own mouse wheel handling and adapts to the OS' WheelScrollLines setting automatically.");
                return;
            }

            ScrollWithMouseWheel(useCustomMouseWheelScrollLines: true);
        }

        private void ScrollWithMouseWheel(bool useCustomMouseWheelScrollLines)
        {
            const int defaultMouseWheelScrollLines = 3;
            int mouseWheelScrollLinesMultiplier = useCustomMouseWheelScrollLines ? 3 : 1;
            double minVerticalScrollPercent = 5.0 * mouseWheelScrollLinesMultiplier;

            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                if (useCustomMouseWheelScrollLines)
                {
                    SetMouseWheelScrollLines(defaultMouseWheelScrollLines * mouseWheelScrollLinesMultiplier);
                }

                GoToSimpleContentsPage();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving cmbShowScrollPresenter");
                ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                Log.Comment("Changing ScrollPresenter selection to scrollPresenter11");
                cmbShowScrollPresenter.SelectItemByName("scrollPresenter11");
                Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                Log.Comment("Retrieving ScrollPresenter");
                UIObject scrollPresenter11UIObject = FindElement.ByName("ScrollPresenter11");
                Verify.IsNotNull(scrollPresenter11UIObject, "Verifying that ScrollPresenter was found");

                WaitForScrollPresenterFinalSize(scrollPresenter11UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                Log.Comment("Scrolling ScrollPresenter with mouse wheel");
                PrepareForScrollPresenterManipulationStart("scrollPresenter11");

                InputHelper.RotateWheel(scrollPresenter11UIObject, -mouseWheelDeltaForVelocityUnit);

                Log.Comment("Waiting for scrollPresenter11 scroll completion");
                WaitForScrollPresenterManipulationEnd("scrollPresenter11");

                ScrollPresenter scrollPresenter11 = new ScrollPresenter(scrollPresenter11UIObject);

                Log.Comment("scrollPresenter11.HorizontalScrollPercent={0}", scrollPresenter11.HorizontalScrollPercent);
                Log.Comment("scrollPresenter11.VerticalScrollPercent={0}", scrollPresenter11.VerticalScrollPercent);

                if (scrollPresenter11.HorizontalScrollPercent != 0.0 || scrollPresenter11.VerticalScrollPercent <= minVerticalScrollPercent)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.AreEqual(scrollPresenter11.HorizontalScrollPercent, 0.0, "Verifying scrollPresenter11 HorizontalScrollPercent is 0%");
                Verify.IsTrue(scrollPresenter11.VerticalScrollPercent > minVerticalScrollPercent, "Verifying scrollPresenter11 VerticalScrollPercent is greater than " + minVerticalScrollPercent + "%");

                // scrollPresenter11's Content height is 1000px.
                double horizontalOffset;
                double verticalOffset;
                double minVerticalOffset = 1000.0 * (1.0 - scrollPresenter11.VerticalViewSize / 100.0) * minVerticalScrollPercent / 100.0;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);
                Verify.AreEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is 0.0");
                Verify.IsTrue(verticalOffset > minVerticalOffset, "Verifying verticalOffset is greater than " + minVerticalOffset);
                Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
            }
        }

        //[TestMethod]
        //[TestProperty("Description", "Pinch a Rectangle in a ScrollPresenter.")]
        // Disabled due to: ScrollPresenterTestsWithInputHelper Pinch/Stretch tests fail on RS5 in Helix #132
        public void PinchRectangle()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is momentarily disabled on phone (bug #12074500).");
                return;
            }

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    SetOutputDebugStringLevel("Verbose");

                    GoToSimpleContentsPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving cmbShowScrollPresenter");
                    ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                    Log.Comment("Changing ScrollPresenter selection to scrollPresenter12");
                    cmbShowScrollPresenter.SelectItemByName("scrollPresenter12");
                    Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                    Log.Comment("Retrieving ScrollPresenter");
                    UIObject scrollPresenter12UIObject = FindElement.ByName("ScrollPresenter12");
                    Verify.IsNotNull(scrollPresenter12UIObject, "Verifying that ScrollPresenter was found");

                    WaitForScrollPresenterFinalSize(scrollPresenter12UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                    Log.Comment("Zooming out the ScrollPresenter");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter12");

                    InputHelper.Pinch(scrollPresenter12UIObject);

                    Log.Comment("Waiting for scrollPresenter12 pinch completion");
                    bool success = WaitForManipulationEnd("scrollPresenter12", "txtScrollPresenterState", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    // On RS1, the offsets do not animate back to exactly 0, because of known InteractionTracker bugs that were fixed in RS2.
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                    {
                        WaitForEditValue(editName: "txtScrollPresenterHorizontalOffset", editValue: "0");
                        WaitForEditValue(editName: "txtScrollPresenterVerticalOffset", editValue: "0");
                    }

                    ScrollPresenter scrollPresenter12 = new ScrollPresenter(scrollPresenter12UIObject);
                    // scrollPresenter12's MinZoomFactor is 0.5f.
                    double horizontalOffset;
                    double verticalOffset;
                    float zoomFactor;

                    GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);

                    Log.Comment("horizontalOffset={0}", horizontalOffset);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Log.Comment("zoomFactor={0}", zoomFactor);

                    // New automation offsets are expected to be -1 because the content is now smaller than the viewport.
                    Log.Comment("scrollPresenter12.HorizontalScrollPercent={0}", scrollPresenter12.HorizontalScrollPercent);
                    Log.Comment("scrollPresenter12.VerticalScrollPercent={0}", scrollPresenter12.VerticalScrollPercent);

                    if (!success ||
                        scrollPresenter12.HorizontalScrollPercent != -1 ||
                        scrollPresenter12.VerticalScrollPercent != -1 ||
                        zoomFactor == 1.0f)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.AreEqual(scrollPresenter12.HorizontalScrollPercent, -1, "Verifying scrollPresenter12 HorizontalScrollPercent is -1");
                        Verify.AreEqual(scrollPresenter12.VerticalScrollPercent, -1, "Verifying scrollPresenter12 VerticalScrollPercent is -1");

                        // On RS1, the offsets do not animate back to exactly 0, because of known InteractionTracker bugs that were fixed in RS2.
                        if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                        {
                            Verify.AreEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is 0.0");
                            Verify.AreEqual(verticalOffset, 0.0, "Verifying verticalOffset is 0.0");
                        }
                        else
                        {
                            Verify.IsTrue(horizontalOffset <= 0.0, "Verifying horizontalOffset is not positive");
                            Verify.IsTrue(verticalOffset <= 0.0, "Verifying verticalOffset is not positive");
                        }
                        Verify.AreEqual(zoomFactor, 0.5f, "Verifying zoomFactor is 0.5f");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                    // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
                }
            }
            while (additionalAttempts > 0);
        }

        //[TestMethod]
        //[TestProperty("Description", "Stretch an Image in a ScrollPresenter.")]
        // Disable due to: ScrollPresenterTestsWithInputHelper Pinch/Stretch tests fail on RS5 in Helix #132
        public void StretchImage()
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is momentarily disabled on phone (bug #12074500).");
                return;
            }
            
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }
            
            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    SetOutputDebugStringLevel("Verbose");

                    GoToSimpleContentsPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving cmbShowScrollPresenter");
                    ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                    Log.Comment("Changing ScrollPresenter selection to scrollPresenter52");
                    cmbShowScrollPresenter.SelectItemByName("scrollPresenter52");
                    Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                    Log.Comment("Retrieving ScrollPresenter");
                    UIObject scrollPresenter52UIObject = FindElement.ByName("ScrollPresenter52");
                    Verify.IsNotNull(scrollPresenter52UIObject, "Verifying that ScrollPresenter was found");

                    WaitForScrollPresenterFinalSize(scrollPresenter52UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                    Log.Comment("Zooming in the ScrollPresenter");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter52");

                    InputHelper.Stretch(scrollPresenter52UIObject);

                    Log.Comment("Waiting for scrollPresenter52 stretch completion");
                    bool success = WaitForManipulationEnd("scrollPresenter52", "txtScrollPresenterState", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    ScrollPresenter scrollPresenter52 = new ScrollPresenter(scrollPresenter52UIObject);
                    double horizontalOffset;
                    double verticalOffset;
                    float zoomFactor;

                    GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);

                    Log.Comment("scrollPresenter52.HorizontalScrollPercent={0}", scrollPresenter52.HorizontalScrollPercent);
                    Log.Comment("scrollPresenter52.VerticalScrollPercent={0}", scrollPresenter52.VerticalScrollPercent);
                    Log.Comment("horizontalOffset={0}", horizontalOffset);
                    Log.Comment("verticalOffset={0}", verticalOffset);
                    Log.Comment("zoomFactor={0}", zoomFactor);

                    if (!success ||
                        scrollPresenter52.HorizontalScrollPercent <= 0.0 ||
                        scrollPresenter52.VerticalScrollPercent <= 0.0 ||
                        zoomFactor <= 1.0f)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.IsTrue(scrollPresenter52.HorizontalScrollPercent > 0.0, "Verifying scrollPresenter52 HorizontalScrollPercent is greater than 0%");
                        Verify.IsTrue(scrollPresenter52.VerticalScrollPercent > 0.0, "Verifying scrollPresenter52 VerticalScrollPercent is greater than 0%");
                        Verify.IsTrue(horizontalOffset > 0.0, "Verifying horizontalOffset is greater than 0.0");
                        Verify.IsTrue(verticalOffset > 0.0, "Verifying verticalOffset is greater than 0.0");
                        Verify.IsTrue(zoomFactor > 1.0f, "Verifying zoomFactor is greater than 1.0f");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                }
            }
            while (additionalAttempts > 0);
        }

        //[TestMethod]
        //[TestProperty("Description", "Pinch a Rectangle in a ScrollPresenter with the mouse wheel.")]
        // Disabled due to: ScrollPresenterTestsWithInputHelper Pinch/Stretch tests fail on RS5 in Helix #132
        public void PinchRectangleWithMouseWheel()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                GoToSimpleContentsPage();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving cmbShowScrollPresenter");
                ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                Log.Comment("Changing ScrollPresenter selection to scrollPresenter12");
                cmbShowScrollPresenter.SelectItemByName("scrollPresenter12");
                Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                Log.Comment("Retrieving ScrollPresenter");
                UIObject scrollPresenter12UIObject = FindElement.ByName("ScrollPresenter12");
                Verify.IsNotNull(scrollPresenter12UIObject, "Verifying that ScrollPresenter was found");

                WaitForScrollPresenterFinalSize(scrollPresenter12UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                Log.Comment("Zooming out the ScrollPresenter");
                PrepareForScrollPresenterManipulationStart("scrollPresenter12");

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                // Starting with 19H1, the InteractionTracker changes the scale by a factor of 1.1 for each 60 mouse wheel delta.
                // For earlier versions, a mouse wheel delta of 120 is required for the same 1.1 scale change.
                InputHelper.RotateWheel(scrollPresenter12UIObject,
                    PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5) ? (int) (-mouseWheelDeltaForVelocityUnit / 2) : -mouseWheelDeltaForVelocityUnit);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);

                Log.Comment("Waiting for scrollPresenter12 pinch completion");
                WaitForScrollPresenterManipulationEnd("scrollPresenter12");

                // On RS1, the offsets do not animate back to exactly 0, because of known InteractionTracker bugs that were fixed in RS2.
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    WaitForEditValue(editName: "txtScrollPresenterHorizontalOffset", editValue: "0");
                    WaitForEditValue(editName: "txtScrollPresenterVerticalOffset", editValue: "0");
                }

                ScrollPresenter scrollPresenter12 = new ScrollPresenter(scrollPresenter12UIObject);
                // scrollPresenter12's MinZoomFactor is 0.5f.
                double horizontalOffset;
                double verticalOffset;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);

                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);

                Log.Comment("scrollPresenter12.HorizontalScrollPercent={0}", scrollPresenter12.HorizontalScrollPercent);
                Log.Comment("scrollPresenter12.VerticalScrollPercent={0}", scrollPresenter12.VerticalScrollPercent);

                if (scrollPresenter12.HorizontalScrollPercent != 0.0 || scrollPresenter12.VerticalScrollPercent != 0.0 || zoomFactor == 1.0f)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.AreEqual(scrollPresenter12.HorizontalScrollPercent, 0.0, "Verifying scrollPresenter12 HorizontalScrollPercent is 0");
                Verify.AreEqual(scrollPresenter12.VerticalScrollPercent, 0.0, "Verifying scrollPresenter12 VerticalScrollPercent is 0");

                // On RS1, the offsets do not animate back to exactly 0, because of known InteractionTracker bugs that were fixed in RS2.
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone2))
                {
                    Verify.AreEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is 0.0");
                    Verify.AreEqual(verticalOffset, 0.0, "Verifying verticalOffset is 0.0");
                }
                else
                {
                    Verify.IsLessThanOrEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is not positive");
                    Verify.IsLessThanOrEqual(verticalOffset, 0.0, "Verifying verticalOffset is not positive");
                }
                Verify.IsGreaterThan(zoomFactor, 0.89f, "Verifying zoomFactor is about 0.1 smaller than the original 1.0");
                Verify.IsLessThan(zoomFactor, 0.91f, "Verifying zoomFactor is about 0.1 smaller than the original 1.0");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        //Test failures with keyboard/gamepad/mousewheel input #269
        //[TestMethod]
        //[TestProperty("Description", "Stretch an Image in a ScrollPresenter with the mouse wheel.")]
        public void StretchImageWithMouseWheel()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                GoToSimpleContentsPage();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving cmbShowScrollPresenter");
                ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                Log.Comment("Changing ScrollPresenter selection to scrollPresenter52");
                cmbShowScrollPresenter.SelectItemByName("scrollPresenter52");
                Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                Log.Comment("Retrieving ScrollPresenter");
                UIObject scrollPresenter52UIObject = FindElement.ByName("ScrollPresenter52");
                Verify.IsNotNull(scrollPresenter52UIObject, "Verifying that ScrollPresenter was found");

                WaitForScrollPresenterFinalSize(scrollPresenter52UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                Log.Comment("Zooming in the ScrollPresenter");
                PrepareForScrollPresenterManipulationStart("scrollPresenter52");

                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                // Starting with 19H1, the InteractionTracker changes the scale by a factor of 1.1 for each 60 mouse wheel delta.
                // For earlier versions, a mouse wheel delta of 120 is required for the same 1.1 scale change.
                InputHelper.RotateWheel(scrollPresenter52UIObject,
                    PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone5) ? mouseWheelDeltaForVelocityUnit / 2 : mouseWheelDeltaForVelocityUnit);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);

                Log.Comment("Waiting for scrollPresenter52 stretch completion");
                WaitForScrollPresenterManipulationEnd("scrollPresenter52");

                ScrollPresenter scrollPresenter52 = new ScrollPresenter(scrollPresenter52UIObject);
                double horizontalOffset;
                double verticalOffset;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);

                Log.Comment("scrollPresenter52.HorizontalScrollPercent={0}", scrollPresenter52.HorizontalScrollPercent);
                Log.Comment("scrollPresenter52.VerticalScrollPercent={0}", scrollPresenter52.VerticalScrollPercent);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);

                if (scrollPresenter52.HorizontalScrollPercent <= 0.0 || scrollPresenter52.VerticalScrollPercent <= 0.0 || zoomFactor <= 1.0f)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.IsTrue(scrollPresenter52.HorizontalScrollPercent > 0.0, "Verifying scrollPresenter52 HorizontalScrollPercent is greater than 0%");
                Verify.IsTrue(scrollPresenter52.VerticalScrollPercent > 0.0, "Verifying scrollPresenter52 VerticalScrollPercent is greater than 0%");
                Verify.IsTrue(horizontalOffset > 0.0, "Verifying horizontalOffset is greater than 0.0");
                Verify.IsTrue(verticalOffset > 0.0, "Verifying verticalOffset is greater than 0.0");
                Verify.IsGreaterThan(zoomFactor, 1.09f, "Verifying zoomFactor is about 0.1 greater than the original 1.0");
                Verify.IsLessThan(zoomFactor, 1.11f, "Verifying zoomFactor is about 0.1 greater than the original 1.0");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Reduce the Content size while it is far-anchored and overpanned.")]
        public void OverpanWithAnchoredSkrinkingContent()
        {
            OverpanWithAnchoredSkrinkingContent(isForHorizontalDirection: true);
            OverpanWithAnchoredSkrinkingContent(isForHorizontalDirection: false);
        }

        private void OverpanWithAnchoredSkrinkingContent(bool isForHorizontalDirection)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                GoToSimpleContentsPage();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving cmbShowScrollPresenter");
                ComboBox cmbShowScrollPresenter = new ComboBox(FindElement.ByName("cmbShowScrollPresenter"));

                Log.Comment("Changing ScrollPresenter selection to scrollPresenter51");
                cmbShowScrollPresenter.SelectItemByName("scrollPresenter51");
                Log.Comment("Selection is now {0}", cmbShowScrollPresenter.Selection[0].Name);

                Log.Comment("Retrieving ScrollPresenter");
                UIObject scrollPresenter51UIObject = FindElement.ByName("ScrollPresenter51");
                Verify.IsNotNull(scrollPresenter51UIObject, "Verifying that ScrollPresenter was found");

                WaitForScrollPresenterFinalSize(scrollPresenter51UIObject, expectedWidth: 300.0, expectedHeight: 400.0);

                // Tapping button before attempting pan operation to guarantee effective touch input
                TapResetViewsButton();

                Log.Comment("Panning ScrollPresenter close to the Content's end");
                PrepareForScrollPresenterManipulationStart("scrollPresenter51");

                Point startPoint = isForHorizontalDirection ? 
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 105, scrollPresenter51UIObject.BoundingRectangle.Top + 100) :
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 100, scrollPresenter51UIObject.BoundingRectangle.Top + 100);
                Point endPoint = isForHorizontalDirection ? 
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 50, scrollPresenter51UIObject.BoundingRectangle.Top + 100) :
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 100, scrollPresenter51UIObject.BoundingRectangle.Top + 50);

                InputHelper.Pan(obj: scrollPresenter51UIObject, start: startPoint, end: endPoint);

                Log.Comment("Waiting for scrollPresenter51 pan completion");
                bool success = WaitForManipulationEnd("scrollPresenter51", "txtScrollPresenterState");

                ScrollPresenter scrollPresenter51 = new ScrollPresenter(scrollPresenter51UIObject);

                Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                // No layout offset is expected
                double contentLayoutOffsetX = 0.0;
                double contentLayoutOffsetY = 0.0;

                GetScrollPresenterContentLayoutOffset(out contentLayoutOffsetX, out contentLayoutOffsetY);

                if (!success ||
                    (isForHorizontalDirection && (scrollPresenter51.HorizontalScrollPercent == 0.0 || scrollPresenter51.HorizontalScrollPercent == 100.0)) ||
                    (isForHorizontalDirection && scrollPresenter51.VerticalScrollPercent != 0.0) ||
                    (!isForHorizontalDirection && scrollPresenter51.HorizontalScrollPercent != 0.0) ||
                    (!isForHorizontalDirection && (scrollPresenter51.VerticalScrollPercent == 0.0 || scrollPresenter51.VerticalScrollPercent == 100.0)) ||
                    contentLayoutOffsetX != 0.0 ||
                    contentLayoutOffsetY != 0.0)
                {
                    LogAndClearTraces(recordWarning: false);
                }

                Log.Comment("Overpan ScrollPresenter to trigger content shrinkage");
                PrepareForScrollPresenterManipulationStart("scrollPresenter51");

                startPoint = isForHorizontalDirection ?
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 240, scrollPresenter51UIObject.BoundingRectangle.Top + 100) :
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 100, scrollPresenter51UIObject.BoundingRectangle.Top + 240);
                endPoint = isForHorizontalDirection ?
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 10, scrollPresenter51UIObject.BoundingRectangle.Top + 100) :
                        new Point(scrollPresenter51UIObject.BoundingRectangle.Left + 100, scrollPresenter51UIObject.BoundingRectangle.Top + 10);

                InputHelper.Pan(obj: scrollPresenter51UIObject, start: startPoint, end: endPoint);

                Log.Comment("Waiting for scrollPresenter51 pan completion");
                success = WaitForManipulationEnd("scrollPresenter51", "txtScrollPresenterState");

                Log.Comment("scrollPresenter51.HorizontalScrollPercent={0}", scrollPresenter51.HorizontalScrollPercent);
                Log.Comment("scrollPresenter51.VerticalScrollPercent={0}", scrollPresenter51.VerticalScrollPercent);

                // Layout offset is expected
                GetScrollPresenterContentLayoutOffset(out contentLayoutOffsetX, out contentLayoutOffsetY);

                if (!success ||
                    (isForHorizontalDirection && scrollPresenter51.HorizontalScrollPercent != 100.0) ||
                    (isForHorizontalDirection && scrollPresenter51.VerticalScrollPercent != 0.0) ||
                    (!isForHorizontalDirection && scrollPresenter51.HorizontalScrollPercent != 0.0) ||
                    (!isForHorizontalDirection && scrollPresenter51.VerticalScrollPercent != 100.0) ||
                    (isForHorizontalDirection && contentLayoutOffsetX != 30.0) ||
                    (isForHorizontalDirection && contentLayoutOffsetY != 0.0) ||
                    (!isForHorizontalDirection && contentLayoutOffsetX != 0.0) ||
                    (!isForHorizontalDirection && contentLayoutOffsetY != 40.0))
                {
                    LogAndClearTraces(recordWarning: false);
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.AreEqual(scrollPresenter51.HorizontalScrollPercent, isForHorizontalDirection ? 100.0 : 0.0, "Verifying scrollPresenter51 HorizontalScrollPercent");
                Verify.AreEqual(scrollPresenter51.VerticalScrollPercent, isForHorizontalDirection ? 0.0 : 100.0, "Verifying scrollPresenter51 VerticalScrollPercent");

                double horizontalOffset;
                double verticalOffset;
                float zoomFactor;

                GetScrollPresenterView(out horizontalOffset, out verticalOffset, out zoomFactor);
                Log.Comment("horizontalOffset={0}", horizontalOffset);
                Log.Comment("verticalOffset={0}", verticalOffset);
                Log.Comment("zoomFactor={0}", zoomFactor);

                if (isForHorizontalDirection)
                {
                    Verify.AreEqual(contentLayoutOffsetX, 30.0, "Verifying contentLayoutOffsetX is 30.0");
                    Verify.AreEqual(contentLayoutOffsetY, 0.0, "Verifying contentLayoutOffsetY is 0.0");
                    Verify.AreEqual(horizontalOffset, 470.0, "Verifying horizontalOffset is 470.0");
                    Verify.AreEqual(verticalOffset, 0.0, "Verifying verticalOffset is 0.0");
                }
                else
                {
                    Verify.AreEqual(contentLayoutOffsetX, 0.0, "Verifying contentLayoutOffsetX is 0.0");
                    Verify.AreEqual(contentLayoutOffsetY, 40.0, "Verifying contentLayoutOffsetY is 40.0");
                    Verify.AreEqual(horizontalOffset, 0.0, "Verifying horizontalOffset is 0.0");
                    Verify.AreEqual(verticalOffset, 360.0, "Verifying verticalOffset is 360.0");
                }

                Verify.AreEqual(zoomFactor, 1.0f, "Verifying zoomFactor is 1.0f");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
            }
        }

        [TestMethod]
        [TestProperty("Description", "Pans an inner ScrollViewer and chains to an outer ScrollPresenter.")]
        public void PanWithChainingFromScrollViewerToScrollPresenter()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                    {
                        Log.Comment("Skipping test on RS1 where chaining from DManip ScrollViewer is not supported.");
                        return;
                    }

                    SetOutputDebugStringLevel("Verbose");

                    GoToChainingAndRailingPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving scrollPresenter3");
                    ScrollPresenter scrollPresenter3UIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter3"));

                    Log.Comment("Retrieving scrollViewer2");
                    ScrollPresenter scrollViewer2UIObject = new ScrollPresenter(FindElement.ByName("scrollViewer2"));

                    // Tapping button before attempting pan operation to guarantee effective touch input
                    TapResetViewsButton();

                    Log.Comment("Scrolling scrollPresenter3 horizontally");
                    ScrollHelper.SetHorizontalScrollPercent(scrollPresenter3UIObject, 50.0);
                    Log.Comment("Final scrollPresenter3 HorizontalScrollPercent={0}", scrollPresenter3UIObject.HorizontalScrollPercent);

                    Log.Comment("Scrolling scrollPresenter3 vertically");
                    ScrollHelper.ScrollVertically(scrollPresenter3UIObject, ScrollAmount.SmallIncrement);
                    ScrollHelper.ScrollVertically(scrollPresenter3UIObject, ScrollAmount.SmallIncrement);
                    Log.Comment("Final scrollPresenter3 VerticalScrollPercent={0}", scrollPresenter3UIObject.VerticalScrollPercent);

                    Log.Comment("Scrolling scrollViewer2 vertically");
                    ScrollHelper.ScrollVertically(scrollViewer2UIObject, ScrollAmount.SmallIncrement);
                    ScrollHelper.ScrollVertically(scrollViewer2UIObject, ScrollAmount.SmallIncrement);
                    Log.Comment("Final scrollViewer2 VerticalScrollPercent={0}", scrollViewer2UIObject.VerticalScrollPercent);

                    Log.Comment("Panning scrollViewer2 vertically");
                    PrepareForScrollViewerManipulationStart("txtScrollViewer2State");

                    InputHelper.Pan(
                        scrollViewer2UIObject,
                        new Point(scrollViewer2UIObject.BoundingRectangle.Left + 10, scrollViewer2UIObject.BoundingRectangle.Top + 10),
                        new Point(scrollViewer2UIObject.BoundingRectangle.Left + 10, scrollViewer2UIObject.BoundingRectangle.Top + 150),
                        InputHelper.DefaultPanHoldDuration,
                        0.0025f);

                    Log.Comment("Waiting for scrollViewer2 pan completion");
                    bool success = WaitForManipulationEnd("scrollViewer2", "txtScrollViewer2State", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    Log.Comment("Final scrollViewer2 VerticalScrollPercent={0}", scrollViewer2UIObject.VerticalScrollPercent);
                    Log.Comment("Final scrollPresenter3 VerticalScrollPercent={0}", scrollPresenter3UIObject.VerticalScrollPercent);

                    if (!success ||
                        scrollViewer2UIObject.VerticalScrollPercent >= 2.0 ||
                        scrollPresenter3UIObject.VerticalScrollPercent >= 2.0)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.IsLessThan(scrollViewer2UIObject.VerticalScrollPercent, 2.0, "Verifying scrollViewer2 VerticalScrollPercent is less than 2.0%");
                        Verify.IsLessThan(scrollPresenter3UIObject.VerticalScrollPercent, 2.0, "Verifying scrollPresenter3 VerticalScrollPercent is less than 2.0%");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                }
            }
            while (additionalAttempts > 0);
        }

        [TestMethod]
        [TestProperty("Description", "Pans an inner ScrollPresenter and chains to an outer ScrollViewer.")]
        public void PanWithChainingFromScrollPresenterToScrollViewer()
        {
            // Inner ScrollPresenter uses ChainingMode.Always
            PanWithChainingFromScrollPresenterToScrollViewerWithChainingMode(useChainingModeAlways: true);

            if (PlatformConfiguration.IsOsVersionGreaterThan(OSVersion.Redstone3))
            {
                // Inner ScrollPresenter uses ChainingMode.Auto
                // Only running this case in RS4+ since the Auto behavior changed in RS4
                PanWithChainingFromScrollPresenterToScrollViewerWithChainingMode(useChainingModeAlways: false);
            }
        }

        public void PanWithChainingFromScrollPresenterToScrollViewerWithChainingMode(bool useChainingModeAlways)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                    {
                        Log.Comment("Skipping test on RS1 where chaining to DManip ScrollViewer is not supported.");
                        return;
                    }

                    SetOutputDebugStringLevel("Verbose");

                    GoToChainingAndRailingPage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving scrollPresenter3");
                    ScrollPresenter scrollPresenter3UIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter3"));

                    Log.Comment("Retrieving scrollViewer2");
                    ScrollPresenter scrollViewer2UIObject = new ScrollPresenter(FindElement.ByName("scrollViewer2"));

                    Log.Comment("Retrieving scrollPresenter1");
                    ScrollPresenter scrollPresenter1UIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter1"));

                    Log.Comment("Retrieving cmbHorizontalScrollChainingMode1");
                    ComboBox cmbHorizontalScrollChainingMode1 = new ComboBox(FindElement.ByName("cmbHorizontalScrollChainingMode1"));

                    Log.Comment("Retrieving cmbVerticalScrollChainingMode1");
                    ComboBox cmbVerticalScrollChainingMode1 = new ComboBox(FindElement.ByName("cmbVerticalScrollChainingMode1"));

                    // Tapping button before attempting pan operation to guarantee effective touch input
                    TapResetViewsButton();

                    Log.Comment("Scrolling scrollPresenter3 horizontally");
                    ScrollHelper.SetHorizontalScrollPercent(scrollPresenter3UIObject, 60.0);
                    Log.Comment("Final scrollPresenter3 HorizontalScrollPercent={0}", scrollPresenter3UIObject.HorizontalScrollPercent);

                    Log.Comment("Scrolling scrollViewer2 horizontally");
                    ScrollHelper.SetHorizontalScrollPercent(scrollViewer2UIObject, 50.0);
                    Log.Comment("Final scrollViewer2 HorizontalScrollPercent={0}", scrollViewer2UIObject.HorizontalScrollPercent);

                    Log.Comment("Scrolling scrollPresenter1 horizontally");
                    ScrollHelper.SetHorizontalScrollPercent(scrollPresenter1UIObject, 10.0);
                    Log.Comment("Final scrollPresenter1 HorizontalScrollPercent={0}", scrollPresenter1UIObject.HorizontalScrollPercent);

                    Wait.ForIdle();

                    if (useChainingModeAlways)
                    {
                        Log.Comment("Changing horizontal chaining to Always");
                        cmbHorizontalScrollChainingMode1.SelectItemByName("Always");
                        Log.Comment("Selection is now {0}", cmbHorizontalScrollChainingMode1.Selection[0].Name);

                        Log.Comment("Changing vertical chaining to Always");
                        cmbVerticalScrollChainingMode1.SelectItemByName("Always");
                        Log.Comment("Selection is now {0}", cmbVerticalScrollChainingMode1.Selection[0].Name);
                    }

                    Log.Comment("Panning scrollPresenter1 horizontally");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter1", "txtScrollPresenter1State");

                    InputHelper.Pan(
                        scrollPresenter1UIObject,
                        new Point(scrollPresenter1UIObject.BoundingRectangle.Left + 20, scrollPresenter1UIObject.BoundingRectangle.Top + 10),
                        new Point(scrollPresenter1UIObject.BoundingRectangle.Left + 300, scrollPresenter1UIObject.BoundingRectangle.Top + 10),
                        InputHelper.DefaultPanHoldDuration,
                        0.0025f);

                    Log.Comment("Waiting for scrollPresenter1 pan completion");
                    bool success = WaitForManipulationEnd("scrollPresenter1", "txtScrollPresenter1State", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    Log.Comment("Final scrollPresenter1 HorizontalScrollPercent={0}", scrollPresenter1UIObject.HorizontalScrollPercent);
                    Log.Comment("Final scrollViewer2 HorizontalScrollPercent={0}", scrollViewer2UIObject.HorizontalScrollPercent);

                    if (!success ||
                         scrollPresenter1UIObject.HorizontalScrollPercent >= 2.0 ||
                         scrollViewer2UIObject.HorizontalScrollPercent >= 2.0)
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }
                    else
                    {
                        additionalAttempts = 0;
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    if (additionalAttempts == 0)
                    {
                        Verify.IsLessThan(scrollPresenter1UIObject.HorizontalScrollPercent, 2.0, "Verifying scrollPresenter1 HorizontalScrollPercent is less than 2.0%");
                        Verify.IsLessThan(scrollViewer2UIObject.HorizontalScrollPercent, 2.0, "Verifying scrollViewer2 HorizontalScrollPercent is less than 2.0%");
                    }

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                }
            }
            while (additionalAttempts > 0);
        }

        [TestMethod]
        [TestProperty("Description", "Attempts to scroll when ManipulationMode is not System.")]
        public void ScrollWithNonSystemManipulationMode()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            // Allow the test to run a second time pre-RS4 if it failed the first time.
            int additionalAttempts = PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4) ? 1 : 2;

            do
            {
                additionalAttempts--;

                Log.Comment("Selecting ScrollPresenter tests");

                using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
                {
                    if (PlatformConfiguration.IsOsVersion(OSVersion.Redstone1))
                    {
                        Log.Comment("Skipping test on RS1 where FindElement.ByName cannot find stackPanel0.");
                        return;
                    }

                    SetOutputDebugStringLevel("Verbose");

                    GoToManipulationModePage();

                    SetLoggingLevel(isPrivateLoggingEnabled: true);

                    Log.Comment("Retrieving scrollPresenter0");
                    ScrollPresenter scrollPresenter0UIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter0"));

                    Log.Comment("Retrieving scrollPresenter1");
                    ScrollPresenter scrollPresenter1UIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter1"));

                    Log.Comment("Retrieving stackPanel0");
                    UIObject stackPanel0UIObject = FindElement.ByName("stackPanel0");
                    Verify.IsNotNull(stackPanel0UIObject, "Verifying that stackPanel0 StackPanel was found");

                    Log.Comment("Retrieving cmbManipulationModeS0");
                    ComboBox cmbManipulationModeS0 = new ComboBox(FindElement.ByName("cmbManipulationModeS0"));

                    Log.Comment("Retrieving cmbManipulationModeSP0");
                    ComboBox cmbManipulationModeSP0 = new ComboBox(FindElement.ByName("cmbManipulationModeSP0"));

                    Log.Comment("Retrieving txtScrollPresenter0State");
                    Edit scrollPresenter0StateTextBox = new Edit(FindElement.ById("txtScrollPresenter0State"));

                    Log.Comment("Retrieving txtScrollPresenter1State");
                    Edit scrollPresenter1StateTextBox = new Edit(FindElement.ById("txtScrollPresenter1State"));

                    Wait.ForIdle();

                    Log.Comment("Changing stackPanel0.ManipulationMode to None");
                    cmbManipulationModeSP0.SelectItemByName("None");
                    Log.Comment("Selection is now {0}", cmbManipulationModeSP0.Selection[0].Name);

                    Log.Comment("Attempt to pan scrollPresenter0 vertically");
                    InputHelper.Pan(
                        stackPanel0UIObject,
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 70),
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 30));

                    Wait.ForIdle();

                    Log.Comment("Current scrollPresenter0.State: " + scrollPresenter0StateTextBox.Value);
                    Verify.AreEqual(scrollPresenter0StateTextBox.Value, string.Empty);

                    Log.Comment("Current scrollPresenter1.State: " + scrollPresenter1StateTextBox.Value);
                    Verify.AreEqual(scrollPresenter1StateTextBox.Value, string.Empty);

                    Log.Comment("Changing stackPanel0.ManipulationMode back to System");
                    cmbManipulationModeSP0.SelectItemByName("System");
                    Log.Comment("Selection is now {0}", cmbManipulationModeSP0.Selection[0].Name);

                    Log.Comment("Pan scrollPresenter0 vertically");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter0", "txtScrollPresenter0State");

                    InputHelper.Pan(
                        stackPanel0UIObject,
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 70),
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 30));

                    Log.Comment("Waiting for scrollPresenter0 pan completion");
                    bool success = WaitForManipulationEnd("scrollPresenter0", "txtScrollPresenter0State", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    Log.Comment("Pan scrollPresenter0 vertically to restore original offset");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter0", "txtScrollPresenter0State");

                    InputHelper.Pan(
                        stackPanel0UIObject,
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 30),
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 70));

                    Log.Comment("Waiting for scrollPresenter0 pan completion");
                    success &= WaitForManipulationEnd("scrollPresenter0", "txtScrollPresenter0State", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    Log.Comment("Resetting scrollPresenter0StateTextBox/scrollPresenter1StateTextBox values");
                    scrollPresenter0StateTextBox.SetValueAndWait(string.Empty);
                    scrollPresenter1StateTextBox.SetValueAndWait(string.Empty);

                    Log.Comment("Changing scrollPresenter0.ManipulationMode to None");
                    cmbManipulationModeS0.SelectItemByName("None");
                    Log.Comment("Selection is now {0}", cmbManipulationModeS0.Selection[0].Name);

                    Log.Comment("Attempt to pan scrollPresenter0 vertically");
                    InputHelper.Pan(
                        stackPanel0UIObject,
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 70),
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 30));

                    Wait.ForIdle();

                    Log.Comment("Current scrollPresenter0.State: " + scrollPresenter0StateTextBox.Value);
                    if (success)
                    {
                        Verify.AreEqual(scrollPresenter0StateTextBox.Value, string.Empty);
                    }

                    Log.Comment("Current scrollPresenter1.State: " + scrollPresenter1StateTextBox.Value);
                    if (success)
                    {
                        Verify.AreEqual(scrollPresenter1StateTextBox.Value, string.Empty);
                    }

                    Log.Comment("Changing scrollPresenter0.ManipulationMode back to System");
                    cmbManipulationModeS0.SelectItemByName("System");
                    Log.Comment("Selection is now {0}", cmbManipulationModeS0.Selection[0].Name);

                    Log.Comment("Pan scrollPresenter0 vertically");
                    PrepareForScrollPresenterManipulationStart("scrollPresenter0", "txtScrollPresenter0State");

                    InputHelper.Pan(
                        stackPanel0UIObject,
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 70),
                        new Point(stackPanel0UIObject.BoundingRectangle.Left + 30, stackPanel0UIObject.BoundingRectangle.Top + 30));

                    Log.Comment("Waiting for scrollPresenter0 pan completion");
                    success &= WaitForManipulationEnd("scrollPresenter0", "txtScrollPresenter0State", failOnError: additionalAttempts == 0, logTraces: additionalAttempts == 0);

                    if (success)
                    {
                        additionalAttempts = 0;
                    }
                    else
                    {
                        LogAndClearTraces(recordWarning: additionalAttempts > 0);
                    }

                    SetLoggingLevel(isPrivateLoggingEnabled: false);

                    Log.Comment("Returning to the main ScrollPresenter test page");
                    TestSetupHelper.GoBack();
                }
            }
            while (additionalAttempts > 0);
        }

        [TestMethod]
        [TestProperty("Description", "Apply two mandatory irregular snap points to the scrollPresenter and pan to the 4 interesting zones around them.")]
        public void PanTowardsTwoManditoryIrregularSnapPoint()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Near, withTouch: true);
            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Center, withTouch: true);
            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Far, withTouch: true);
        }

        [TestMethod]
        [TestProperty("Description", "Apply two mandatory irregular snap points to the scrollPresenter and snap to them using mouse-wheel input.")]
        public void ScrollTowardsTwoManditoryIrregularSnapPoint()
        {
            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Near, withTouch: false);
            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Center, withTouch: false);
            MoveTowardsTwoManditoryIrregularSnapPoint(alignment: ScrollSnapPointsAlignment.Far, withTouch: false);
        }

        // withTouch==True, InputHelper.Pan is used to simulate touch input.
        // withTouch==False, InputHelper.RotateWheel is used to simulate mouse-wheel input.
        private void MoveTowardsTwoManditoryIrregularSnapPoint(ScrollSnapPointsAlignment alignment, bool withTouch)
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToScrollSnapPointsPage();

                int warningCount = 0;
                const double viewportHeight = 500.0;
                const double firstSnapPointOffset = 0.0;
                double secondSnapPointOffset = withTouch ? 600.0 : 150.0;
                double thirdSnapPointOffset = withTouch ? 1200.0 : 300.0;
                double firstSnapPointValue = firstSnapPointOffset;
                double secondSnapPointValue = secondSnapPointOffset;
                double thirdSnapPointValue = thirdSnapPointOffset;

                if (withTouch)
                {
                    Verify.IsTrue(PanUntilInputWorks(elements.scrollPresenterOffset, elements.scrollPresenterUIObject), "Pan inputs are moving the scrollPresenter!");
                }

                if (alignment == ScrollSnapPointsAlignment.Center)
                {
                    // Center alignment
                    firstSnapPointValue += viewportHeight / 2.0;
                    secondSnapPointValue += viewportHeight / 2.0;
                    thirdSnapPointValue += viewportHeight / 2.0;

                    Log.Comment("Changing alignment to Center");
                    elements.cmbMISnapPointAlignment.SelectItemByName("Center");
                    Log.Comment("Selection is now {0}", elements.cmbMISnapPointAlignment.Selection[0].Name);
                }
                else if (alignment == ScrollSnapPointsAlignment.Far)
                {
                    // Far alignment
                    firstSnapPointValue += viewportHeight;
                    secondSnapPointValue += viewportHeight;
                    thirdSnapPointValue += viewportHeight;

                    Log.Comment("Changing alignment to Far");
                    elements.cmbMISnapPointAlignment.SelectItemByName("Far");
                    Log.Comment("Selection is now {0}", elements.cmbMISnapPointAlignment.Selection[0].Name);
                }

                Log.Comment("Adding irregular snap point at value " + firstSnapPointValue.ToString());
                elements.txtMISnapPointValueUIObject.SetValue(firstSnapPointValue.ToString());
                elements.btnAddMISnapPointUIObject.Invoke();

                Log.Comment("Adding irregular snap point at value " + secondSnapPointValue.ToString());
                elements.txtMISnapPointValueUIObject.SetValue(secondSnapPointValue.ToString());
                elements.btnAddMISnapPointUIObject.Invoke();

                Log.Comment("Adding irregular snap point at value " + thirdSnapPointValue.ToString());
                elements.txtMISnapPointValueUIObject.SetValue(thirdSnapPointValue.ToString());
                elements.btnAddMISnapPointUIObject.Invoke();

                if (withTouch)
                {
                    InputHelper.Tap(elements.scrollPresenterUIObject);

                    InputHelper.Pan(elements.scrollPresenterUIObject, 75, Direction.North);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, secondSnapPointOffset, double.PositiveInfinity, thirdSnapPointOffset, thirdSnapPointOffset);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);
                    InputHelper.Pan(elements.scrollPresenterUIObject, 95, Direction.North);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, secondSnapPointOffset, double.PositiveInfinity, thirdSnapPointOffset, thirdSnapPointOffset);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                    InputHelper.Pan(elements.scrollPresenterUIObject, 150, Direction.North);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, thirdSnapPointOffset, double.PositiveInfinity, secondSnapPointOffset, secondSnapPointOffset);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);
                    InputHelper.Pan(elements.scrollPresenterUIObject, 200, Direction.North);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, thirdSnapPointOffset, double.PositiveInfinity, secondSnapPointOffset, secondSnapPointOffset);
                }
                else
                {
                    InputHelper.RotateWheel(elements.scrollPresenterUIObject, -mouseWheelDeltaForVelocityUnit);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, secondSnapPointOffset);
                    SnapPointsPageChangeOffset(elements, "-" + secondSnapPointOffset, 0);

                    InputHelper.RotateWheel(elements.scrollPresenterUIObject, -15 * mouseWheelDeltaForVelocityUnit);
                    warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, thirdSnapPointOffset);
                }

                Verify.IsLessThan(warningCount, 4);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a single mandatory repeated snap point across the extent, with Offset equal to Start, and pan within it.")]
        public void PanWithinARepeatedMandatorySnapPoint()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: true, alignment: ScrollSnapPointsAlignment.Near, withTouch: true);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: true, alignment: ScrollSnapPointsAlignment.Center, withTouch: true);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: true, alignment: ScrollSnapPointsAlignment.Far, withTouch: true);
        }

        [TestMethod]
        [TestProperty("Description", "Apply a single mandatory repeated snap point across the extent, with Offset different from Start, and pan within it.")]
        public void PanWithinARepeatedMandatorySnapPointWithDifferentOffset()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Near, withTouch: true);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Center, withTouch: true);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Far, withTouch: true);
        }

        [TestMethod]
        [TestProperty("Description", "Apply a single mandatory repeated snap point across the scrollPresenter extent and snap to it using mouse-wheel input.")]
        public void ScrollWithinARepeatedMandatorySnapPoint()
        {
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Near, withTouch: false);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Center, withTouch: false);
            MoveWithinARepeatedMandatorySnapPoint(withOffsetEqualToStart: false, alignment: ScrollSnapPointsAlignment.Far, withTouch: false);
        }

        // withTouch==True, InputHelper.Pan is used to simulate touch input.
        // withTouch==False, InputHelper.RotateWheel is used to simulate mouse-wheel input.
        private void MoveWithinARepeatedMandatorySnapPoint(bool withOffsetEqualToStart, ScrollSnapPointsAlignment alignment, bool withTouch)
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToScrollSnapPointsPage();

                if (withTouch)
                {
                    Verify.IsTrue(PanUntilInputWorks(elements.scrollPresenterOffset, elements.scrollPresenterUIObject), "Pan inputs aren't moving the scrollPresenter...");
                }

                const double viewportHeight = 500.0;
                const double start = 0.0;
                const double end = 9000.0;
                double interval = withTouch ? 50.0 : 400.0;
                double offset = withOffsetEqualToStart ? 0.0 : (withTouch ? 25.0 : 100.0);
                double adjustedStart = start;
                double adjustedEnd = end;
                double adjustedOffset = offset;

                if (alignment == ScrollSnapPointsAlignment.Center)
                {
                    // Center alignment
                    adjustedStart += viewportHeight / 2.0;
                    adjustedEnd += viewportHeight / 2.0;
                    adjustedOffset += viewportHeight / 2.0;

                    Log.Comment("Changing alignment to Center");
                    elements.cmbMRSnapPointAlignment.SelectItemByName("Center");
                    Log.Comment("Selection is now {0}", elements.cmbMRSnapPointAlignment.Selection[0].Name);
                }
                else if (alignment == ScrollSnapPointsAlignment.Far)
                {
                    // Far alignment
                    adjustedStart += viewportHeight;
                    adjustedEnd += viewportHeight;
                    adjustedOffset += viewportHeight;

                    Log.Comment("Changing alignment to Far");
                    elements.cmbMRSnapPointAlignment.SelectItemByName("Far");
                    Log.Comment("Selection is now {0}", elements.cmbMRSnapPointAlignment.Selection[0].Name);
                }

                Log.Comment($"Adding repeated snap point with start={adjustedStart.ToString()}, end={adjustedEnd.ToString()}, offset={adjustedOffset.ToString()}, interval={interval.ToString()}.");
                elements.txtMRSnapPointOffsetUIObject.SetValue(adjustedOffset.ToString());
                elements.txtMRSnapPointIntervalUIObject.SetValue(interval.ToString());
                elements.txtMRSnapPointStartUIObject.SetValue(adjustedStart.ToString());
                elements.txtMRSnapPointEndUIObject.SetValue(adjustedEnd.ToString());
                elements.btnAddMRSnapPointUIObject.Invoke();

                if (withTouch)
                {
                    InputHelper.Tap(elements.scrollPresenterUIObject);

                    InputHelper.Pan(elements.scrollPresenterUIObject, withOffsetEqualToStart ? 25 : 60, Direction.North);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset, 50.0);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                    InputHelper.Pan(elements.scrollPresenterUIObject, 50, Direction.North);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset, 50.0);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                    InputHelper.Pan(elements.scrollPresenterUIObject, 150, Direction.North);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset, 50.0);
                    PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                    InputHelper.Pan(elements.scrollPresenterUIObject, 200, Direction.North);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset, 50.0);
                }
                else
                {
                    InputHelper.RotateWheel(elements.scrollPresenterUIObject, -mouseWheelDeltaForVelocityUnit);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset);

                    InputHelper.RotateWheel(elements.scrollPresenterUIObject, -mouseWheelDeltaForVelocityUnit);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset + interval);

                    InputHelper.RotateWheel(elements.scrollPresenterUIObject, mouseWheelDeltaForVelocityUnit);
                    WaitForOffsetUpdated(elements.scrollPresenterOffset, offset);
                }

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

#if ApplicableRangeType
        [TestMethod]
        [TestProperty("Description", "Apply a single optional irregular snap point to the scrollPresenter and pan the scrollPresenter towards and away from the snap point.")]
        public void PanOverAnOptionalIrregularSnapPoint()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();
                Verify.IsTrue(PanUntilInputWorks(elements.scrollPresenterOffset, elements.scrollPresenterUIObject), "Pan inputs aren't moving the scrollPresenter...");

                elements.txtOISnapPointValueUIObject.SetValue("100");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();

                InputHelper.Tap(elements.scrollPresenterUIObject);

                InputHelper.Pan(elements.scrollPresenterUIObject, 75, Direction.North);
                WaitForOffsetUpdated(150.0, 10000.0, elements.scrollPresenterOffset);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a single optional irregular snap point to the scrollPresenter and pan the scrollPresenter towards and away from the snap point.")]
        public void PanTowardsASingleOptionalIrregularSnapPoint()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                int warningCount = 0;
                Verify.IsTrue(PanUntilInputWorks(elements.scrollPresenterOffset, elements.scrollPresenterUIObject), "Pan inputs aren't moving the scrollPresenter...");

                elements.txtOISnapPointValueUIObject.SetValue("600");
                elements.txtOISnapPointRangeUIObject.SetValue("500");
                elements.btnAddOISnapPointUIObject.Invoke();

                InputHelper.Tap(elements.scrollPresenterUIObject);

                elements.scrollPresenterOffset.SetValue("0");
                SnapPointsPageChangeOffset(elements, "20", 0.0, 200.0);
                PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                InputHelper.Pan(elements.scrollPresenterUIObject, 75, Direction.North);
                warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, 600.0, double.PositiveInfinity, 1100.01);
                PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                InputHelper.Pan(elements.scrollPresenterUIObject, 125, Direction.North);
                warningCount += WaitForOffsetUpdated(elements.scrollPresenterOffset, 600.0, double.PositiveInfinity, 1100.01);

                InputHelper.Pan(elements.scrollPresenterUIObject, 200, Direction.North);
                warningCount += WaitForOffsetUpdated(1100.0, 10000.0, elements.scrollPresenterOffset, 600.0, 600.0);

                Verify.IsLessThan(warningCount, 3);
                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        private void SetUpOptionalIrregularSnapPoints(
            ScrollSnapPointsTestPageElements elements,
            string previousValue,
            string previousRange,
            string snap1Value,
            string snap1Range,
            string snap2Value,
            string snap2Range,
            string nextValue,
            string nextRange)
        {
            Log.Comment("SetUpOptionalIrregularSnapPoints setting up snap points with:");
            Log.Comment("previousValue: " + previousValue + ", previousRange: " + previousRange + ", nextValue: " + nextValue + ", nextRange: " + nextRange);
            Log.Comment("snap1Value: " + snap1Value + ", snap1Range: " + snap1Range + ", snap2Value: " + snap2Value + ", snap2Range: " + snap2Range);

            if (previousValue != "" && previousRange != "")
            {
                elements.txtOISnapPointValueUIObject.SetValue(previousValue);
                elements.txtOISnapPointRangeUIObject.SetValue(previousRange);
                elements.btnAddOISnapPointUIObject.Invoke();
            }

            elements.txtOISnapPointValueUIObject.SetValue(snap1Value);
            elements.txtOISnapPointRangeUIObject.SetValue(snap1Range);
            elements.btnAddOISnapPointUIObject.Invoke();

            elements.txtOISnapPointValueUIObject.SetValue(snap2Value);
            elements.txtOISnapPointRangeUIObject.SetValue(snap2Range);
            elements.btnAddOISnapPointUIObject.Invoke();

            if (nextValue != "" && nextRange != "")
            {
                elements.txtOISnapPointValueUIObject.SetValue(nextValue);
                elements.txtOISnapPointRangeUIObject.SetValue(nextRange);
                elements.btnAddOISnapPointUIObject.Invoke();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (--|--)(--|--).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint1()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|--)(--|--)
                SetUpOptionalIrregularSnapPoints(elements, "", "", "100", "50", "200", "50", "400", "100");
                
                SnapPointsPageChangeOffset(elements, "20", 0.0, 50.0);
                SnapPointsPageChangeOffset(elements, "50", 100.0);
                SnapPointsPageChangeOffset(elements, "48", 100.0);
                SnapPointsPageChangeOffset(elements, "60", 200.0);
                SnapPointsPageChangeOffset(elements, "30", 200.0);
                SnapPointsPageChangeOffset(elements, "70", 250.0, 300.0);
                SnapPointsPageChangeOffset(elements, "50", 400.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (--|--(-)--|--).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint2()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|--)(--|--)
                SetUpOptionalIrregularSnapPoints(elements, "200", "50", "400", "100", "550", "100", "800", "100");

                SnapPointsPageChangeOffset(elements, "270", 250.0, 300.0);
                SnapPointsPageChangeOffset(elements, "50", 400.0);
                SnapPointsPageChangeOffset(elements, "20", 400.0);
                SnapPointsPageChangeOffset(elements, "60", 400.0);
                SnapPointsPageChangeOffset(elements, "90", 550.0);
                SnapPointsPageChangeOffset(elements, "80", 550.0);
                SnapPointsPageChangeOffset(elements, "120", 650.0, 700.0);
                SnapPointsPageChangeOffset(elements, "40", 800.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (-(-|--)-|----).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint3()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|--)(--|--)
                SetUpOptionalIrregularSnapPoints(elements, "550", "100", "800", "100", "950", "200", "1300", "100");

                SnapPointsPageChangeOffset(elements, "670", 650.0, 700.0);
                SnapPointsPageChangeOffset(elements, "40", 800.0);
                SnapPointsPageChangeOffset(elements, "60", 800.0);
                SnapPointsPageChangeOffset(elements, "90", 950.0);
                SnapPointsPageChangeOffset(elements, "130", 950.0);
                SnapPointsPageChangeOffset(elements, "230", 1150.0, 1200.0);
                SnapPointsPageChangeOffset(elements, "100", 1300.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (-(-|-|-)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint4()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|--)(--|--)
                SetUpOptionalIrregularSnapPoints(elements, "950", "200", "1300", "100", "1350", "100", "1800", "200");

                SnapPointsPageChangeOffset(elements, "1170", 1150.0, 1200.0);
                SnapPointsPageChangeOffset(elements, "100", 1300.0);
                SnapPointsPageChangeOffset(elements, "20", 1300.0);
                SnapPointsPageChangeOffset(elements, "40", 1350.0);
                SnapPointsPageChangeOffset(elements, "40", 1350.0);
                SnapPointsPageChangeOffset(elements, "80", 1350.0);
                SnapPointsPageChangeOffset(elements, "120", 1450.0, 1600.0);
                SnapPointsPageChangeOffset(elements, "150", 1800.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (----|-(--|-)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint5()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (----|-(--|-)-)
                SetUpOptionalIrregularSnapPoints(elements, "350", "100", "800", "200", "950", "100", "1300", "200");

                SnapPointsPageChangeOffset(elements, "520", 450.0, 600.0);
                SnapPointsPageChangeOffset(elements, "150", 800.0);
                SnapPointsPageChangeOffset(elements, "60", 800.0);
                SnapPointsPageChangeOffset(elements, "90", 950.0);
                SnapPointsPageChangeOffset(elements, "20", 950.0);
                SnapPointsPageChangeOffset(elements, "120", 1050.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "60", 1300.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (----|-(-|-)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint6()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (----|-(-|-)-)
                SetUpOptionalIrregularSnapPoints(elements, "950", "100", "1300", "200", "1400", "50", "1800", "200");

                SnapPointsPageChangeOffset(elements, "1070", 1050.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "60", 1300.0);
                SnapPointsPageChangeOffset(elements, "20", 1300.0);
                SnapPointsPageChangeOffset(elements, "60", 1400.0);
                SnapPointsPageChangeOffset(elements, "20", 1400.0);
                SnapPointsPageChangeOffset(elements, "80", 1450.0, 1600.0);
                SnapPointsPageChangeOffset(elements, "300", 1800.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (---(-|-|--)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint7()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (---(-|-|--)-)
                SetUpOptionalIrregularSnapPoints(elements, "400", "50", "800", "200", "850", "100", "1250", "100");

                SnapPointsPageChangeOffset(elements, "500", 450.0, 600.0);
                SnapPointsPageChangeOffset(elements, "300", 800.0);
                SnapPointsPageChangeOffset(elements, "20", 800.0);
                SnapPointsPageChangeOffset(elements, "40", 850.0);
                SnapPointsPageChangeOffset(elements, "70", 850.0);
                SnapPointsPageChangeOffset(elements, "140", 950.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "130", 1100.0, 1150.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration: (-(--|-|-)---).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint8()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (-(--|-|-)---)
                SetUpOptionalIrregularSnapPoints(elements, "850", "100", "1300", "200", "1250", "100", "1800", "200");

                SnapPointsPageChangeOffset(elements, "1000", 950.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "130", 1100.0, 1150.0);
                SnapPointsPageChangeOffset(elements, "50", 1250.0);
                SnapPointsPageChangeOffset(elements, "20", 1250.0);
                SnapPointsPageChangeOffset(elements, "40", 1300.0);
                SnapPointsPageChangeOffset(elements, "20", 1300.0);
                SnapPointsPageChangeOffset(elements, "250", 1500.0, 1600.0);
                SnapPointsPageChangeOffset(elements, "70", 1800.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (-(-|-)-|----).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint9()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (-(-|-)-|----)
                SetUpOptionalIrregularSnapPoints(elements, "300", "200", "800", "200", "700", "50", "1200", "100");

                SnapPointsPageChangeOffset(elements, "550", 500.0, 600.0);
                SnapPointsPageChangeOffset(elements, "70", 600.0, 650.0);
                SnapPointsPageChangeOffset(elements, "50", 700.0);
                SnapPointsPageChangeOffset(elements, "30", 700.0);
                SnapPointsPageChangeOffset(elements, "70", 800.0);
                SnapPointsPageChangeOffset(elements, "150", 800.0);
                SnapPointsPageChangeOffset(elements, "230", 1000.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "100", 1200.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (-|-)-(-|-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint10()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (-|-)-(-|-)
                SetUpOptionalIrregularSnapPoints(elements, "700", "50", "1200", "100", "1500", "100", "1900", "200");

                SnapPointsPageChangeOffset(elements, "1050", 1000.0, 1100.0);
                SnapPointsPageChangeOffset(elements, "100", 1200.0);
                SnapPointsPageChangeOffset(elements, "70", 1200.0);
                SnapPointsPageChangeOffset(elements, "120", 1300.0, 1400.0);
                SnapPointsPageChangeOffset(elements, "100", 1500.0);
                SnapPointsPageChangeOffset(elements, "70", 1500.0);
                SnapPointsPageChangeOffset(elements, "120", 1600.0, 1700.0);
                SnapPointsPageChangeOffset(elements, "100", 1900.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (--|-(-)|-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint11()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|-(-)|-)
                SetUpOptionalIrregularSnapPoints(elements, "500", "100", "900", "200", "1100", "100", "1400", "100");

                SnapPointsPageChangeOffset(elements, "650", 600.0, 700.0);
                SnapPointsPageChangeOffset(elements, "100", 900.0);
                SnapPointsPageChangeOffset(elements, "80", 900.0);
                SnapPointsPageChangeOffset(elements, "120", 1100.0);
                SnapPointsPageChangeOffset(elements, "70", 1100.0);
                SnapPointsPageChangeOffset(elements, "150", 1200.0, 1300.0);
                SnapPointsPageChangeOffset(elements, "100", 1400.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (--|(-)|-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint12()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|(-)|-)
                SetUpOptionalIrregularSnapPoints(elements, "100", "100", "400", "100", "500", "100", "900", "200");

                SnapPointsPageChangeOffset(elements, "250", 200.0, 300.0);
                SnapPointsPageChangeOffset(elements, "100", 400.0);
                SnapPointsPageChangeOffset(elements, "30", 400.0);
                SnapPointsPageChangeOffset(elements, "80", 500.0);
                SnapPointsPageChangeOffset(elements, "70", 500.0);
                SnapPointsPageChangeOffset(elements, "150", 600.0, 700.0);
                SnapPointsPageChangeOffset(elements, "200", 900.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (-(-|--)|---).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint13()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //   (-(-|--)|---)
                SetUpOptionalIrregularSnapPoints(elements, "500", "100", "900", "200", "1100", "300", "1600", "100");

                SnapPointsPageChangeOffset(elements, "650", 600.0, 700.0);
                SnapPointsPageChangeOffset(elements, "200", 900.0);
                SnapPointsPageChangeOffset(elements, "80", 900.0);
                SnapPointsPageChangeOffset(elements, "120", 1100.0);
                SnapPointsPageChangeOffset(elements, "270", 1100.0);
                SnapPointsPageChangeOffset(elements, "350", 1400.0, 1500.0);
                SnapPointsPageChangeOffset(elements, "100", 1600.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  ((-|-)|--).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint14()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  ((-|-)|--)
                SetUpOptionalIrregularSnapPoints(elements, "100", "300", "600", "100", "700", "200", "100", "50");

                SnapPointsPageChangeOffset(elements, "450", 400.0, 500.0);
                SnapPointsPageChangeOffset(elements, "100", 600.0);
                SnapPointsPageChangeOffset(elements, "30", 600.0);
                SnapPointsPageChangeOffset(elements, "80", 700.0);
                SnapPointsPageChangeOffset(elements, "180", 700.0);
                SnapPointsPageChangeOffset(elements, "250", 900.0, 1000.0);
                SnapPointsPageChangeOffset(elements, "80", 1000.0, 1050.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (-(-|-)|---).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint15()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (-(-|-)|---)
                SetUpOptionalIrregularSnapPoints(elements, "700", "200", "1100", "50", "1150", "150", "1500", "100");

                SnapPointsPageChangeOffset(elements, "950", 900.0, 1000.0);
                SnapPointsPageChangeOffset(elements, "80", 1000.0, 1050.0);
                SnapPointsPageChangeOffset(elements, "50", 1100.0);
                SnapPointsPageChangeOffset(elements, "20", 1100.0);
                SnapPointsPageChangeOffset(elements, "40", 1150.0);
                SnapPointsPageChangeOffset(elements, "100", 1150.0);
                SnapPointsPageChangeOffset(elements, "200", 1300.0, 1400.0);
                SnapPointsPageChangeOffset(elements, "100", 1500.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (-|(-)-|--).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint16()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (-|(-)-|--)
                SetUpOptionalIrregularSnapPoints(elements, "100", "50", "700", "200", "500", "100", "1150", "100");

                SnapPointsPageChangeOffset(elements, "350", 300.0, 400.0);
                SnapPointsPageChangeOffset(elements, "100", 500.0);
                SnapPointsPageChangeOffset(elements, "80", 500.0);
                SnapPointsPageChangeOffset(elements, "120", 700.0);
                SnapPointsPageChangeOffset(elements, "180", 700.0);
                SnapPointsPageChangeOffset(elements, "250", 900.0, 1000.0);
                SnapPointsPageChangeOffset(elements, "100", 1150.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (---|(--|-)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint17()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (---|(--|-)-)
                SetUpOptionalIrregularSnapPoints(elements, "200", "200", "750", "100", "650", "150", "1050", "50");

                SnapPointsPageChangeOffset(elements, "450", 400.0, 500.0);
                SnapPointsPageChangeOffset(elements, "100", 650.0);
                SnapPointsPageChangeOffset(elements, "30", 650.0);
                SnapPointsPageChangeOffset(elements, "80", 750.0);
                SnapPointsPageChangeOffset(elements, "30", 750.0);
                SnapPointsPageChangeOffset(elements, "80", 750.0);
                SnapPointsPageChangeOffset(elements, "120", 850.0, 1000.0);
                SnapPointsPageChangeOffset(elements, "150", 1050.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (--|(-|-)).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint18()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (--|(-|-))
                SetUpOptionalIrregularSnapPoints(elements, "250", "100", "550", "50", "500", "100", "900", "200");

                SnapPointsPageChangeOffset(elements, "375", 350.0, 400.0);
                SnapPointsPageChangeOffset(elements, "50", 500.0);
                SnapPointsPageChangeOffset(elements, "20", 500.0);
                SnapPointsPageChangeOffset(elements, "40", 550.0);
                SnapPointsPageChangeOffset(elements, "49", 550.0);
                SnapPointsPageChangeOffset(elements, "100", 600.0, 700.0);
                SnapPointsPageChangeOffset(elements, "100", 900.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Change Offsets around option irregular snap points in this configuration:  (---|(-|-)-).")]
        public void ChangeOffsetsBetweenInterwovenOptionalIrregularSnapPoint19()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                //  (---|(-|-)-)
                SetUpOptionalIrregularSnapPoints(elements, "550", "50", "900", "200", "950", "50", "", "");

                SnapPointsPageChangeOffset(elements, "650", 600.0, 700.0);
                SnapPointsPageChangeOffset(elements, "100", 900.0);
                SnapPointsPageChangeOffset(elements, "20", 900.0);
                SnapPointsPageChangeOffset(elements, "40", 950.0);
                SnapPointsPageChangeOffset(elements, "30", 950.0);
                SnapPointsPageChangeOffset(elements, "100", 1000.0, 1100.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetBetweenMixOfMandatoryAndOptionalSnapPoints1()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtMISnapPointValueUIObject.SetValue("100");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("200");
                elements.btnAddMISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("300");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("400");
                elements.txtOISnapPointRangeUIObject.SetValue("25");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("450");
                elements.txtOISnapPointRangeUIObject.SetValue("25");
                elements.btnAddOISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "10", 100.0);
                SnapPointsPageChangeOffset(elements, "45", 100.0);
                SnapPointsPageChangeOffset(elements, "55", 200.0);
                SnapPointsPageChangeOffset(elements, "45", 200.0);
                SnapPointsPageChangeOffset(elements, "55", 300.0);
                SnapPointsPageChangeOffset(elements, "25", 300.0);
                SnapPointsPageChangeOffset(elements, "55", 350.0, 375.0);
                SnapPointsPageChangeOffset(elements, "25", 400.0);
                SnapPointsPageChangeOffset(elements, "20", 400.0);
                SnapPointsPageChangeOffset(elements, "30", 450.0);
                SnapPointsPageChangeOffset(elements, "20", 450.0);
                SnapPointsPageChangeOffset(elements, "30", 480.0, 500.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetBetweenMixOfMandatoryAndOptionalSnapPoints2()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtMISnapPointValueUIObject.SetValue("500");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("525");
                elements.txtOISnapPointRangeUIObject.SetValue("25");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("550");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("575");
                elements.txtOISnapPointRangeUIObject.SetValue("25");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("600");
                elements.btnAddMISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "10", 500.0);
                SnapPointsPageChangeOffset(elements, "15", 525.0);
                SnapPointsPageChangeOffset(elements, "10", 525.0);
                SnapPointsPageChangeOffset(elements, "15", 550.0);
                SnapPointsPageChangeOffset(elements, "10", 550.0);
                SnapPointsPageChangeOffset(elements, "15", 575.0);
                SnapPointsPageChangeOffset(elements, "10", 575.0);
                SnapPointsPageChangeOffset(elements, "15", 600.0);
                SnapPointsPageChangeOffset(elements, "30", 600.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetBetweenMixOfMandatoryAndOptionalSnapPoints3()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtOISnapPointValueUIObject.SetValue("700");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("675");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("725");
                elements.btnAddMISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "55", 675.0);
                SnapPointsPageChangeOffset(elements, "10", 675.0);
                SnapPointsPageChangeOffset(elements, "15", 700.0);
                SnapPointsPageChangeOffset(elements, "10", 700.0);
                SnapPointsPageChangeOffset(elements, "15", 725.0);
                SnapPointsPageChangeOffset(elements, "80", 725.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetBetweenMixOfMandatoryAndOptionalSnapPoints4()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtORSnapPointOffsetUIObject.SetValue("800");
                elements.txtORSnapPointIntervalUIObject.SetValue("100");
                elements.txtORSnapPointStartUIObject.SetValue("800");
                elements.txtORSnapPointEndUIObject.SetValue("1000");
                elements.txtORSnapPointRangeUIObject.SetValue("30");
                elements.btnAddORSnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("1050");
                elements.txtOISnapPointRangeUIObject.SetValue("100");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtORSnapPointOffsetUIObject.SetValue("1100");
                elements.txtORSnapPointIntervalUIObject.SetValue("100");
                elements.txtORSnapPointStartUIObject.SetValue("1100");
                elements.txtORSnapPointEndUIObject.SetValue("1290");
                elements.txtORSnapPointRangeUIObject.SetValue("30");
                elements.btnAddORSnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "750", 700.0, 800.0);
                SnapPointsPageChangeOffset(elements, "75", 800.0);
                SnapPointsPageChangeOffset(elements, "25", 800.0);
                SnapPointsPageChangeOffset(elements, "50", 830.0, 870.0);
                SnapPointsPageChangeOffset(elements, "25", 900.0);
                SnapPointsPageChangeOffset(elements, "25", 900.0);
                SnapPointsPageChangeOffset(elements, "50", 930.0, 970.0);
                SnapPointsPageChangeOffset(elements, "25", 1000.0);
                SnapPointsPageChangeOffset(elements, "10", 1050.0);
                SnapPointsPageChangeOffset(elements, "40", 1050.0);
                SnapPointsPageChangeOffset(elements, "60", 1100.0);
                SnapPointsPageChangeOffset(elements, "25", 1100.0);
                SnapPointsPageChangeOffset(elements, "50", 1130.0, 1170.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetBetweenMixOfMandatoryAndOptionalSnapPoints5()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtORSnapPointOffsetUIObject.SetValue("100");
                elements.txtORSnapPointIntervalUIObject.SetValue("100");
                elements.txtORSnapPointStartUIObject.SetValue("100");
                elements.txtORSnapPointEndUIObject.SetValue("290");
                elements.txtORSnapPointRangeUIObject.SetValue("30");
                elements.btnAddORSnapPointUIObject.Invoke();

                elements.txtMISnapPointValueUIObject.SetValue("350");
                elements.btnAddMISnapPointUIObject.Invoke();

                elements.txtORSnapPointOffsetUIObject.SetValue("400");
                elements.txtORSnapPointIntervalUIObject.SetValue("100");
                elements.txtORSnapPointStartUIObject.SetValue("400");
                elements.txtORSnapPointEndUIObject.SetValue("615");
                elements.txtORSnapPointRangeUIObject.SetValue("30");
                elements.btnAddORSnapPointUIObject.Invoke();

                SnapPointsPageChangeOffset(elements, "60", 0.0, 100.0);
                SnapPointsPageChangeOffset(elements, "45", 100.0);
                SnapPointsPageChangeOffset(elements, "25", 100.0);
                SnapPointsPageChangeOffset(elements, "50", 130.0, 170.0);
                SnapPointsPageChangeOffset(elements, "25", 200.0);
                SnapPointsPageChangeOffset(elements, "50", 230.0, 270.0);
                SnapPointsPageChangeOffset(elements, "30", 270.0, 290.0);
                SnapPointsPageChangeOffset(elements, "12", 350.0);
                SnapPointsPageChangeOffset(elements, "45", 350.0);
                SnapPointsPageChangeOffset(elements, "60", 400.0);
                SnapPointsPageChangeOffset(elements, "25", 400.0);
                SnapPointsPageChangeOffset(elements, "50", 430.0, 470.0);
                SnapPointsPageChangeOffset(elements, "25", 500.0);
                SnapPointsPageChangeOffset(elements, "25", 500.0);
                SnapPointsPageChangeOffset(elements, "50", 530.0, 570.0);
                SnapPointsPageChangeOffset(elements, "25", 600.0);
                SnapPointsPageChangeOffset(elements, "10", 600.0);
                SnapPointsPageChangeOffset(elements, "25", 600.0, 700.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points which are defined to have the same value and scroll precisely between them.")]
        public void ChangeOffsetBetweenRepeatedSnapPointsStackedOnTopOfEachOther1()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtMRSnapPointOffsetUIObject.SetValue("600");
                elements.txtMRSnapPointIntervalUIObject.SetValue("30");
                elements.txtMRSnapPointStartUIObject.SetValue("600");
                elements.txtMRSnapPointEndUIObject.SetValue("700");
                elements.btnAddMRSnapPointUIObject.Invoke();

                elements.txtMRSnapPointOffsetUIObject.SetValue("700");
                elements.txtMRSnapPointIntervalUIObject.SetValue("30");
                elements.txtMRSnapPointStartUIObject.SetValue("700");
                elements.txtMRSnapPointEndUIObject.SetValue("780");
                elements.btnAddMRSnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("780");
                elements.txtOISnapPointRangeUIObject.SetValue("20");
                elements.btnAddOISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "575", 550.0, 600.0);
                SnapPointsPageChangeOffset(elements, "30", 600.0);
                SnapPointsPageChangeOffset(elements, "10", 600.0);
                SnapPointsPageChangeOffset(elements, "20", 630.0);
                SnapPointsPageChangeOffset(elements, "10", 630.0);
                SnapPointsPageChangeOffset(elements, "20", 660.0);
                SnapPointsPageChangeOffset(elements, "10", 660.0);
                SnapPointsPageChangeOffset(elements, "20", 690.0);
                SnapPointsPageChangeOffset(elements, "8", 690.0);
                SnapPointsPageChangeOffset(elements, "12", 700.0);
                SnapPointsPageChangeOffset(elements, "10", 700.0);
                SnapPointsPageChangeOffset(elements, "20", 730.0);
                SnapPointsPageChangeOffset(elements, "10", 730.0);
                SnapPointsPageChangeOffset(elements, "20", 760.0);
                SnapPointsPageChangeOffset(elements, "18", 760.0);
                SnapPointsPageChangeOffset(elements, "22", 780.0);
                SnapPointsPageChangeOffset(elements, "10", 780.0);
                SnapPointsPageChangeOffset(elements, "22", 800.0, 810.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points which are defined to have the same value and scroll precisely between them.")]
        public void ChangeOffsetBetweenRepeatedSnapPointsStackedOnTopOfEachOther2()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtOISnapPointValueUIObject.SetValue("780");
                elements.txtOISnapPointRangeUIObject.SetValue("20");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtMRSnapPointOffsetUIObject.SetValue("780");
                elements.txtMRSnapPointIntervalUIObject.SetValue("30");
                elements.txtMRSnapPointStartUIObject.SetValue("780");
                elements.txtMRSnapPointEndUIObject.SetValue("900");
                elements.btnAddMRSnapPointUIObject.Invoke();

                elements.txtMISnapPointValueUIObject.SetValue("900");
                elements.btnAddMISnapPointUIObject.Invoke();

                elements.txtMRSnapPointOffsetUIObject.SetValue("900");
                elements.txtMRSnapPointIntervalUIObject.SetValue("30");
                elements.txtMRSnapPointStartUIObject.SetValue("900");
                elements.txtMRSnapPointEndUIObject.SetValue("1000");
                elements.btnAddMRSnapPointUIObject.Invoke();

                elements.txtMISnapPointValueUIObject.SetValue("1100");
                elements.btnAddMISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "762", 780.0);
                SnapPointsPageChangeOffset(elements, "10", 780.0);
                SnapPointsPageChangeOffset(elements, "20", 810.0);
                SnapPointsPageChangeOffset(elements, "10", 810.0);
                SnapPointsPageChangeOffset(elements, "20", 840.0);
                SnapPointsPageChangeOffset(elements, "10", 840.0);
                SnapPointsPageChangeOffset(elements, "20", 870.0);
                SnapPointsPageChangeOffset(elements, "10", 870.0);
                SnapPointsPageChangeOffset(elements, "20", 900.0);
                SnapPointsPageChangeOffset(elements, "10", 900.0);
                SnapPointsPageChangeOffset(elements, "20", 930.0);
                SnapPointsPageChangeOffset(elements, "65", 990.0);
                SnapPointsPageChangeOffset(elements, "12", 1100.0);
                SnapPointsPageChangeOffset(elements, "10", 1100.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points which are defined to have the same value and scroll precisely between them.")]
        public void ChangeOffsetBetweenRepeatedSnapPointsStackedOnTopOfEachOther3()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtMISnapPointValueUIObject.SetValue("1100");
                elements.btnAddMISnapPointUIObject.Invoke();

                elements.txtMRSnapPointOffsetUIObject.SetValue("1100");
                elements.txtMRSnapPointIntervalUIObject.SetValue("30");
                elements.txtMRSnapPointStartUIObject.SetValue("1100");
                elements.txtMRSnapPointEndUIObject.SetValue("1200");
                elements.btnAddMRSnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("1200");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "12", 1100.0);
                SnapPointsPageChangeOffset(elements, "10", 1100.0);
                SnapPointsPageChangeOffset(elements, "20", 1130.0);
                SnapPointsPageChangeOffset(elements, "10", 1130.0);
                SnapPointsPageChangeOffset(elements, "20", 1160.0);
                SnapPointsPageChangeOffset(elements, "10", 1160.0);
                SnapPointsPageChangeOffset(elements, "20", 1190.0);
                SnapPointsPageChangeOffset(elements, "8", 1190.0);
                SnapPointsPageChangeOffset(elements, "15", 1200.0);
                SnapPointsPageChangeOffset(elements, "45", 1200.0);
                SnapPointsPageChangeOffset(elements, "60", 1250.0, 1300.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points which are defined to have the same value and scroll precisely between them.")]
        public void ChangeOffsetBetweenSnapPointsStackedOnTopOfEachOther()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 OS due to interaction tracker issues that were fixed in RS2 that cause ChangeOffsets to not be reliable enough for this test.");
                return;
            }

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                elements.txtMISnapPointValueUIObject.SetValue("100");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("100");
                elements.txtOISnapPointRangeUIObject.SetValue("20");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtMISnapPointValueUIObject.SetValue("200");
                elements.btnAddMISnapPointUIObject.Invoke();
                elements.txtMISnapPointValueUIObject.SetValue("200");
                elements.btnAddMISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("300");
                elements.txtOISnapPointRangeUIObject.SetValue("20");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("300");
                elements.txtOISnapPointRangeUIObject.SetValue("30");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("400");
                elements.txtOISnapPointRangeUIObject.SetValue("30");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("400");
                elements.txtOISnapPointRangeUIObject.SetValue("20");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("500");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();
                elements.txtOISnapPointValueUIObject.SetValue("500");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();
                
                SnapPointsPageChangeOffset(elements, "10", 100.0);
                SnapPointsPageChangeOffset(elements, "10", 100.0);
                SnapPointsPageChangeOffset(elements, "45", 100.0);
                SnapPointsPageChangeOffset(elements, "55", 200.0);
                SnapPointsPageChangeOffset(elements, "45", 200.0);
                SnapPointsPageChangeOffset(elements, "75", 300.0);
                SnapPointsPageChangeOffset(elements, "10", 300.0);
                SnapPointsPageChangeOffset(elements, "25", 300.0);
                SnapPointsPageChangeOffset(elements, "50", 330.0, 370.0);
                SnapPointsPageChangeOffset(elements, "25", 400.0);
                SnapPointsPageChangeOffset(elements, "10", 400.0);
                SnapPointsPageChangeOffset(elements, "25", 400.0);
                SnapPointsPageChangeOffset(elements, "35", 430.0, 450.0);
                SnapPointsPageChangeOffset(elements, "25", 500.0);
                SnapPointsPageChangeOffset(elements, "25", 500.0);
                SnapPointsPageChangeOffset(elements, "60", 550.0, 600.0);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a single optional repeated snap point across the extent and pan within it.")]
        public void PanWithinARepeatedOptionalSnapPoint()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();

                Verify.IsTrue(PanUntilInputWorks(elements.scrollPresenterOffset, elements.scrollPresenterUIObject), "Pan inputs aren't moving the scrollPresenter...");

                elements.txtORSnapPointOffsetUIObject.SetValue("0");
                elements.txtORSnapPointIntervalUIObject.SetValue("100");
                elements.txtORSnapPointStartUIObject.SetValue("0");
                elements.txtORSnapPointEndUIObject.SetValue("9000");
                elements.txtORSnapPointRangeUIObject.SetValue("25");
                elements.btnAddORSnapPointUIObject.Invoke();

                InputHelper.Tap(elements.scrollPresenterUIObject);

                InputHelper.Pan(elements.scrollPresenterUIObject, 25, Direction.North);
                WaitForOptionalRepeatedOffsetUpdated(elements.scrollPresenterOffset, 25, 100);
                PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                InputHelper.Pan(elements.scrollPresenterUIObject, 75, Direction.North);
                WaitForOptionalRepeatedOffsetUpdated(elements.scrollPresenterOffset, 25, 100);
                PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                InputHelper.Pan(elements.scrollPresenterUIObject, 150, Direction.North);
                WaitForOptionalRepeatedOffsetUpdated(elements.scrollPresenterOffset, 25, 100);
                PanToZero(elements.scrollPresenterUIObject, elements.scrollPresenterOffset);

                InputHelper.Pan(elements.scrollPresenterUIObject, 200, Direction.North);
                WaitForOptionalRepeatedOffsetUpdated(elements.scrollPresenterOffset, 25, 100);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Apply a mix of mandatory and optional snap points and scroll precisely between them.")]
        public void ChangeOffsetWithSnapPoints()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                var elements = GoToSnapPointsPage();
                
                Log.Comment("Retrieving btnOffsetPlus10With");
                Button btnOffsetPlus10WithUIObject = new Button(FindElement.ByName("btnOffsetPlus10With"));

                Log.Comment("Retrieving btnOffsetPlus10Without");
                Button btnOffsetPlus10WithoutUIObject = new Button(FindElement.ByName("btnOffsetPlus10Without"));

                elements.txtOISnapPointValueUIObject.SetValue("50");
                elements.txtOISnapPointRangeUIObject.SetValue("45");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("100");
                elements.txtOISnapPointRangeUIObject.SetValue("50");
                elements.btnAddOISnapPointUIObject.Invoke();

                elements.txtOISnapPointValueUIObject.SetValue("101");
                elements.txtOISnapPointRangeUIObject.SetValue("8");
                elements.btnAddOISnapPointUIObject.Invoke();

                Wait.ForIdle();
                btnOffsetPlus10WithoutUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 10.0);

                Wait.ForIdle();
                btnOffsetPlus10WithUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 50.0);
 
                Wait.ForIdle();
                btnOffsetPlus10WithUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 50.0);

                Wait.ForIdle();
                btnOffsetPlus10WithoutUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 60.0);

                Wait.ForIdle();
                btnOffsetPlus10WithoutUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 70.0);

                Wait.ForIdle();
                btnOffsetPlus10WithUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 100.0);

                Wait.ForIdle();
                btnOffsetPlus10WithUIObject.Invoke();
                WaitForOffsetUpdated(elements.scrollPresenterOffset, 110.0);

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        private void SnapPointsPageChangeOffset(ScrollSnapPointsTestPageElements elements, String amount, double minValue, double maxValue)
        {
            Log.Comment("SnapPointsPageChangeOffset with amount: " + amount + ", minValue: " + minValue + ", maxValue: " + maxValue);

            elements.scrollPresenterOffsetChangeAmount.SetValue(amount);
            elements.changeScrollPresenterOffset.Invoke();
            WaitForOffsetUpdated(minValue, maxValue, elements.scrollPresenterOffset);
        }
#endif

        private void SnapPointsPageChangeOffset(ScrollSnapPointsTestPageElements elements, String amount, double value)
        {
            Log.Comment("SnapPointsPageChangeOffset with amount: " + amount + ", value: " + value);

            elements.scrollPresenterOffsetChangeAmount.SetValue(amount);
            elements.changeScrollPresenterOffset.Invoke();
            WaitForOffsetUpdated(elements.scrollPresenterOffset, value);
        }

        private void GoToSimpleContentsPage()
        {
            Log.Comment("Navigating to ScrollPresentersWithSimpleContentsPage");
            UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
            Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

            Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
            navigateToSimpleContentsButton.Invoke();
            Wait.ForIdle();
        }

        private void GoToChainingAndRailingPage()
        {
            Log.Comment("Navigating to ScrollPresenterChainingAndRailingPage");
            UIObject navigateToChainingAndRailingUIObject = FindElement.ByName("navigateToChainingAndRailing");
            Verify.IsNotNull(navigateToChainingAndRailingUIObject, "Verifying that navigateToChainingAndRailing Button was found");

            Button navigateToChainingAndRailingButton = new Button(navigateToChainingAndRailingUIObject);
            navigateToChainingAndRailingButton.Invoke();
            Wait.ForIdle();
        }

        private void GoToManipulationModePage()
        {
            Log.Comment("Navigating to ScrollPresenterManipulationModePage");
            UIObject navigateToManipulationModeUIObject = FindElement.ByName("navigateToManipulationMode");
            Verify.IsNotNull(navigateToManipulationModeUIObject, "Verifying that navigateToManipulationMode Button was found");

            Button navigateToManipulationModeButton = new Button(navigateToManipulationModeUIObject);
            navigateToManipulationModeButton.Invoke();
            Wait.ForIdle();
        }

        private ScrollSnapPointsTestPageElements GoToScrollSnapPointsPage()
        {
            Log.Comment("Navigating to ScrollSnapPointsPage");
            UIObject navigateToScrollSnapPointsUIObject = FindElement.ByName("navigateToScrollSnapPoints");
            Verify.IsNotNull(navigateToScrollSnapPointsUIObject, "Verifying that navigateToScrollSnapPoints Button was found");

            Button navigateToScrollSnapPointsButton = new Button(navigateToScrollSnapPointsUIObject);
            navigateToScrollSnapPointsButton.Invoke();
            Wait.ForIdle();

            return GatherScrollSnapPointsTestPageElements();
        }

        private int WaitForOffsetUpdated(
            Edit text,
            double expected,
            double modulo = double.PositiveInfinity,
            double alternateMin = double.PositiveInfinity,
            double alternateMax = double.PositiveInfinity,
            double millisecondsTimeout = defaultAnimatedViewChangeTimeout,
            bool failOnError = true)
        {
            return WaitForOffsetUpdated(
                expected,
                expected,
                text,
                alternateMin,
                alternateMax,
                modulo,
                millisecondsTimeout,
                failOnError);
        }

        private int WaitForOffsetUpdated(
            double expectedMin,
            double expectedMax,
            Edit text,
            double alternateMin = double.PositiveInfinity,
            double alternateMax = double.PositiveInfinity,
            double modulo = double.PositiveInfinity,
            double millisecondsTimeout = defaultAnimatedViewChangeTimeout,
            bool failOnError = true)
        {
            Log.Comment("WaitForOffsetUpdated with expectedMin: " + expectedMin + ", expectedMax: " + expectedMax);

            int warningCount = 0;
            bool success = WaitForOffsetToSettle(text, millisecondsTimeout, failOnError);
            double value = Convert.ToDouble(text.Value) % modulo;
            bool goodValue = false;
            if (value >= expectedMin && value <= expectedMax)
            {
                goodValue = true;
            }
            else if (value >= alternateMin && value <= alternateMax)
            {
                Log.Warning("Passing based on alternate value, this should be relatively rare.");
                warningCount++;
                goodValue = true;
            }
            Verify.IsTrue(goodValue);
            return warningCount;
        }

#if ApplicableRangeType
        private void WaitForOptionalRepeatedOffsetUpdated(
            Edit text,
            double range,
            double interval,
            double millisecondsTimeout = defaultAnimatedViewChangeTimeout,
            bool failOnError = true)
        {
            bool success = WaitForOffsetToSettle(text, millisecondsTimeout, failOnError);
            Log.Comment("Final Offset value: " + text.Value);
            double valueModuloInterval = Convert.ToDouble(text.Value) % interval;
            Log.Comment("Final Offset value modulo interval " + interval + ": " + valueModuloInterval);
            Verify.IsTrue((valueModuloInterval > range && valueModuloInterval < interval - range) || valueModuloInterval == 0.0);
        }
#endif

        private bool WaitForOffsetToSettle(Edit text, double millisecondsTimeout, bool failOnError)
        {
            Wait.ForIdle();

            const double millisecondsNormalStepTimeout = 100;
            const double millisecondsIdleStepTimeout = 600;
            Edit elementStateTextBox = new Edit(FindElement.ById("txtScrollPresenterState"));
            ValueChangedEventWaiter waiter = new ValueChangedEventWaiter(text);
            int unsuccessfulWaits = 0;
            int maxUnsuccessfulWaits = (int)(millisecondsIdleStepTimeout / millisecondsNormalStepTimeout);

            Log.Comment("Original State: " + elementStateTextBox.Value);
            Log.Comment("Original Offset: " + text.Value);

            // When the initial State is still Idle, use a longer timeout to allow it to transition out of Idle.
            double millisecondsWait = (elementStateTextBox.Value == "Idle") ? millisecondsIdleStepTimeout : millisecondsNormalStepTimeout;
            double millisecondsCumulatedWait = 0;

            do
            {
                Log.Comment("Waiting for Offset change.");
                waiter.Reset();
                if (waiter.TryWait(TimeSpan.FromMilliseconds(millisecondsWait)))
                {
                    unsuccessfulWaits = 0;
                }
                else
                {
                    unsuccessfulWaits++;
                }
                millisecondsCumulatedWait += millisecondsWait;
                millisecondsWait = millisecondsNormalStepTimeout;

                Log.Comment("Current State: " + elementStateTextBox.Value);
                Log.Comment("Current Offset: " + text.Value);

                Wait.ForIdle();
            }
            while (elementStateTextBox.Value != "Idle" &&
                   millisecondsCumulatedWait < millisecondsTimeout &&
                   unsuccessfulWaits <= maxUnsuccessfulWaits);

            if (elementStateTextBox.Value == "Idle")
            {
                Log.Comment("Idle State reached after " + millisecondsCumulatedWait + " out of " + millisecondsTimeout + " milliseconds. Final Offset: " + text.Value);
                return true;
            }
            else
            {
                LogAndClearTraces();
                string message = unsuccessfulWaits > maxUnsuccessfulWaits ? 
                    "Offset has not changed within " + millisecondsIdleStepTimeout + " milliseconds outside of Idle State." :
                    "Idle State was not reached within " + millisecondsTimeout + " milliseconds.";
                if (failOnError)
                {
                    Log.Error(message);
                }
                else
                {
                    Log.Warning(message);
                }

                return false;
            }
        }

        private bool PanUntilInputWorks(Edit text, UIObject scrollPresenterUIObject)
        {
            int tries = 0;
            do
            {
                InputHelper.Pan(scrollPresenterUIObject, 25, Direction.North);
                WaitForOffsetToSettle(text, millisecondsTimeout: 6000, failOnError: true);
                double value = Convert.ToDouble(text.Value);
                if (value != 0)
                {
                    PanToZero(scrollPresenterUIObject, text);
                    return true;
                }
                tries++;
            }
            while (tries < 10);
            return false;
        }

        private void TapResetViewsButton()
        {
            Log.Comment("Retrieving btnResetViews");
            UIObject resetViewsUIObject = FindElement.ByName("btnResetViews");
            Verify.IsNotNull(resetViewsUIObject, "Verifying that btnResetViews Button was found");

            Button resetViewsButton = new Button(resetViewsUIObject);
            InputHelper.Tap(resetViewsButton);
            if (!WaitForEditValue(editName: "txtResetStatus", editValue: "Views reset", secondsTimeout: 2.0, throwOnError: false))
            {
                InputHelper.Tap(resetViewsButton);
                WaitForEditValue(editName: "txtResetStatus", editValue: "Views reset");
            }
        }

        private void GetScrollPresenterContentLayoutOffset(out double contentLayoutOffsetX, out double contentLayoutOffsetY)
        {
            contentLayoutOffsetX = 0.0;
            contentLayoutOffsetY = 0.0;

            UIObject viewUIObject = FindElement.ById("txtScrollPresenterContentLayoutOffsetX");
            Edit viewTextBox = new Edit(viewUIObject);
            Log.Comment($"Current ContentLayoutOffsetX: {viewTextBox.Value}");
            contentLayoutOffsetX = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollPresenterContentLayoutOffsetY");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment($"Current ContentLayoutOffsetY: {viewTextBox.Value}");
            contentLayoutOffsetY = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);
        }

        private void GetScrollPresenterView(out double horizontalOffset, out double verticalOffset, out float zoomFactor)
        {
            horizontalOffset = 0.0;
            verticalOffset = 0.0;
            zoomFactor = 1.0f;

            UIObject viewUIObject = FindElement.ById("txtScrollPresenterHorizontalOffset");
            Edit viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current HorizontalOffset: " + viewTextBox.Value);
            horizontalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollPresenterVerticalOffset");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current VerticalOffset: " + viewTextBox.Value);
            verticalOffset = String.IsNullOrWhiteSpace(viewTextBox.Value) ? double.NaN : Convert.ToDouble(viewTextBox.Value);

            viewUIObject = FindElement.ById("txtScrollPresenterZoomFactor");
            viewTextBox = new Edit(viewUIObject);
            Log.Comment("Current ZoomFactor: " + viewTextBox.Value);
            zoomFactor = String.IsNullOrWhiteSpace(viewTextBox.Value) ? float.NaN : Convert.ToSingle(viewTextBox.Value);
        }

        private void PrepareForScrollPresenterManipulationStart(string scrollPresenterName, string stateTextBoxName = "txtScrollPresenterState")
        {
            UIObject scrollPresenterStateUIObject = FindElement.ById(stateTextBoxName);
            Edit scrollPresenterStateTextBox = new Edit(scrollPresenterStateUIObject);
            Log.Comment("Pre-manipulation ScrollPresenterState: " + scrollPresenterStateTextBox.Value);
            Wait.ForIdle();
        }

        private void WaitForScrollPresenterManipulationEnd(string scrollPresenterName, string stateTextBoxName = "txtScrollPresenterState")
        {
            WaitForManipulationEnd(scrollPresenterName, stateTextBoxName);
        }

        private bool TryWaitForScrollPresenterManipulationEnd(string scrollPresenterName, string stateTextBoxName = "txtScrollPresenterState")
        {
            return WaitForManipulationEnd(scrollPresenterName, stateTextBoxName, failOnError: false);
        }

        private void PrepareForScrollViewerManipulationStart(string stateTextBoxName = "txtScrollViewerState")
        {
            UIObject scrollViewerStateUIObject = FindElement.ById(stateTextBoxName);
            Edit scrollViewerStateTextBox = new Edit(scrollViewerStateUIObject);
            Log.Comment("Pre-manipulation ScrollViewerState: " + scrollViewerStateTextBox.Value);
            Wait.ForIdle();
        }

        private void WaitForScrollViewerManipulationEnd(string scrollViewerName, string stateTextBoxName = "txtScrollViewerState")
        {
            WaitForManipulationEnd(scrollViewerName, stateTextBoxName);
        }

        private bool WaitForManipulationEnd(string elementName, string stateTextBoxName, bool failOnError = true, bool logTraces = true)
        {
            UIObject elementStateUIObject = FindElement.ById(stateTextBoxName);
            Edit elementStateTextBox = new Edit(elementStateUIObject);
            Log.Comment("Current State: " + elementStateTextBox.Value);
            if (elementStateTextBox.Value != elementName + " Idle")
            {
                using (var waiter = new ValueChangedEventWaiter(elementStateTextBox, elementName + " Idle"))
                {
                    int loops = 0;

                    Log.Comment("Waiting for " + elementName + "'s manipulation end.");
                    while (true)
                    {
                        bool success = waiter.TryWait(TimeSpan.FromMilliseconds(250));

                        Log.Comment("Current State: " + elementStateTextBox.Value);

                        if (success)
                        {
                            Log.Comment("Wait succeeded");
                            break;
                        }
                        else
                        {
                            if (elementStateTextBox.Value == elementName + " Idle")
                            {
                                Log.Warning("Wait failed but TextBox contains expected text");
                                if (logTraces)
                                {
                                    LogAndClearTraces();
                                }
                                break;
                            }
                            else if (loops < 20)
                            {
                                loops++;
                                waiter.Reset();
                            }
                            else
                            {
                                if (logTraces)
                                {
                                    LogAndClearTraces();
                                }
                                string message = "Wait for manipulation end failed";
                                if (failOnError)
                                {
                                    Log.Error(message);
                                }
                                else
                                {
                                    Log.Warning(message);
                                }
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        private void WaitForScrollPresenterFinalSize(UIObject scrollPresenterUIObject, double expectedWidth, double expectedHeight)
        {
            int pauses = 0;
            int widthDelta = Math.Abs(scrollPresenterUIObject.BoundingRectangle.Width - (int)expectedWidth);
            int heightDelta = Math.Abs(scrollPresenterUIObject.BoundingRectangle.Height - (int)expectedHeight);

            Log.Comment("scrollPresenterUIObject.BoundingRectangle={0}", scrollPresenterUIObject.BoundingRectangle);

            while ((widthDelta > 1 || heightDelta > 1) && pauses < 5)
            {
                Wait.ForMilliseconds(60);
                pauses++;
                Log.Comment("scrollPresenterUIObject.BoundingRectangle={0}", scrollPresenterUIObject.BoundingRectangle);
                widthDelta = Math.Abs(scrollPresenterUIObject.BoundingRectangle.Width - (int)expectedWidth);
                heightDelta = Math.Abs(scrollPresenterUIObject.BoundingRectangle.Height - (int)expectedHeight);
            };

            Verify.IsLessThanOrEqual(widthDelta, 1);
            Verify.IsLessThanOrEqual(heightDelta, 1);
        }

        private void PanToZero(UIObject scrollPresenterUIObject, Edit text, int maxTries = 10)
        {
            Log.Comment("Panning ScrollPresenter to reset its offset.");
            int tries = 0;
            do
            {
                InputHelper.Pan(scrollPresenterUIObject, Math.Max((int) Convert.ToDouble(text.Value) / 2, 25), Direction.South);
                WaitForOffsetToSettle(text, millisecondsTimeout: defaultAnimatedViewChangeTimeout, failOnError: false);
                tries++;
            }
            while (Convert.ToDouble(text.Value) != 0.0 && tries < maxTries);
            Verify.AreEqual(0.0, Convert.ToDouble(text.Value));
        }

        private ScrollSnapPointsTestPageElements GatherScrollSnapPointsTestPageElements()
        {
            Log.Comment("GatherScrollSnapPointsTestPageElements - entry");
            var elements = new ScrollSnapPointsTestPageElements();
            
            elements.btnAddMISnapPointUIObject = new Button(FindElement.ByName("btnMIAddSnapPoint"));
            elements.txtMISnapPointValueUIObject = new Edit(FindElement.ByName("txtMISnapPointValue"));
            elements.cmbMISnapPointAlignment = new ComboBox(FindElement.ByName("cmbMISnapPointAlignment"));

#if ApplicableRangeType
            elements.btnAddOISnapPointUIObject = new Button(FindElement.ByName("btnOIAddSnapPoint"));
            elements.txtOISnapPointValueUIObject = new Edit(FindElement.ByName("txtOISnapPointValue"));
            elements.txtOISnapPointRangeUIObject = new Edit(FindElement.ByName("txtOIApplicableRange"));
#endif

            elements.btnAddMRSnapPointUIObject = new Button(FindElement.ByName("btnMRAddSnapPoint"));
            elements.txtMRSnapPointOffsetUIObject = new Edit(FindElement.ByName("txtMRSnapPointOffset"));
            elements.txtMRSnapPointIntervalUIObject = new Edit(FindElement.ByName("txtMRSnapPointInterval"));
            elements.txtMRSnapPointStartUIObject = new Edit(FindElement.ByName("txtMRSnapPointStart"));
            elements.txtMRSnapPointEndUIObject = new Edit(FindElement.ByName("txtMRSnapPointEnd"));
            elements.cmbMRSnapPointAlignment = new ComboBox(FindElement.ByName("cmbMRSnapPointAlignment"));

#if ApplicableRangeType
            elements.btnAddORSnapPointUIObject = new Button(FindElement.ByName("btnORAddSnapPoint"));
            elements.txtORSnapPointOffsetUIObject = new Edit(FindElement.ByName("txtORSnapPointOffset"));
            elements.txtORSnapPointIntervalUIObject = new Edit(FindElement.ByName("txtORSnapPointInterval"));
            elements.txtORSnapPointStartUIObject = new Edit(FindElement.ByName("txtORSnapPointStart"));
            elements.txtORSnapPointEndUIObject = new Edit(FindElement.ByName("txtORSnapPointEnd"));
            elements.txtORSnapPointRangeUIObject = new Edit(FindElement.ByName("txtORApplicableRange"));
#endif

            elements.scrollPresenterUIObject = FindElement.ByName("markupScrollPresenter");
            Verify.IsNotNull(elements.scrollPresenterUIObject, "Verifying that markupScrollPresenter was found");

            elements.scrollPresenterOffset = new Edit(FindElement.ById("txtScrollPresenterOffset"));
            elements.scrollPresenterOffsetChangeAmount = new Edit(FindElement.ById("txtScrollPresenterOffsetChange"));
            elements.changeScrollPresenterOffset = new Button(FindElement.ById("btnScrollPresenterOffsetChange"));

            elements.scrollPresenterOffset.SetValue("0");

            Log.Comment("GatherScrollSnapPointsTestPageElements - exit");
            return elements;
        }

        private struct ScrollSnapPointsTestPageElements
        {
            public Button btnAddMISnapPointUIObject;
            public Edit txtMISnapPointValueUIObject;
            public ComboBox cmbMISnapPointAlignment;

#if ApplicableRangeType
            public Button btnAddOISnapPointUIObject;
            public Edit txtOISnapPointValueUIObject;
            public Edit txtOISnapPointRangeUIObject;
#endif

            public Button btnAddMRSnapPointUIObject;
            public Edit txtMRSnapPointOffsetUIObject;
            public Edit txtMRSnapPointIntervalUIObject;
            public Edit txtMRSnapPointStartUIObject;
            public Edit txtMRSnapPointEndUIObject;
            public ComboBox cmbMRSnapPointAlignment;

#if ApplicableRangeType
            public Button btnAddORSnapPointUIObject;
            public Edit txtORSnapPointOffsetUIObject;
            public Edit txtORSnapPointIntervalUIObject;
            public Edit txtORSnapPointStartUIObject;
            public Edit txtORSnapPointEndUIObject;
            public Edit txtORSnapPointRangeUIObject;
#endif

            public UIObject scrollPresenterUIObject;

            public Edit scrollPresenterOffset;
            public Edit scrollPresenterOffsetChangeAmount;
            public Button changeScrollPresenterOffset;
        }
    }
}
