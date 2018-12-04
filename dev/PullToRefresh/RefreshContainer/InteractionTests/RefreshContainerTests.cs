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

#if BUILD_WINDOWS || USING_TESTNET
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RefreshContainerTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
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
        public void BasicInteractionTest()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

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
                Verify.IsNotNull(listViewItem1, "Verifying that we found a UIElement called listViewItem2");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item ten as generic UIElement");
                UIObject listViewItem10 = FindElement.ByName("listViewItem10");
                Verify.IsNotNull(listViewItem1, "Verifying that we found a UIElement called listViewItem10");

                Wait.ForIdle();

                Log.Comment("Retrieve list view item nine as generic UIElement");
                UIObject listViewItem9 = FindElement.ByName("listViewItem9");
                Verify.IsNotNull(listViewItem1, "Verifying that we found a UIElement called listViewItem9");

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

        [TestMethod]
        public void RefreshRequestedHandlersWorkOnEitherContainerOrVisualizerOrBoth()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled on pre-RS2 due to chaining being disabled before this point");
                return;
            }

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

        [TestMethod]
        public void InteractionOnImageTest()
        {
            InteractionOnImageTestPrivate(withInfoProvider: false);
        }

        [TestMethod]
        public void InteractionOnImageWithInfoProviderTest()
        {
            InteractionOnImageTestPrivate(withInfoProvider: true);
        }

        public void InteractionOnImageTestPrivate(bool withInfoProvider)
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone5))
            {
                Log.Warning("This test relies on touch input, the injection of which is only supported in RS5 and up. Test is disabled.");
                return;
            }

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