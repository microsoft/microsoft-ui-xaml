// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
    public class RefreshContainerTests
    {
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
        
        // Scenario: on the RefreshContainer test page (with the page's refresh delay toggled), refresh with the
        //           Refresh-on-container and Refresh-on-visualizer buttons, then touch-drag the list 450 px in each
        //           pull direction (rotating the pull direction and alignment in between), including one drag from a
        //           list that is not scrolled to its edge.
        // Expected: RefreshCount goes 1, 2 (buttons), 3 (pull down), 4 (pull right); the drag that starts away from the
        //           edge only peeks and leaves it at 4; then 5 (pull up) and 6 (pull left).
        // A failure means: touch pull-to-refresh or programmatic refresh does not work for some pull direction, or a
        //                  pull that does not start at the edge refreshes.
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void BasicInteractionTest()
        {
            using (var setup = new TestSetupHelper("PTR Tests")) // This literally clicks the button corresponding to the test page.
            {
                Wait.ForIdle();
                Log.Comment("Retrieve the refresh container page button as generic UIElement");
                UIObject refreshContainerButtonUIObject = FindElement.ByName("RefreshContainerPageButton");
                Verify.IsNotNull(refreshContainerButtonUIObject, "Verifying that we found a UIElement called RefreshContainerPageButton");
                Wait.ForIdle();

                //Navigate to the right page
                InputHelper.Tap(refreshContainerButtonUIObject);
                Wait.ForIdle();

                Log.Comment("Retrieve adapted list view as generic UIElement");
                UIObject lvUIObject = FindElement.ByName("listView");
                Verify.IsNotNull(lvUIObject, "Verifying that we found a UIElement called listView");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item one as generic UIElement");
                UIObject listViewItem1 = FindElement.ByName("listViewItem1");
                Verify.IsNotNull(listViewItem1, "Verifying that we found a UIElement called listViewItem1");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item two as generic UIElement");
                UIObject listViewItem2 = FindElement.ByName("listViewItem2");
                Verify.IsNotNull(listViewItem2, "Verifying that we found a UIElement called listViewItem2");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item ten as generic UIElement");
                UIObject listViewItem10 = FindElement.ByName("listViewItem10");
                Verify.IsNotNull(listViewItem10, "Verifying that we found a UIElement called listViewItem10");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item nine as generic UIElement");
                UIObject listViewItem9 = FindElement.ByName("listViewItem9");
                Verify.IsNotNull(listViewItem9, "Verifying that we found a UIElement called listViewItem9");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh on container button as generic UIElement");
                UIObject refreshOnContainerButtonUIObject = FindElement.ByName("RefreshOnContainerButton");
                Verify.IsNotNull(refreshOnContainerButtonUIObject, "Verifying that we found a UIElement called RefreshOnContainerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh on visualizer button as generic UIElement");
                UIObject refreshOnVisualizerButtonUIObject = FindElement.ByName("RefreshOnVisualizerButton");
                Verify.IsNotNull(refreshOnVisualizerButtonUIObject, "Verifying that we found a UIElement called RefreshOnVisualizerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve rotate refresh visualizer button as generic UIElement");
                UIObject rotateButtonUIObject = FindElement.ByName("RotateButton");
                Verify.IsNotNull(rotateButtonUIObject, "Verifying that we found a UIElement called RotateButton");

                Wait.ForIdle();

                Log.Comment("Retrieve the change alignment button as generic UIElement");
                UIObject changeAlignmentUIObject = FindElement.ByName("ChangeAlignment");
                Verify.IsNotNull(changeAlignmentUIObject, "Verifying that we found a UIElement called ChangeAlignment");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh count text block as generic UIElement");
                UIObject textboxUIObject = FindElement.ByName("RefreshCount");
                Verify.IsNotNull(textboxUIObject, "Verifying that we found a UIElement called RefreshCount");
                Edit textBox = new Edit(textboxUIObject);

                Wait.ForIdle();

                Log.Comment("Retrieve AddOrRemoveRefreshDelay button as generic UIElement");
                UIObject AddOrRemoveRefreshDelayButtonUIObject = FindElement.ByName("AddOrRemoveRefreshDelay");
                Verify.IsNotNull(AddOrRemoveRefreshDelayButtonUIObject, "Verifying that we found a UIElement called AddOrRemoveRefreshDelay");

                Wait.ForIdle();

                InputHelper.Tap(AddOrRemoveRefreshDelayButtonUIObject);
                InputHelper.Tap(refreshOnContainerButtonUIObject);
                WaitForEditValue("RefreshCount", "1");

                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                WaitForEditValue("RefreshCount", "2");

                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                InputHelper.DragDistance(listViewItem1, 450, Direction.South, 800);
                WaitForEditValue("RefreshCount", "3");

                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                InputHelper.DragDistance(listViewItem2, 450, Direction.East, 800);
                WaitForEditValue("RefreshCount", "4");

                //The SV won't be at the bottom of its content to start so we will enter the peeking
                //state and not perform a refresh
                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                InputHelper.DragDistance(lvUIObject, 750, Direction.North);
                Wait.ForIdle();
                LogRefreshVisualizerStates();
                Verify.AreEqual("4", textBox.Value);

                InputHelper.DragDistance(listViewItem10, 450, Direction.North, 800);
                WaitForEditValue("RefreshCount", "5");

                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                InputHelper.DragDistance(listViewItem9, 450, Direction.West, 800);
                WaitForEditValue("RefreshCount", "6");

                Log.Comment("Returning to the main PTR test page");
                TestSetupHelper.GoBack();
            }
        }

        // Scenario: on the RefreshContainer test page, refresh with both buttons while toggling the page's
        //           RefreshRequested handlers on the container and on the visualizer (container only, both, visualizer
        //           only, neither).
        // Expected: RefreshCount goes 1, 2 with the container handler; 4, 6 with both (each refresh counted twice); 7,
        //           8 with the visualizer handler only; and stays 8 with neither.
        // A failure means: RefreshRequested is not raised on the container or the visualizer (or is raised the wrong
        //                  number of times) when apps subscribe on either one or both.
        [TestMethod]
        public void RefreshRequestedHandlersWorkOnEitherContainerOrVisualizerOrBoth()
        {
            using (var setup = new TestSetupHelper("PTR Tests")) // This literally clicks the button corresponding to the test page.
            {
                Log.Comment("Retrieve the refresh container page button as generic UIElement");
                UIObject refreshContainerButtonUIObject = FindElement.ByName("RefreshContainerPageButton");
                Verify.IsNotNull(refreshContainerButtonUIObject, "Verifying that we found a UIElement called RefreshContainerPageButton");
                Wait.ForIdle();

                //Navigate to the right page
                InputHelper.Tap(refreshContainerButtonUIObject);
                Wait.ForIdle();

                Log.Comment("Retrieve refresh on container button as generic UIElement");
                UIObject refreshOnContainerButtonUIObject = FindElement.ByName("RefreshOnContainerButton");
                Verify.IsNotNull(refreshOnContainerButtonUIObject, "Verifying that we found a UIElement called RefreshOnContainerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh on visualizer button as generic UIElement");
                UIObject refreshOnVisualizerButtonUIObject = FindElement.ByName("RefreshOnVisualizerButton");
                Verify.IsNotNull(refreshOnVisualizerButtonUIObject, "Verifying that we found a UIElement called RefreshOnVisualizerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh count text block as generic UIElement");
                UIObject textboxUIObject = FindElement.ByName("RefreshCount");
                Verify.IsNotNull(textboxUIObject, "Verifying that we found a UIElement called RefreshCount");
                Edit textBox = new Edit(textboxUIObject);

                Wait.ForIdle();

                Log.Comment("Retrieve AddOrRemoveRefreshDelay button as generic UIElement");
                UIObject AddOrRemoveRefreshDelayButtonUIObject = FindElement.ByName("AddOrRemoveRefreshDelay");
                Verify.IsNotNull(AddOrRemoveRefreshDelayButtonUIObject, "Verifying that we found a UIElement called AddOrRemoveRefreshDelay");

                Wait.ForIdle();

                Log.Comment("Retrieve RCRefreshRequestedComboBoxSwitcher button as generic UIElement");
                UIObject RCRefreshRequestedComboBoxSwitcherUIObject = FindElement.ByName("RCRefreshRequestedComboBoxSwitcher");
                Verify.IsNotNull(RCRefreshRequestedComboBoxSwitcherUIObject, "Verifying that we found a UIElement called RCRefreshRequestedComboBoxSwitcher");

                Wait.ForIdle();

                Log.Comment("Retrieve RVRefreshRequestedComboBoxSwitcher button as generic UIElement");
                UIObject RVRefreshRequestedComboBoxSwitcherUIObject = FindElement.ByName("RVRefreshRequestedComboBoxSwitcher");
                Verify.IsNotNull(RVRefreshRequestedComboBoxSwitcherUIObject, "Verifying that we found a UIElement called RVRefreshRequestedComboBoxSwitcher");

                Wait.ForIdle();

                InputHelper.Tap(AddOrRemoveRefreshDelayButtonUIObject);

                InputHelper.Tap(refreshOnContainerButtonUIObject);
                WaitForEditValue("RefreshCount", "1");
                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                WaitForEditValue("RefreshCount", "2");

                InputHelper.Tap(RVRefreshRequestedComboBoxSwitcherUIObject);

                //Now that both are on, the refresh count will go up twice as fast
                InputHelper.Tap(refreshOnContainerButtonUIObject);
                WaitForEditValue("RefreshCount", "4");
                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                WaitForEditValue("RefreshCount", "6");

                InputHelper.Tap(RCRefreshRequestedComboBoxSwitcherUIObject);
                
                InputHelper.Tap(refreshOnContainerButtonUIObject);
                WaitForEditValue("RefreshCount", "7");
                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                WaitForEditValue("RefreshCount", "8");

                InputHelper.Tap(RVRefreshRequestedComboBoxSwitcherUIObject);

                //Now that neither are on the refresh count won't increase
                InputHelper.Tap(refreshOnContainerButtonUIObject);
                Wait.ForIdle();
                LogRefreshVisualizerStates();
                Verify.AreEqual("8", textBox.Value);
                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                Wait.ForIdle();
                LogRefreshVisualizerStates();
                Verify.AreEqual("8", textBox.Value);

                Log.Comment("Returning to the main PTR test page");
                TestSetupHelper.GoBack();
            }
        }

        // Scenario: arm the page's scroll-away hook and touch-drag the list down 450 px; when the visualizer enters
        //           Interacting the hook scrolls the list to 200 and, once that scroll lands, to 250 (DIPs).
        // Expected: transitions start with Idle->Interacting;Interacting->Idle, never reach Pending or Refreshing,
        //           RefreshCount is unchanged and the final offset is at least 200; the pull ratio at the cancel is at
        //           most 0.8 and keeps growing by at least 0.1 afterwards (the finger was still dragging); the pull is
        //           still active after the first scroll (which started inside the 1 DIP threshold) and is cancelled at
        //           an offset of at least 249.
        // A failure means: scrolling the content during a pull no longer cancels the refresh interaction, or cancels it
        //                  while the view is still within the threshold.
        [TestMethod]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        [TestProperty("Description", "If the adapted ScrollViewer scrolls beyond the pull threshold while a touch pull is Interacting, the pull stops being a refresh interaction: the visualizer returns from Interacting to Idle, never reaches Pending/Refreshing, and no refresh is requested on release.")]
        public void ScrollingAwayDuringPullCancelsRefreshInteraction()
        {
            using (var setup = new TestSetupHelper("PTR Tests")) // This literally clicks the button corresponding to the test page.
            {
                Wait.ForIdle();
                UIObject refreshContainerButtonUIObject = FindElement.ByName("RefreshContainerPageButton");
                Verify.IsNotNull(refreshContainerButtonUIObject, "Verifying that we found a UIElement called RefreshContainerPageButton");
                InputHelper.Tap(refreshContainerButtonUIObject);
                Wait.ForIdle();

                UIObject listViewItem1 = FindElement.ByName("listViewItem1");
                Verify.IsNotNull(listViewItem1, "Verifying that we found a UIElement called listViewItem1");
                Edit refreshCount = new Edit(FindElement.ByName("RefreshCount"));
                Edit transitions = new Edit(FindElement.ByName("StateTransitions"));
                Edit scrollerOffset = new Edit(FindElement.ById("ScrollerOffset"));
                CheckBox scrollAway = new CheckBox(FindElement.ByName("ScrollAwayDuringPullCheckBox"));
                string initialRefreshCount = refreshCount.Value;

                scrollAway.Check();
                Wait.ForIdle();
                Verify.AreEqual(ToggleState.On, scrollAway.ToggleState, "Precondition: the scroll-away hook is armed");
                Verify.AreEqual("", transitions.Value, "Precondition: no transitions recorded yet");

                InputHelper.DragDistance(listViewItem1, 450, Direction.South, 800);
                Wait.ForIdle();
                Wait.RetryUntilEvalFuncSuccessOrTimeout(
                    () => { return transitions.Value.EndsWith("->Idle;"); },
                    retryTimoutByMilliseconds: 5000);
                Log.Comment("State transitions: " + transitions.Value);
                Log.Comment("Scroller offset: " + scrollerOffset.Value);

                double offset = double.Parse(scrollerOffset.Value, System.Globalization.CultureInfo.InvariantCulture);
                Verify.IsTrue(offset >= 200.0, "Precondition: the hook scrolled the list (far beyond the 1px pull threshold) during the pull; offset " + offset);
                Verify.IsTrue(transitions.Value.StartsWith("Idle->Interacting;Interacting->Idle;"), "The pull leaves Interacting for Idle once the list scrolls away");
                Verify.IsFalse(transitions.Value.Contains("Pending"), "The pull never reaches Pending");
                Verify.IsFalse(transitions.Value.Contains("Refreshing"), "No refresh starts");
                Verify.AreEqual(initialRefreshCount, refreshCount.Value, "No refresh was requested");

                // The pull ratio kept growing after the cancel: the finger was still dragging, so the interaction was ended
                // by the view change and not by the end of the manipulation. The page samples the ratio from the provider's
                // composition property (written on every tracker change, before InteractionRatioChanged is throttled):
                // once at the cancel and then every frame.
                string trace = new Edit(FindElement.ByName("ScrollAwayTrace")).Value;
                Log.Comment("Scroll-away trace: " + trace);
                System.Text.RegularExpressions.Match ratios = System.Text.RegularExpressions.Regex.Match(trace, @"ratioAtCancel=([0-9.]+);maxRatioAfterCancel=([0-9.]+);offsetAtCancel=([0-9.]+);interactingAfterFirstScroll=(\w+)");
                Verify.IsTrue(ratios.Success, "Ratios, the offset and the state after the first scroll were recorded around the cancel");
                double ratioAtCancel = double.Parse(ratios.Groups[1].Value, System.Globalization.CultureInfo.InvariantCulture);
                double maxRatioAfterCancel = double.Parse(ratios.Groups[2].Value, System.Globalization.CultureInfo.InvariantCulture);
                double offsetAtCancel = double.Parse(ratios.Groups[3].Value, System.Globalization.CultureInfo.InvariantCulture);
                Verify.IsTrue(ratioAtCancel <= 0.8, "Cancelled before reaching the execution ratio: " + ratioAtCancel);
                Verify.IsTrue(maxRatioAfterCancel >= ratioAtCancel + 0.1, "The pull continued after the cancel: " + maxRatioAfterCancel + " >= " + ratioAtCancel + " + 0.1");
                // The hook scrolls twice. The first ChangeView(200) starts while the offset is still 0, inside the adapter's
                // 1 DIP threshold, so it must not cancel the pull: the provider is still interacting once that scroll has
                // landed. Only the second ChangeView(250), which starts beyond the threshold, cancels it. Offsets are in DIPs.
                Verify.AreEqual("True", ratios.Groups[4].Value, "Still interacting for refresh after a view change that started inside the threshold");
                Verify.IsTrue(offsetAtCancel >= 249.0, "The pull was cancelled only by the scroll that started beyond the threshold: offset at cancel " + offsetAtCancel);

                scrollAway.Uncheck();
                Wait.ForIdle();
                Log.Comment("Returning to the main PTR test page");
                TestSetupHelper.GoBack();
            }
        }

        //Issue #6649 Some tests fail when run on OS 21H1 
        // Scenario: on the RefreshContainer-on-image page (a TestUI adapter adapts an Image), refresh with the
        //           container button, then with the visualizer button (after moving the page's handler), then
        //           touch-drag the image 450 px in all four directions, rotating the pull direction and alignment in
        //           between.
        // Expected: RefreshCount goes 1, 2, then 3, 4, 5 and 6 after the four drags.
        // A failure means: the issue is still present (the app crashes on the horizontal drag), or pull-to-refresh with
        //                  a custom IRefreshInfoProviderAdapter does not refresh.
        // Ignored: tracked by 24178380 (after RotateButton installs a new TestUI adapter, the horizontal drag crashes
        //          the test app with E_INVALIDARG); remove the Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] // 24178380. Still failing on a desktop test VM (2026-10): after RotateButton installs a new TestUI ImageIRefreshInfoProviderAdapter, the horizontal drag on the image crashes the test app (E_INVALIDARG 0x80070057 raised in Microsoft.UI.Xaml.dll, 0xC000027B).
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void InteractionOnImageTest()
        {
            InteractionOnImageTestPrivate(withInfoProvider: false);
        }

        //Issue #6649 Some tests fail when run on OS 21H1 
        // Scenario: same as InteractionOnImageTest, but first tap AdaptButton so the page re-adapts the image and
        //           assigns the visualizer's InfoProvider itself, and re-adapt after each rotation.
        // Expected: RefreshCount goes 1, 2, then 3, 4, 5 and 6 after the four drags (interaction ratios are logged).
        // A failure means: the issue is still present (tapping AdaptButton crashes the app), or a custom InfoProvider
        //                  set by the app does not drive the refresh.
        // Ignored: tracked by task 24429465 (tapping AdaptButton crashes the test app with E_INVALIDARG); remove the
        //          Ignore once fixed.
        [TestMethod]
        [TestProperty("Ignore", "True")] //Task 24429465: DCPP: Multiple MUXControls interaction tests failing after IXP integration. Still failing on a desktop test VM (2026-10), also when run alone: tapping AdaptButton (TestUI ImageIRefreshInfoProviderAdapter re-adapting the image) crashes the test app (E_INVALIDARG 0x80070057 raised in Microsoft.UI.Xaml.dll, 0xC000027B).
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public void InteractionOnImageWithInfoProviderTest()
        {
            InteractionOnImageTestPrivate(withInfoProvider: true);
        }

        public void InteractionOnImageTestPrivate(bool withInfoProvider)
        {
            using (var setup = new TestSetupHelper("PTR Tests")) // This literally clicks the button corresponding to the test page.
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("MSFT: 12949886 - Test is disabled on phone due to reliability issues");
                    return;
                }

                Wait.ForIdle();

                Log.Comment("Retrieve the refresh container on image page button as generic UIElement");
                UIObject refreshContainerOnImageButtonUIObject = FindElement.ByName("RefreshContainerOnImagePageButton");
                Verify.IsNotNull(refreshContainerOnImageButtonUIObject, "Verifying that we found a UIElement called RefreshContainerOnImagePageButton");
                Wait.ForIdle();

                //Navigate to the right page
                InputHelper.Tap(refreshContainerOnImageButtonUIObject);
                Wait.ForIdle();

                UIObject adaptButtonUIObject = null;

                if (withInfoProvider)
                {
                    Log.Comment("Retrieve the adapt button as generic UIElement");
                    adaptButtonUIObject = FindElement.ByName("AdaptButton");
                    Verify.IsNotNull(adaptButtonUIObject, "Verifying that we found a UIElement called AdaptButton");
                    Wait.ForIdle();
                }

                Log.Comment("Retrieve adapted image as generic UIElement");
                UIObject imageUIObject = FindElement.ByName("Image");
                Verify.IsNotNull(imageUIObject, "Verifying that we found a UIElement called Image");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh on container button as generic UIElement");
                UIObject refreshOnContainerButtonUIObject = FindElement.ByName("RefreshOnContainerButton");
                Verify.IsNotNull(refreshOnContainerButtonUIObject, "Verifying that we found a UIElement called RefreshOnContainerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve refresh on visualizer button as generic UIElement");
                UIObject refreshOnVisualizerButtonUIObject = FindElement.ByName("RefreshOnVisualizerButton");
                Verify.IsNotNull(refreshOnVisualizerButtonUIObject, "Verifying that we found a UIElement called RefreshOnVisualizerButton");

                Wait.ForIdle();

                Log.Comment("Retrieve rotate refresh visualizer button as generic UIElement");
                UIObject rotateButtonUIObject = FindElement.ByName("RotateButton");
                Verify.IsNotNull(rotateButtonUIObject, "Verifying that we found a UIElement called RotateButton");

                Wait.ForIdle();

                Log.Comment("Retrieve the change alignment button as generic UIElement");
                UIObject changeAlignmentUIObject = FindElement.ByName("ChangeAlignment");
                Verify.IsNotNull(changeAlignmentUIObject, "Verifying that we found a UIElement called ChangeAlignment");

                Wait.ForIdle();

                Log.Comment("Retrieve AddOrRemoveRefreshDelay button as generic UIElement");
                UIObject AddOrRemoveRefreshDelayButtonUIObject = FindElement.ByName("AddOrRemoveRefreshDelay");
                Verify.IsNotNull(AddOrRemoveRefreshDelayButtonUIObject, "Verifying that we found a UIElement called AddOrRemoveRefreshDelay");

                Wait.ForIdle();

                Log.Comment("Retrieve ChangeRefreshRequested button as generic UIElement");
                UIObject ChangeRefreshRequestedUIObject = FindElement.ByName("ChangeRefreshRequested");
                Verify.IsNotNull(ChangeRefreshRequestedUIObject, "Verifying that we found a UIElement called ChangeRefreshRequested");

                Wait.ForIdle();

                if (withInfoProvider)
                {
                    InputHelper.Tap(adaptButtonUIObject);
                    Wait.ForIdle();
                }

                InputHelper.Tap(AddOrRemoveRefreshDelayButtonUIObject);
                InputHelper.Tap(refreshOnContainerButtonUIObject);
                WaitForEditValue("RefreshCount", "1", logInteractionRatios: withInfoProvider);

                InputHelper.Tap(ChangeRefreshRequestedUIObject);
                InputHelper.Tap(refreshOnVisualizerButtonUIObject);
                WaitForEditValue("RefreshCount", "2", logInteractionRatios: withInfoProvider);
                InputHelper.Tap(ChangeRefreshRequestedUIObject);

                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                InputHelper.DragDistance(imageUIObject, 450, Direction.South, 600);
                WaitForEditValue("RefreshCount", "3", logInteractionRatios: withInfoProvider);

                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                if (withInfoProvider)
                {
                    InputHelper.Tap(adaptButtonUIObject);
                    Wait.ForIdle();
                }
                InputHelper.DragDistance(imageUIObject, 450, Direction.East, 600);
                WaitForEditValue("RefreshCount", "4", logInteractionRatios: withInfoProvider);

                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                if (withInfoProvider)
                {
                    InputHelper.Tap(adaptButtonUIObject);
                    Wait.ForIdle();
                }
                InputHelper.DragDistance(imageUIObject, 450, Direction.North, 600);
                WaitForEditValue("RefreshCount", "5", logInteractionRatios: withInfoProvider);

                InputHelper.Tap(rotateButtonUIObject);
                InputHelper.Tap(changeAlignmentUIObject);
                Wait.ForIdle();
                if (withInfoProvider)
                {
                    InputHelper.Tap(adaptButtonUIObject);
                    Wait.ForIdle();
                }
                InputHelper.DragDistance(imageUIObject, 450, Direction.West, 600);
                WaitForEditValue("RefreshCount", "6", logInteractionRatios: withInfoProvider);

                Log.Comment("Returning to the main PTR test page");
                TestSetupHelper.GoBack();
            }
        }

        private void LogRefreshVisualizerStates()
        {
            Log.Comment("Logging successive refresh visualizer states");
            UIObject stateUIObject = FindElement.ById("States");
            Verify.IsNotNull(stateUIObject);
            ComboBox cmbStateUIObject = new ComboBox(stateUIObject);
            foreach (ComboBoxItem item in cmbStateUIObject.AllItems)
            {
                Log.Comment(item.Name);
            }

            Log.Comment("Resetting refresh visualizer states log");
            UIObject resetStatesUIObject = FindElement.ById("ResetStates");
            Verify.IsNotNull(resetStatesUIObject);
            Button resetStatesButton = new Button(resetStatesUIObject);
            resetStatesButton.Invoke();
            Wait.ForIdle();
        }

        // Logs the InteractionRatio history stored in the ComboBox.Items collection of
        // the ComboBox named "InteractionRatios" then clears those items by invoking the
        // Button named "ResetInteractionRatios" to start a new history from scratch.
        private void LogInteractionRatios()
        {
            Log.Comment("Logging successive interaction ratios");
            UIObject interactionRatioUIObject = FindElement.ById("InteractionRatios");
            Verify.IsNotNull(interactionRatioUIObject);
            ComboBox cmbInteractionRatioUIObject = new ComboBox(interactionRatioUIObject);
            foreach (ComboBoxItem item in cmbInteractionRatioUIObject.AllItems)
            {
                Log.Comment(item.Name);
            }

            Log.Comment("Resetting interaction ratios log");
            UIObject resetInteractionRatiosUIObject = FindElement.ById("ResetInteractionRatios");
            Verify.IsNotNull(resetInteractionRatiosUIObject);
            Button resetInteractionRatiosButton = new Button(resetInteractionRatiosUIObject);
            resetInteractionRatiosButton.Invoke();
            Wait.ForIdle();
        }

        private void WaitForEditValue(string editName, string editValue, bool logInteractionRatios = false)
        {
            Wait.ForIdle();
            Edit edit = new Edit(FindElement.ById(editName));
            Verify.IsNotNull(edit);
            Log.Comment("Current value for " + editName + ": " + edit.Value);
            if (edit.Value != editValue)
            {
                using (var waiter = new ValueChangedEventWaiter(edit, editValue))
                {
                    Log.Comment("Waiting for " + editName + " to be set to " + editValue);

                    bool success = waiter.TryWait(TimeSpan.FromSeconds(3));
                    Log.Comment("Current value for " + editName + ": " + edit.Value);

                    if (success)
                    {
                        Log.Comment("Wait succeeded");
                    }
                    else
                    {
                        if (edit.Value == editValue)
                        {
                            Log.Warning("Wait failed but TextBox contains expected Text");
                        }
                        else
                        {
                            Log.Error("Wait for edit value failed");
                            LogRefreshVisualizerStates();
                            if (logInteractionRatios)
                            {
                                LogInteractionRatios();
                            }
                            throw new WaiterException();
                        }
                    }
                }
            }

            LogRefreshVisualizerStates();
            if (logInteractionRatios)
            {
                LogInteractionRatios();
            }
        }
    }
}
