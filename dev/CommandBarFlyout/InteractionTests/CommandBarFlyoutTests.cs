// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using System.Collections.Generic;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

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
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class CommandBarFlyoutTests
    {
        // Values taken from https://docs.microsoft.com/en-us/windows/desktop/winauto/uiauto-automation-element-propids
        private const int UIA_FlowsFromPropertyId = 30148;
        private const int UIA_FlowsToPropertyId = 30106;
        private const int UIA_PositionInSetPropertyId = 30152;
        private const int UIA_SizeOfSetPropertyId = 30153;

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

        internal class CommandBarFlyoutTestSetupHelper : TestSetupHelper
        {
            public CommandBarFlyoutTestSetupHelper(string languageOverride = "", bool attemptRestartOnDispose = true)
                : base(new[] { "CommandBarFlyout Tests", "Base CommandBarFlyout Tests" }, languageOverride, attemptRestartOnDispose)
            {
                statusReportingTextBox = FindElement.ById<Edit>("StatusReportingTextBox");
            }

            public void ExecuteAndWaitForEvent(Action action, string eventText)
            {
                ExecuteAndWaitForEvents(action, new List<string>() { eventText });
            }

            public void ExecuteAndWaitForEvents(Action action, IList<string> eventTexts)
            {
                IList<ValueChangedEventWaiter> waiters = new List<ValueChangedEventWaiter>();

                foreach (string eventText in eventTexts)
                {
                    waiters.Add(new ValueChangedEventWaiter(statusReportingTextBox, eventText));
                }

                action.Invoke();

                for (int i = 0; i < waiters.Count; i++)
                {
                    Log.Comment(string.Format("Waiting for the event \"{0}\"...", eventTexts[i]));
                    waiters[i].Wait();
                    Log.Comment("Event occurred.");

                    waiters[i].Dispose();
                }
            }

            private Edit statusReportingTextBox;
        }

        [TestMethod]
        public void CanTapOnPrimaryItems()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                Log.Comment("Tapping on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Log.Comment("Tapping on the buttons in the primary commands list.");
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("CutButton1")), new List<string>() { "CutButton1 clicked" });
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("CopyButton1")), new List<string>() { "CopyButton1 clicked" });
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("PasteButton1")), new List<string>() { "PasteButton1 clicked" });
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("BoldButton1")), new List<string>() { "BoldButton1 clicked" });
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("ItalicButton1")), new List<string>() { "ItalicButton1 clicked" });
                setup.ExecuteAndWaitForEvents(() => InputHelper.Tap(FindElement.ById("UnderlineButton1")), new List<string>() { "UnderlineButton1 clicked" });

                Log.Comment("Tapping on the button to hide the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);
            }
        }

        [TestMethod]
        public void CanTapOnSecondaryItems()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");
                ToggleButton isFlyoutOpenCheckBox = FindElement.ById<ToggleButton>("IsFlyoutOpenCheckBox");

                Action openCommandBarAction = () =>
                {
                    Log.Comment("Tapping on a button to show the CommandBarFlyout.");
                    InputHelper.Tap(showCommandBarFlyoutButton);
                    
                    // Pre-RS5, CommandBarFlyouts always open expanded,
                    // so we don't need to tap on the more button in that case.
                    if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                    {
                        Log.Comment("Expanding the CommandBar by invoking the more button.");
                        FindElement.ById<Button>("MoreButton").InvokeAndWait();
                    }
                };
                
                Log.Comment("Opening the CommandBar and invoking the first button in the secondary commands list.");
                openCommandBarAction();

                using (var waiter = isFlyoutOpenCheckBox.GetToggledWaiter())
                {
                    setup.ExecuteAndWaitForEvents(() => FindElement.ById<Button>("UndoButton1").Invoke(), new List<string>() { "UndoButton1 clicked" });
                    waiter.Wait();
                }

                Verify.IsTrue(isFlyoutOpenCheckBox.ToggleState == ToggleState.Off);

                Log.Comment("Opening the CommandBar and invoking the second button in the secondary commands list.");
                openCommandBarAction();

                using (var waiter = isFlyoutOpenCheckBox.GetToggledWaiter())
                {
                    setup.ExecuteAndWaitForEvents(() => FindElement.ById<Button>("RedoButton1").Invoke(), new List<string>() { "RedoButton1 clicked" });
                    waiter.Wait();
                }

                Verify.IsTrue(isFlyoutOpenCheckBox.ToggleState == ToggleState.Off);

                Log.Comment("Opening the CommandBar and invoking the third button in the secondary commands list.");
                openCommandBarAction();

                using (var waiter = isFlyoutOpenCheckBox.GetToggledWaiter())
                {
                    setup.ExecuteAndWaitForEvents(() => FindElement.ById<Button>("SelectAllButton1").Invoke(), new List<string>() { "SelectAllButton1 clicked" });
                    waiter.Wait();
                }

                Verify.IsTrue(isFlyoutOpenCheckBox.ToggleState == ToggleState.Off);

                Log.Comment("Opening the CommandBar and invoking the toggle button in the secondary commands list.");
                openCommandBarAction();

                using (var waiter = isFlyoutOpenCheckBox.GetToggledWaiter())
                {
                    setup.ExecuteAndWaitForEvents(() => FindElement.ById<ToggleButton>("FavoriteToggleButton1").Toggle(), new List<string>() { "FavoriteToggleButton1 checked" });
                    waiter.Wait();
                }
                
                Verify.IsTrue(isFlyoutOpenCheckBox.ToggleState == ToggleState.Off);
            }
        }

        [TestMethod]
        public void VerifyTabNavigationBetweenPrimaryAndSecondaryCommands()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                Log.Comment("Tap on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Log.Comment("Press Tab key to move focus to first secondary command: Undo.");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                Button undoButton1 = FindElement.ById<Button>("UndoButton1");
                var undoButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(undoButtonElement.Current.AutomationId, undoButton1.AutomationId);

                Log.Comment("Press Tab key to move focus to first primary command: Cut.");
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();

                Button cutButton1 = FindElement.ById<Button>("CutButton1");
                var cutButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);
            }
        }

        [TestMethod]
        public void VerifyLeftAndRightNavigationBetweenPrimaryCommands()
        {
            VerifyLeftAndRightNavigationBetweenPrimaryCommands(inRTL: false, useUpDownKeys: false);
            VerifyLeftAndRightNavigationBetweenPrimaryCommands(inRTL: true,  useUpDownKeys: false);
            VerifyLeftAndRightNavigationBetweenPrimaryCommands(inRTL: false, useUpDownKeys: true);
            VerifyLeftAndRightNavigationBetweenPrimaryCommands(inRTL: true,  useUpDownKeys: true);
        }

        private void VerifyLeftAndRightNavigationBetweenPrimaryCommands(bool inRTL, bool useUpDownKeys)
        { 
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                if (inRTL)
                {
                    ToggleButton isRTLCheckBox = FindElement.ById<ToggleButton>("IsRTLCheckBox");

                    Log.Comment("Switch to RightToLeft FlowDirection.");
                    isRTLCheckBox.Toggle();
                    Wait.ForIdle();
                }

                string rightStr;
                string leftStr;
                Key rightKey;
                Key leftKey;

                if (useUpDownKeys)
                {
                    // Down and Up keys are used to move logically within the primary commands: Up to move to the previous command and Down to move to the next command.
                    rightStr = "Down";
                    leftStr = "Up";
                    rightKey = Key.Down;
                    leftKey = Key.Up;
                }
                else
                {
                    // Left and Right keys are used to move physically within the primary commands: Left to move to the left command and Right to move to the right command.
                    rightStr = inRTL ? "Left" : "Right";
                    leftStr = inRTL ? "Right" : "Left";
                    rightKey = inRTL ? Key.Left : Key.Right;
                    leftKey = inRTL ? Key.Right : Key.Left;
                }

                Log.Comment("Tap on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Log.Comment($"Press {rightStr} key to move focus to second primary command: Copy.");
                KeyboardHelper.PressKey(rightKey);
                Wait.ForIdle();

                Button copyButton1 = FindElement.ById<Button>("CopyButton1");
                var copyButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(copyButtonElement.Current.AutomationId, copyButton1.AutomationId);

                Log.Comment($"Press {leftStr} key to move focus back to first primary command: Cut.");
                KeyboardHelper.PressKey(leftKey);
                Wait.ForIdle();

                Button cutButton1 = FindElement.ById<Button>("CutButton1");
                var cutButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);

                if (!useUpDownKeys)
                {
                    Log.Comment($"Press {leftStr} key and remain on first primary command: Cut.");
                    KeyboardHelper.PressKey(leftKey);
                    Wait.ForIdle();

                    cutButtonElement = AutomationElement.FocusedElement;
                    Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);
                }

                Log.Comment($"Press {rightStr} key to move focus to MoreButton.");
                for (int i = 0; i <= 5; i++)
                {
                    KeyboardHelper.PressKey(rightKey);
                    Wait.ForIdle();
                }

                Button moreButton = FindElement.ById<Button>("MoreButton");
                var moreButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);

                if (!useUpDownKeys)
                {
                    Log.Comment($"Press {rightStr} key and remain on MoreButton.");
                    KeyboardHelper.PressKey(rightKey);
                    Wait.ForIdle();

                    moreButtonElement = AutomationElement.FocusedElement;
                    Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);
                }
            }
        }

        [TestMethod]
        public void VerifyUpAndDownNavigationBetweenPrimaryAndSecondaryCommands()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                Log.Comment("Tap on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Log.Comment("Press Down key to move focus to second primary command: Copy.");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Button copyButton1 = FindElement.ById<Button>("CopyButton1");
                var copyButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(copyButtonElement.Current.AutomationId, copyButton1.AutomationId);

                Log.Comment("Press Up key to move focus back to first primary command: Cut.");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();

                Button cutButton1 = FindElement.ById<Button>("CutButton1");
                var cutButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);

                Log.Comment("Press Down key to move focus to last primary command: Underline.");
                for (int i = 0; i <= 4; i++)
                {
                    KeyboardHelper.PressKey(Key.Down);
                    Wait.ForIdle();
                }

                Button underlineButton1 = FindElement.ById<Button>("UnderlineButton1");
                var underlineButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(underlineButtonElement.Current.AutomationId, underlineButton1.AutomationId);

                Log.Comment("Press Down key to move focus to MoreButton.");
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();

                Button moreButton = FindElement.ById<Button>("MoreButton");
                var moreButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);

                Log.Comment("Press Up key to move focus to first primary command: Cut.");
                for (int i = 0; i <= 5; i++)
                {
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                }

                cutButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);

                Log.Comment("Press Up key to move focus to last secondary command: Favorite.");
                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();

                Button favoriteToggleButton1 = FindElement.ById<Button>("FavoriteToggleButton1");
                var favoriteToggleButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(favoriteToggleButtonElement.Current.AutomationId, favoriteToggleButton1.AutomationId);

                Log.Comment("Press Up key to move focus to first secondary command: Undo.");
                for (int i = 0; i <= 2; i++)
                {
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();
                }

                Button undoButton1 = FindElement.ById<Button>("UndoButton1");
                var undoButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(undoButtonElement.Current.AutomationId, undoButton1.AutomationId);

                if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
                {
                    Log.Comment("Press Up key and remain on first secondary command: Undo.");
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();

                    undoButtonElement = AutomationElement.FocusedElement;
                    Verify.AreEqual(undoButtonElement.Current.AutomationId, undoButton1.AutomationId);
                }
                else
                {
                    Log.Comment("Press Up key to move focus to MoreButton.");
                    KeyboardHelper.PressKey(Key.Up);
                    Wait.ForIdle();

                    moreButtonElement = AutomationElement.FocusedElement;
                    Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);

                    Log.Comment("Press Down key to move focus to first primary command through all secondary commands: Cut.");
                    for (int i = 0; i <= 4; i++)
                    {
                        KeyboardHelper.PressKey(Key.Down);
                        Wait.ForIdle();
                    }

                    cutButtonElement = AutomationElement.FocusedElement;
                    Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);
                }
            }
        }

        [TestMethod]
        public void VerifyPrimaryCommandsAutomationSet()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                Log.Comment("Tap on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Button cutButton1 = FindElement.ById<Button>("CutButton1");
                var cutButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(cutButtonElement.Current.AutomationId, cutButton1.AutomationId);

                int sizeOfSet = (int)cutButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_SizeOfSetPropertyId));
                int positionInSet = (int)cutButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_PositionInSetPropertyId));

                Log.Comment("Verify first primary command's SizeOfSet and PositionInSet automation properties.");
                Verify.AreEqual(sizeOfSet, 7);
                Verify.IsTrue(positionInSet == -1 || positionInSet == 1);

                Log.Comment("Press Right key to move focus to last primary command: Underline.");
                for (int i = 0; i <= 4; i++)
                {
                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                }

                Button underlineButton1 = FindElement.ById<Button>("UnderlineButton1");
                var underlineButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(underlineButtonElement.Current.AutomationId, underlineButton1.AutomationId);

                sizeOfSet = (int)underlineButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_SizeOfSetPropertyId));
                positionInSet = (int)underlineButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_PositionInSetPropertyId));

                Log.Comment("Verify last primary command's SizeOfSet and PositionInSet automation properties.");
                Verify.AreEqual(sizeOfSet, 7);
                Verify.IsTrue(positionInSet == -1 || positionInSet == 6);

                Log.Comment("Press Right key to move focus to MoreButton.");
                KeyboardHelper.PressKey(Key.Right);
                Wait.ForIdle();

                Button moreButton = FindElement.ById<Button>("MoreButton");
                var moreButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);

                sizeOfSet = (int)moreButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_SizeOfSetPropertyId));
                positionInSet = (int)moreButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_PositionInSetPropertyId));

                Log.Comment("Verify MoreButton's SizeOfSet and PositionInSet automation properties.");
                Verify.AreEqual(sizeOfSet, 7);
                Verify.AreEqual(positionInSet, 7);
            }
        }

        [TestMethod]
        public void VerifyFlowsToAndFromConnectsPrimaryAndSecondaryCommands()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout");

                Log.Comment("Tapping on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                // Pre-RS5, CommandBarFlyouts always open expanded,
                // so we don't need to tap on the more button in that case.
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Expanding the CommandBar by invoking the more button.");
                    FindElement.ById<Button>("MoreButton").InvokeAndWait();
                }

                Log.Comment("Retrieving the more button and undo button's automation element objects.");

                // Moving to the MoreButton to retrieve it
                for (int i = 0; i <= 6; i++)
                {
                    KeyboardHelper.PressKey(Key.Right);
                    Wait.ForIdle();
                }
                Button moreButton = FindElement.ById<Button>("MoreButton");
                var moreButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(moreButtonElement.Current.AutomationId, moreButton.AutomationId);

                // Moving to the Undo button to retrieve it
                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                Button undoButton1 = FindElement.ById<Button>("UndoButton1");
                var undoButtonElement = AutomationElement.FocusedElement;
                Verify.AreEqual(undoButtonElement.Current.AutomationId, undoButton1.AutomationId);

                Log.Comment("Verifying that the two elements point at each other using FlowsTo and FlowsFrom.");
                var flowsToCollection = (AutomationElementCollection)moreButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_FlowsToPropertyId));

                Verify.AreEqual(1, flowsToCollection.Count);
                Verify.AreEqual(undoButtonElement, flowsToCollection[0]);

                var flowsFromCollection = (AutomationElementCollection)undoButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_FlowsFromPropertyId));

                Verify.AreEqual(1, flowsFromCollection.Count);
                Verify.AreEqual(moreButtonElement, flowsFromCollection[0]);

                Log.Comment("Tapping on a button to hide the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);
            }
        }

        [TestMethod]
        public void VerifyFlowsToAndFromIsNotSetWithoutPrimaryCommands()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout with no primary commands");

                Log.Comment("Tapping on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                Log.Comment("Retrieving the undo button's automation element object.");
                FindElement.ById("UndoButton6").SetFocus();
                Wait.ForIdle();
                var undoButtonElement = AutomationElement.FocusedElement;

                Log.Comment("Verifying that the undo button does not point at the more button using FlowsFrom.");
                var flowsFromCollection = (AutomationElementCollection)undoButtonElement.GetCurrentPropertyValue(AutomationProperty.LookupById(UIA_FlowsFromPropertyId));
                Verify.AreEqual(0, flowsFromCollection.Count);

                Log.Comment("Tapping on a button to hide the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);
            }
        }

        [TestMethod]
        public void VerifyFlyoutClosingBehavior()
        {
            if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone2))
            {
                Log.Warning("Test is disabled pre-RS2 because CommandBarFlyout is not supported pre-RS2");
                return;
            }

            using (var setup = new CommandBarFlyoutTestSetupHelper())
            {
                Button showCommandBarFlyoutButton = FindElement.ByName<Button>("Show CommandBarFlyout with sub-menu");

                Log.Comment("Tapping on a button to show the CommandBarFlyout.");
                InputHelper.Tap(showCommandBarFlyoutButton);

                // Pre-RS5, CommandBarFlyouts always open expanded,
                // so we don't need to tap on the more button in that case.
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    Log.Comment("Expanding the CommandBar by invoking the more button.");
                    FindElement.ById<Button>("MoreButton").InvokeAndWait();
                }

                // Move around over item to open flyout
                PointerInput.Move(FindElement.ByName("ProofingAppBarButton"), 5, 5);
                PointerInput.Move(FindElement.ByName("ProofingAppBarButton"), 6, 5);
                PointerInput.Move(FindElement.ByName("ProofingAppBarButton"), 5, 6);

                // Move around over the first item to keep flyout open safely
                // This also verifies that the flyout is open as it would crash otherwise
                PointerInput.Move(FindElement.ByName("FirstFlyoutItem"), 5, 5);
                PointerInput.Move(FindElement.ByName("FirstFlyoutItem"), 6, 5);
                PointerInput.Move(FindElement.ByName("FirstFlyoutItem"), 5, 6);

                // Click outside of the flyout to close it
                InputHelper.Tap(FindElement.ByName<Button>("Show CommandBarFlyout"));

                // Check that the flyout item is not present anymore
                VerifyElement.NotFound("FirstFlyoutItem",FindBy.Name);
            }
        }
    }
}
