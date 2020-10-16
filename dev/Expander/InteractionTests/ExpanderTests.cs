// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;
using Windows.Foundation.Metadata;

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
    public class ExpanderTests
    {
        public const string ExpandedExpanderAutomationId = "ExpandedExpander";
        public const string CollapsedExpanderAutomationId = "CollapsedExpander";
        public const string ExpandedExpanderContentAutomationId = "ExpandedExpanderContent";
        public const string CollapsedExpanderContentAutomationId = "CollapsedExpanderContent";

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
        public void BasicTest()
        {
            Log.Comment("Expander Basic Test");
        }

        [TestMethod]
        public void DoesExpandWithKeyboard()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            Log.Comment("The expander's content, when collapsed should not load.");
            var collapsedExpanderContent = FindElement.ById(CollapsedExpanderContentAutomationId);
            Verify.IsNull(collapsedExpanderContent, "Verifying that the collapsed content is not loaded.");

            Log.Comment("Focus the expander.");
            var collapsedExpander = FindElement.ById(CollapsedExpanderAutomationId);
            collapsedExpander.SetFocus();
            Wait.ForIdle();

            Log.Comment("Press the SPACE key to expand the expander.");
            collapsedExpander.SendKeys("{SPACE}");

            Log.Comment("Find the previously collapsed content, it should now be available.");
            collapsedExpanderContent = FindElement.ById(CollapsedExpanderContentAutomationId);
            Verify.IsNotNull(collapsedExpanderContent, "Verifying that the collapsed content is now loaded.");
        }

        [TestMethod]
        public void DoesCollapseWithKeyboard()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            Log.Comment("The expander's content, when expanded should be loaded.");
            var expandedExpanderContent = FindElement.ById(ExpandedExpanderContentAutomationId);
            Verify.IsNotNull(expandedExpanderContent, "Verifying that the expanded content is loaded.");

            Log.Comment("Focus the expander.");
            var expandedExpander = FindElement.ById(ExpandedExpanderAutomationId);
            expandedExpander.SetFocus();
            Wait.ForIdle();

            Log.Comment("Press the SPACE key to collapse the expander.");
            expandedExpander.SendKeys("{SPACE}");

            Log.Comment("The previously expanded content should now be unloaded.");
            expandedExpanderContent = FindElement.ById(ExpandedExpanderContentAutomationId);
            Verify.IsNull(expandedExpanderContent, "Verifying that the expanded content is now unloaded.");
        }
    }
}
