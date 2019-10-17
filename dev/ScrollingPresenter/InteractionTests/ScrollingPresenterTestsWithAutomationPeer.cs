// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using Common;

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
    public class ScrollerTestsWithAutomationPeer : ScrollerTestsBase
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

        [TestMethod]
        [TestProperty("Description", "Vertically scrolls a Rectangle in a Scroller, using its automation peer.")]
        public void ScrollVertically()
        {
            Log.Comment("Selecting Scroller tests");

            using (var setup = new TestSetupHelper("Scroller Tests"))
            {
                Log.Comment("Navigating to ScrollersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving Scroller");
                Scroller scroller11UIObject = new Scroller(FindElement.ByName("Scroller11"));
                Verify.IsNotNull(scroller11UIObject, "Verifying that Scroller was found");

                Verify.IsTrue(scroller11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scroller11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.AreEqual(scroller11UIObject.VerticalScrollPercent, 0.0, "Verifying VerticalScrollPercent is zero");

                Log.Comment("Scrolling Scroller vertically");
                scroller11UIObject.ScrollVertical(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for VerticalScrollPercent={0} to change", scroller11UIObject.VerticalScrollPercent);
                Wait.ForScrollChanged(scroller11UIObject, ScrollProperty.VerticalScrollPercent);

                Log.Comment("Final VerticalScrollPercent={0}", scroller11UIObject.VerticalScrollPercent);

                Verify.IsTrue(scroller11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scroller11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsTrue(scroller11UIObject.VerticalScrollPercent > 0.0, "Verifying VerticalScrollPercent is positive");

                Log.Comment("Returning to the main Scroller test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Horizontally scrolls an Image in a Scroller, using its automation peer.")]
        public void ScrollHorizontally()
        {
            Log.Comment("Selecting Scroller tests");

            using (var setup = new TestSetupHelper("Scroller Tests"))
            {
                Log.Comment("Navigating to ScrollersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving Scroller");
                Scroller scroller31UIObject = new Scroller(FindElement.ByName("Scroller31"));
                Verify.IsNotNull(scroller31UIObject, "Verifying that Scroller was found");

                Verify.IsTrue(scroller31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scroller31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scroller31UIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Scrolling Scroller horizontally");
                scroller31UIObject.ScrollHorizontal(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scroller31UIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scroller31UIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scroller31UIObject.HorizontalScrollPercent);

                Verify.IsTrue(scroller31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scroller31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsTrue(scroller31UIObject.HorizontalScrollPercent > 0.0, "Verifying HorizontalScrollPercent is positive");

                Log.Comment("Returning to the main Scroller test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls an Image, using the Scroller automation peer, while its ExpressionAnimationSources property is being consumed.")]
        public void ScrollWhileUsingExpressionAnimationSources()
        {
            Log.Comment("Selecting Scroller tests");

            using (var setup = new TestSetupHelper("Scroller Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                Log.Comment("Navigating to ScrollerExpressionAnimationSourcesPage");
                UIObject navigateToExpressionAnimationSourcesUIObject = FindElement.ByName("navigateToExpressionAnimationSources");
                Verify.IsNotNull(navigateToExpressionAnimationSourcesUIObject, "Verifying that navigateToExpressionAnimationSources Button was found");

                Button navigateToExpressionAnimationSourcesButton = new Button(navigateToExpressionAnimationSourcesUIObject);
                navigateToExpressionAnimationSourcesButton.Invoke();
                Wait.ForIdle();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving Scroller");
                Scroller scrollerUIObject = new Scroller(FindElement.ByName("scroller"));
                Verify.IsNotNull(scrollerUIObject, "Verifying that Scroller was found");

                Verify.IsTrue(scrollerUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollerUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scrollerUIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Waiting for final layout");
                WaitForEditValue(editName: "txtLayoutCompleted", editValue: "Yes");

                Edit textBox = new Edit(FindElement.ById("txtViewport"));
                Log.Comment("Viewport: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtExtent"));
                Log.Comment("Extent: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtBarVisualWidth"));

                Log.Comment("Scrolling Scroller horizontally and vertically");
                scrollerUIObject.SetScrollPercent(10.0, 20.0);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scrollerUIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scrollerUIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scrollerUIObject.HorizontalScrollPercent);
                Log.Comment("Final VerticalScrollPercent={0}", scrollerUIObject.VerticalScrollPercent);

                if (Math.Abs(scrollerUIObject.HorizontalScrollPercent - 10.0) >= 0.0001 || 
                    Math.Abs(scrollerUIObject.VerticalScrollPercent - 20.0) >= 0.0001)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.IsTrue(scrollerUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollerUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollerUIObject.HorizontalScrollPercent - 10.0), 0.0001, "Verifying HorizontalScrollPercent is close to 10.0");

                Verify.IsTrue(scrollerUIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollerUIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollerUIObject.VerticalScrollPercent - 20.0), 0.0001, "Verifying VerticalScrollPercent is close to 20.0");

                Log.Comment("Returning to the main Scroller test page");
                TestSetupHelper.GoBack();
                // Output-debug-string-level "None" is automatically restored when landing back on the Scroller test page.
            }
        }
    }
}
