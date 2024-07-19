﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;
using Common;

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
    //[TestClass] This feature requires the Xaml2018 feature, which is currently disabled by Velocity
    public class ButtonInteractionTests
    {
        public const string TargetElementAutomationId = "TargetElement";
        public const string DummyControlAutomationId = "DummyControl";
        public const string InvokeCountTextBlockAutomationId = "InvokeCountTextBlock";
        public const string InvokeModeComboBoxAutomationId = "InvokeModeComboBox";
        public const string IsHoveringTextBlockAutomationId = "IsHoveringTextBlock";
        public const string IsPressingTextBlockAutomationId = "IsPressingTextBlock";
        public const string ReleaseModeAutomationId = "ReleaseMode";
        public const string PressModeAutomationId = "PressMode";
        public const string HoverModeAutomationId = "HoverMode";

        [Flags]
        public enum TestOptions
        {
            None = 0,
            ReleaseInvokeMode = 1,
            PressInvokeMode = 2,
            HoverInvokeMode = 3,
        }

        private TestSetupHelper ButtonInteractionTestSetup(TestOptions options = TestOptions.None)
        {
            var setupHelper = new TestSetupHelper("ButtonInteraction Tests");

            switch (options)
            {
                case TestOptions.ReleaseInvokeMode:
                    {
                        ComboBox invokeModeComboBox = new ComboBox(FindElement.ById(InvokeModeComboBoxAutomationId));
                        invokeModeComboBox.SelectItemById(ReleaseModeAutomationId);
                    }
                    break;

                case TestOptions.PressInvokeMode:
                    {
                        ComboBox invokeModeComboBox = new ComboBox(FindElement.ById(InvokeModeComboBoxAutomationId));
                        invokeModeComboBox.SelectItemById(PressModeAutomationId);
                    }
                    break;

                case TestOptions.HoverInvokeMode:
                    {
                        ComboBox invokeModeComboBox = new ComboBox(FindElement.ById(InvokeModeComboBoxAutomationId));
                        invokeModeComboBox.SelectItemById(HoverModeAutomationId);
                    }
                    break;
            }

            return setupHelper;
        }

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
        public void ValidateInvokeWithReleaseMode()
        {
            using (var setup = ButtonInteractionTestSetup(TestOptions.ReleaseInvokeMode))
            {
                var targetElement = FindElement.ById(TargetElementAutomationId);
                var invokeCountTextBlock = new TextBlock(FindElement.ById(InvokeCountTextBlockAutomationId));
                var isPressingTextBlock = new TextBlock(FindElement.ById(IsPressingTextBlockAutomationId));
                var isHoveringTextBlock = new TextBlock(FindElement.ById(IsHoveringTextBlockAutomationId));

                Log.Comment("Start with mouse not over the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width - 10, 0);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);

                Log.Comment("Press the left mouse button on the element.");
                InputHelper.LeftMouseButtonDown(targetElement);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("True", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("True", isPressingTextBlock.DocumentText);

                Log.Comment("Release the left mouse button on the element.");
                InputHelper.LeftMouseButtonUp(targetElement);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("True", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);

                Log.Comment("Move the mouse off the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width + 10, 0);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void ValidateInvokeWithPressMode()
        {
            using (var setup = ButtonInteractionTestSetup(TestOptions.PressInvokeMode))
            {
                var targetElement = FindElement.ById(TargetElementAutomationId);
                var invokeCountTextBlock = new TextBlock(FindElement.ById(InvokeCountTextBlockAutomationId));
                var isPressingTextBlock = new TextBlock(FindElement.ById(IsPressingTextBlockAutomationId));
                var isHoveringTextBlock = new TextBlock(FindElement.ById(IsHoveringTextBlockAutomationId));

                Log.Comment("Start with mouse not over the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width - 10, 0);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);

                Log.Comment("Press the left mouse button on the element.");
                InputHelper.LeftMouseButtonDown(targetElement);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("True", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("True", isPressingTextBlock.DocumentText);

                Log.Comment("Release the left mouse button on the element.");
                InputHelper.LeftMouseButtonUp(targetElement);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("True", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);

                Log.Comment("Move the mouse off the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width + 10, 0);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void ValidateInvokeWithHoverMode()
        {
            using (var setup = ButtonInteractionTestSetup(TestOptions.HoverInvokeMode))
            {
                var targetElement = FindElement.ById(TargetElementAutomationId);
                var invokeCountTextBlock = new TextBlock(FindElement.ById(InvokeCountTextBlockAutomationId));
                var isPressingTextBlock = new TextBlock(FindElement.ById(IsPressingTextBlockAutomationId));
                var isHoveringTextBlock = new TextBlock(FindElement.ById(IsHoveringTextBlockAutomationId));

                Log.Comment("Start with mouse not over the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width - 10, 0);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);

                Log.Comment("Move the mouse onto the element.");
                InputHelper.MoveMouse(targetElement, 0, 0);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("True", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("True", isPressingTextBlock.DocumentText);

                Log.Comment("Move the mouse off the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width + 10, 0);
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);
                Verify.AreEqual("False", isHoveringTextBlock.DocumentText);
                Verify.AreEqual("False", isPressingTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void DoesNotInvokeWhenClickIsDraggedOffBeforeRelease()
        {
            using (var setup = ButtonInteractionTestSetup(TestOptions.ReleaseInvokeMode))
            {
                var targetElement = FindElement.ById(TargetElementAutomationId);
                var invokeCountTextBlock = new TextBlock(FindElement.ById(InvokeCountTextBlockAutomationId));

                Log.Comment("Press the left mouse button on the element.");
                InputHelper.LeftMouseButtonDown(targetElement);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);

                Log.Comment("Move the mouse off the element.");
                InputHelper.MoveMouse(targetElement, targetElement.BoundingRectangle.Width + 10, 0);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);

                Log.Comment("Release the left mouse button off the element.");
                InputHelper.LeftMouseButtonUp(targetElement);
                Verify.AreEqual("0", invokeCountTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void DoesInvokeWithKeyboard()
        {
            using (var setup = ButtonInteractionTestSetup())
            {
                var targetElement = FindElement.ById(TargetElementAutomationId);
                var invokeCountTextBlock = new TextBlock(FindElement.ById(InvokeCountTextBlockAutomationId));

                Log.Comment("Focus the element.");
                targetElement.SetFocus();
                Wait.ForIdle();

                Log.Comment("Press the SPACE key to invoke the element.");
                targetElement.SendKeys("{SPACE}");
                Verify.AreEqual("1", invokeCountTextBlock.DocumentText);

                Log.Comment("Press the ENTER key to invoke the element.");
                targetElement.SendKeys("{ENTER}");
                Verify.AreEqual("2", invokeCountTextBlock.DocumentText);
            }
        }
    }
}