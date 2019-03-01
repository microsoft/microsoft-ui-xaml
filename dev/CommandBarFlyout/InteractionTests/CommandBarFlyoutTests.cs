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

#if BUILD_WINDOWS
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
    public class CommandBarFlyoutTests
    {
        // Values taken from https://docs.microsoft.com/en-us/windows/desktop/winauto/uiauto-automation-element-propids
        private const int UIA_FlowsFromPropertyId = 30148;
        private const int UIA_FlowsToPropertyId = 30106;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
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
                ToggleButton isFlyoutOpenCheckBox = FindElement.ById<ToggleButton>("IsFlyoutOpenCheckBox");
                
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
                FindElement.ById("MoreButton").SetFocus();
                Wait.ForIdle();
                var moreButtonElement = AutomationElement.FocusedElement;

                FindElement.ById("UndoButton1").SetFocus();
                Wait.ForIdle();
                var undoButtonElement = AutomationElement.FocusedElement;

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
                ToggleButton isFlyoutOpenCheckBox = FindElement.ById<ToggleButton>("IsFlyoutOpenCheckBox");

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
    }
}
