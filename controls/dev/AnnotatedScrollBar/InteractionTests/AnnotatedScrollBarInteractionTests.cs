// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class AnnotatedScrollBarInteractionTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // AnnotatedScrollBarScrollingEventArgs reports incorrect scrolloffset when using arrow buttons
        public void ScrollUsingArrowButtons()
        {
            Log.Comment("AnnotatedScrollBar ScrollUsingDecrementArrowButton Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving txtValue");
                TextBlock txtValue = new TextBlock(FindElement.ById("txtValue"));
                Verify.IsNotNull(txtValue, "Verifying that txtValue was found");

                Log.Comment("Retrieving btnGetValue");
                Button btnGetValue = new Button(FindElement.ById("btnGetValue"));
                Verify.IsNotNull(btnGetValue, "Verifying that btnGetValue was found");

                Log.Comment("Retrieving VerticalDecrementRepeatButton");
                Button VerticalDecrementRepeatButton = new Button(FindElement.ById("PART_VerticalDecrementRepeatButton"));
                Verify.IsNotNull(VerticalDecrementRepeatButton, "Verifying that VerticalDecrementRepeatButton was found");

                Log.Comment("Retrieving VerticalIncrementRepeatButton");
                Button VerticalIncrementRepeatButton = new Button(FindElement.ById("PART_VerticalIncrementRepeatButton"));
                Verify.IsNotNull(VerticalIncrementRepeatButton, "Verifying that VerticalIncrementRepeatButton was found");

                Log.Comment("Verify Value is set to 0.");
                btnGetValue.Click();
                Verify.AreEqual(0.0, Convert.ToDouble(txtValue.DocumentText));

                Log.Comment("Click VerticalDecrementRepeatButton.");
                VerticalDecrementRepeatButton.Click();

                Log.Comment("Verify Value is more than 0.");
                btnGetValue.Click();
                var newOffsetValue = Convert.ToDouble(txtValue.DocumentText);
                Verify.IsTrue(newOffsetValue > 0.0);

                Log.Comment("Click VerticalIncrementRepeatButton.");
                VerticalIncrementRepeatButton.Click();

                Log.Comment("Verify Value is back at 0.");
                btnGetValue.Click();
                Verify.AreEqual(0.0, Convert.ToDouble(txtValue.DocumentText));
            }

        }

        [TestMethod]
        public void ScrollUsingClick()
        {
            Log.Comment("AnnotatedScrollBar ScrollUsingClick Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving txtValue");
                TextBlock txtValue = new TextBlock(FindElement.ById("txtValue"));
                Verify.IsNotNull(txtValue, "Verifying that txtValue was found");

                Log.Comment("Retrieving btnGetValue");
                Button btnGetValue = new Button(FindElement.ById("btnGetValue"));
                Verify.IsNotNull(btnGetValue, "Verifying that btnGetValue was found");

                Log.Comment("Retrieving VerticalIncrementRepeatButton");
                Button VerticalIncrementRepeatButton = new Button(FindElement.ById("PART_VerticalIncrementRepeatButton"));
                Verify.IsNotNull(VerticalIncrementRepeatButton, "Verifying that VerticalIncrementRepeatButton was found");

                Log.Comment("Verify Value is set to 0.");
                btnGetValue.Click();
                Verify.AreEqual(0.0, Convert.ToDouble(txtValue.DocumentText));

                Log.Comment("Click on the AnnotatedScrollBar");
                InputHelper.LeftClick(VerticalIncrementRepeatButton, 0, 100);
                Wait.ForIdle();

                Log.Comment("Verify Value is more than 0.");
                btnGetValue.Click();
                var newOffsetValue = Convert.ToDouble(txtValue.DocumentText);
                Verify.IsTrue(newOffsetValue > 0.0);
            }

        }


        [TestMethod]
        public void ScrollUsingTouchAndDrag()
        {
            Log.Comment("AnnotatedScrollBar ScrollUsingTouchAndDrag Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving txtValue");
                TextBlock txtValue = new TextBlock(FindElement.ById("txtValue"));
                Verify.IsNotNull(txtValue, "Verifying that txtValue was found");

                Log.Comment("Retrieving btnGetValue");
                Button btnGetValue = new Button(FindElement.ById("btnGetValue"));
                Verify.IsNotNull(btnGetValue, "Verifying that btnGetValue was found");

                Log.Comment("Retrieving VerticalIncrementRepeatButton");
                Button VerticalIncrementRepeatButton = new Button(FindElement.ById("PART_VerticalIncrementRepeatButton"));
                Verify.IsNotNull(VerticalIncrementRepeatButton, "Verifying that VerticalIncrementRepeatButton was found");

                Log.Comment("Verify Value is set to 0.");
                btnGetValue.Click();
                Verify.AreEqual(0.0, Convert.ToDouble(txtValue.DocumentText));

                Log.Comment("Tap on the AnnotatedScrollBar");
                InputHelper.Tap(VerticalIncrementRepeatButton, 0, 100);
                Wait.ForIdle();

                Log.Comment("Verify Value is more than 0.");
                btnGetValue.Click();
                var firstOffsetValue = Convert.ToDouble(txtValue.DocumentText);
                Verify.IsTrue(firstOffsetValue > 0.0);

                Log.Comment("TapAndDrag on the AnnotatedScrollBar");
                var incrementButtonPoint = VerticalIncrementRepeatButton.GetClickablePoint();
                incrementButtonPoint.Y = incrementButtonPoint.Y + 100;
                InputHelper.Pan(VerticalIncrementRepeatButton, incrementButtonPoint, 200, Direction.South, 2000);
                Wait.ForIdle();

                Log.Comment("Verify Value is more than firstOffsetValue.");
                btnGetValue.Click();
                var secondOffsetValue = Convert.ToDouble(txtValue.DocumentText);
                Verify.IsTrue(secondOffsetValue > firstOffsetValue);
            }

        }

        [TestMethod]
        public void DetailLabelRequestedOffsetDoesNotExceedMaxPossibleValue()
        {
            Log.Comment("AnnotatedScrollBar DetailLabelRequestedOffset Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving isDetailLabelEnabledCheckBox");
                CheckBox isDetailLabelEnabledCheckBox = new CheckBox(FindElement.ById("isDetailLabelEnabledCheckBox"));
                Verify.IsNotNull(isDetailLabelEnabledCheckBox, "Verifying that isDetailLabelEnabledCheckBox was found");

                Log.Comment("Retrieving btnGetMaximum");
                Button btnGetMaximum = new Button(FindElement.ById("btnGetMaximum"));
                Verify.IsNotNull(btnGetMaximum, "Verifying that btnGetMaximum was found");

                Log.Comment("Retrieving txtMaximum");
                TextBlock txtMaximum = new TextBlock(FindElement.ById("txtMaximum"));
                Verify.IsNotNull(txtMaximum, "Verifying that txtMaximum was found");

                Log.Comment("Retrieving btnGetViewportSize");
                Button btnGetViewportSize = new Button(FindElement.ById("btnGetViewportSize"));
                Verify.IsNotNull(btnGetViewportSize, "Verifying that btnGetViewportSize was found");

                Log.Comment("Retrieving txtViewportSize");
                TextBlock txtViewportSize = new TextBlock(FindElement.ById("txtViewportSize"));
                Verify.IsNotNull(txtViewportSize, "Verifying that txtViewportSize was found");

                Log.Comment("Retrieving btnGetLastRequestedDetailLabelOffset");
                Button btnGetLastRequestedDetailLabelOffset = new Button(FindElement.ById("btnGetLastRequestedDetailLabelOffset"));
                Verify.IsNotNull(btnGetLastRequestedDetailLabelOffset, "Verifying that btnGetLastRequestedDetailLabelOffset was found");

                Log.Comment("Retrieving txtLastRequestedDetailLabelOffset");
                TextBlock txtLastRequestedDetailLabelOffset = new TextBlock(FindElement.ById("txtLastRequestedDetailLabelOffset"));
                Verify.IsNotNull(txtLastRequestedDetailLabelOffset, "Verifying that txtLastRequestedDetailLabelOffset was found");

                Log.Comment("Retrieving VerticalIncrementRepeatButton");
                Button VerticalIncrementRepeatButton = new Button(FindElement.ById("PART_VerticalIncrementRepeatButton"));
                Verify.IsNotNull(VerticalIncrementRepeatButton, "Verifying that VerticalIncrementRepeatButton was found");

                Log.Comment("Retrieving VerticalDecrementRepeatButton");
                Button VerticalDecrementRepeatButton = new Button(FindElement.ById("PART_VerticalDecrementRepeatButton"));
                Verify.IsNotNull(VerticalDecrementRepeatButton, "Verifying that VerticalDecrementRepeatButton was found");

                Log.Comment("Check isDetailLabelEnabledCheckBox");
                isDetailLabelEnabledCheckBox.Toggle();
                Wait.ForIdle();

                Log.Comment("Get ScrollView.Maximum value.");
                btnGetMaximum.Click();
                var maximum = Convert.ToDouble(txtMaximum.GetText());

                Log.Comment("Get ScrollView.ViewportSize value.");
                btnGetViewportSize.Click();
                var viewportSize = Convert.ToDouble(txtViewportSize.GetText());

                var maxAllowableOffset = maximum + viewportSize;

                Log.Comment("Move mouse over AnnotatedScrollBar.");
                InputHelper.MoveMouse(VerticalIncrementRepeatButton, 0, VerticalIncrementRepeatButton.BoundingRectangle.Height / 2);
                Wait.ForIdle();

                Log.Comment("Move mouse to the end of the tooltip rail.");
                InputHelper.MoveMouse(VerticalDecrementRepeatButton, 0, -1*VerticalDecrementRepeatButton.BoundingRectangle.Height / 2);
                Wait.ForIdle();

                Log.Comment("Move mouse just past the end of tooltip rail.");
                InputHelper.MoveMouse(VerticalDecrementRepeatButton, 0, -1*VerticalDecrementRepeatButton.BoundingRectangle.Height / 2);
                Wait.ForIdle();

                Log.Comment("Get lastRequestedDetailLabelOffset.");
                btnGetLastRequestedDetailLabelOffset.Click();
                var lastRequestedDetailLabelOffset = Convert.ToDouble(txtLastRequestedDetailLabelOffset.GetText());

                Log.Comment("Verify lastRequestedDetailLabelOffset is more than 0.");
                Verify.IsTrue(lastRequestedDetailLabelOffset > 0);

                Log.Comment("Verify lastRequestedDetailLabelOffset did not exceed Maximum + ViewportSize.");
                Verify.IsTrue(lastRequestedDetailLabelOffset <= maxAllowableOffset);
            }

        }

        [TestMethod]
        public void DetailLabelDoesNotShowWhenDataFieldIsEmpty()
        {
            Log.Comment("AnnotatedScrollBar DetailLabelDoesNotShowWhenDataFieldIsEmpty Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving isDetailLabelEnabledCheckBox");
                CheckBox isDetailLabelEnabledCheckBox = new CheckBox(FindElement.ById("isDetailLabelEnabledCheckBox"));
                Verify.IsNotNull(isDetailLabelEnabledCheckBox, "Verifying that isDetailLabelEnabledCheckBox was found");

                Log.Comment("Retrieving VerticalIncrementRepeatButton");
                Button VerticalIncrementRepeatButton = new Button(FindElement.ById("PART_VerticalIncrementRepeatButton"));
                Verify.IsNotNull(VerticalIncrementRepeatButton, "Verifying that VerticalIncrementRepeatButton was found");

                Log.Comment("Check isDetailLabelEnabledCheckBox");
                isDetailLabelEnabledCheckBox.Toggle();
                Wait.ForIdle();

                Log.Comment("Move mouse over AnnotatedScrollBar.");
                InputHelper.MoveMouse(VerticalIncrementRepeatButton, 0, 200);
                Wait.ForIdle();

                var toolTip = FindElement.ById("PART_DetailLabelToolTip");
                Verify.IsNull(toolTip);
            }

        }

        [TestMethod]
        public void LabelsGrowToAccomodateLongText()
        {
            Log.Comment("AnnotatedScrollBar ScrollUsingTouchAndDrag Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving PART_ScrollPresenter");
                Button scrollPresenter = new Button(FindElement.ById("PART_ScrollPresenter"));
                Verify.IsNotNull(scrollPresenter, "Verifying that PART_ScrollPresenter was found");

                Log.Comment("Retrieving useLongLabelContentCheckBox");
                CheckBox useLongLabelContentCheckBox = new CheckBox(FindElement.ById("useLongLabelContentCheckBox"));
                Verify.IsNotNull(useLongLabelContentCheckBox, "Verifying that useLongLabelContentCheckBox was found");

                Log.Comment("Verify scrollPresenter starting width.");
                Verify.AreEqual(108, scrollPresenter.BoundingRectangle.Width);

                Log.Comment("Check useLongLabelContentCheckBox");
                useLongLabelContentCheckBox.Toggle();
                Wait.ForMilliseconds(500);

                Log.Comment("Verify scrollPresenter ending width.");
                Verify.AreEqual(0, scrollPresenter.BoundingRectangle.Width);
            }

        }

        [TestMethod]
        public void CollidingLabelsAreRemoved()
        {
            Log.Comment("AnnotatedScrollBar CollidingLabelsAreRemoved Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving btnPrePopulateLabels");
                Button btnPrePopulateLabels = new Button(FindElement.ById("btnPrePopulateLabels"));
                Verify.IsNotNull(btnPrePopulateLabels, "Verifying that btnPrePopulateLabels was found");

                Log.Comment("Clicking btnPrePopulateLabels.");
                btnPrePopulateLabels.Click();

                Log.Comment("First item should be visible.");
                var label0 = ElementCache.TryGetObjectByName("Num 0", false);
                Verify.IsNotNull(label0);

                Log.Comment("Last item should be visible.");
                var label19 = ElementCache.TryGetObjectByName("Num 19", false);
                Verify.IsNotNull(label19);

                Log.Comment("Some intermediate items not removed by collision logic should be visible.");
                var label5 = ElementCache.TryGetObjectByName("Num 9", false);
                Verify.IsNotNull(label5);

                var label10 = ElementCache.TryGetObjectByName("Num 14", false);
                Verify.IsNotNull(label10);

                Log.Comment("Some intermediate items removed by collision logic should not be visible.");
                var label1 = ElementCache.TryGetObjectByName("Num 1", false);
                Verify.IsNull(label1);
            }
        }

        [TestMethod]
        public void VerifyCanCancelScrolling()
        {
            Log.Comment("AnnotatedScrollBar Cancel Scrolling via AnnotatedScrollBarScrollingEventArgs Test");

            using (var setup = new TestSetupHelper(new[] { "AnnotatedScrollBar Tests", "navigateToSummary" }))
            {
                Log.Comment("Retrieving txtValue");
                TextBlock txtValue = new TextBlock(FindElement.ById("txtValue"));
                Verify.IsNotNull(txtValue, "Verifying that txtValue was found");

                Log.Comment("Retrieving btnGetValue");
                Button btnGetValue = new Button(FindElement.ById("btnGetValue"));
                Verify.IsNotNull(btnGetValue, "Verifying that btnGetValue was found");

                Log.Comment("Retrieving VerticalDecrementRepeatButton");
                Button VerticalDecrementRepeatButton = new Button(FindElement.ById("PART_VerticalDecrementRepeatButton"));
                Verify.IsNotNull(VerticalDecrementRepeatButton, "Verifying that VerticalDecrementRepeatButton was found");

                Log.Comment("Retrieving CancelScrollingEventCheckBox");
                CheckBox cancelScrollingEventCheckBox = new CheckBox(FindElement.ById("cancelScrollingEventCheckBox"));
                Verify.IsNotNull(cancelScrollingEventCheckBox, "Verifying that cancelScrollingEventCheckBox was found");

                Log.Comment("Verify Value is set to 0.");
                btnGetValue.Click();
                Verify.AreEqual(0.0, Convert.ToDouble(txtValue.DocumentText));

                Log.Comment("Check cancelScrollingEventCheckBox.");
                cancelScrollingEventCheckBox.Toggle();
                Wait.ForIdle();

                Log.Comment("Click VerticalDecrementRepeatButton.");
                VerticalDecrementRepeatButton.Click();

                Log.Comment("Uncheck cancelScrollingEventCheckBox.");
                cancelScrollingEventCheckBox.Toggle();
                Wait.ForIdle();

                Log.Comment("Click VerticalDecrementRepeatButton.");
                VerticalDecrementRepeatButton.Click();

                Log.Comment("Verify Value is more than 0.");
                btnGetValue.Click();
                var newOffsetValue = Convert.ToDouble(txtValue.DocumentText);
                Verify.AreEqual(83, newOffsetValue);
            }

        }
    }
}
