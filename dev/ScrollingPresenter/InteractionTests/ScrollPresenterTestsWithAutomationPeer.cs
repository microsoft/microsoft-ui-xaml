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
    public class ScrollingPresenterTestsWithAutomationPeer : ScrollingPresenterTestsBase
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
        [TestProperty("Description", "Vertically scrolls a Rectangle in a ScrollingPresenter, using its automation peer.")]
        public void ScrollVertically()
        {
            Log.Comment("Selecting ScrollingPresenter tests");

            using (var setup = new TestSetupHelper("ScrollingPresenter Tests"))
            {
                Log.Comment("Navigating to ScrollingPresentersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ScrollingPresenter");
                ScrollingPresenter scrollingPresenter11UIObject = new ScrollingPresenter(FindElement.ByName("ScrollingPresenter11"));
                Verify.IsNotNull(scrollingPresenter11UIObject, "Verifying that ScrollingPresenter was found");

                Verify.IsTrue(scrollingPresenter11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollingPresenter11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.AreEqual(scrollingPresenter11UIObject.VerticalScrollPercent, 0.0, "Verifying VerticalScrollPercent is zero");

                Log.Comment("Scrolling ScrollingPresenter vertically");
                scrollingPresenter11UIObject.ScrollVertical(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for VerticalScrollPercent={0} to change", scrollingPresenter11UIObject.VerticalScrollPercent);
                Wait.ForScrollChanged(scrollingPresenter11UIObject, ScrollProperty.VerticalScrollPercent);

                Log.Comment("Final VerticalScrollPercent={0}", scrollingPresenter11UIObject.VerticalScrollPercent);

                Verify.IsTrue(scrollingPresenter11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollingPresenter11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsTrue(scrollingPresenter11UIObject.VerticalScrollPercent > 0.0, "Verifying VerticalScrollPercent is positive");

                Log.Comment("Returning to the main ScrollingPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Horizontally scrolls an Image in a ScrollingPresenter, using its automation peer.")]
        public void ScrollHorizontally()
        {
            Log.Comment("Selecting ScrollingPresenter tests");

            using (var setup = new TestSetupHelper("ScrollingPresenter Tests"))
            {
                Log.Comment("Navigating to ScrollingPresentersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ScrollingPresenter");
                ScrollingPresenter scrollingPresenter31UIObject = new ScrollingPresenter(FindElement.ByName("ScrollingPresenter31"));
                Verify.IsNotNull(scrollingPresenter31UIObject, "Verifying that ScrollingPresenter was found");

                Verify.IsTrue(scrollingPresenter31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollingPresenter31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scrollingPresenter31UIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Scrolling ScrollingPresenter horizontally");
                scrollingPresenter31UIObject.ScrollHorizontal(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scrollingPresenter31UIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scrollingPresenter31UIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scrollingPresenter31UIObject.HorizontalScrollPercent);

                Verify.IsTrue(scrollingPresenter31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollingPresenter31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsTrue(scrollingPresenter31UIObject.HorizontalScrollPercent > 0.0, "Verifying HorizontalScrollPercent is positive");

                Log.Comment("Returning to the main ScrollingPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls an Image, using the ScrollingPresenter automation peer, while its ExpressionAnimationSources property is being consumed.")]
        public void ScrollWhileUsingExpressionAnimationSources()
        {
            Log.Comment("Selecting ScrollingPresenter tests");

            using (var setup = new TestSetupHelper("ScrollingPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                Log.Comment("Navigating to ScrollingPresenterExpressionAnimationSourcesPage");
                UIObject navigateToExpressionAnimationSourcesUIObject = FindElement.ByName("navigateToExpressionAnimationSources");
                Verify.IsNotNull(navigateToExpressionAnimationSourcesUIObject, "Verifying that navigateToExpressionAnimationSources Button was found");

                Button navigateToExpressionAnimationSourcesButton = new Button(navigateToExpressionAnimationSourcesUIObject);
                navigateToExpressionAnimationSourcesButton.Invoke();
                Wait.ForIdle();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving ScrollingPresenter");
                ScrollingPresenter scrollingPresenterUIObject = new ScrollingPresenter(FindElement.ByName("scrollingPresenter"));
                Verify.IsNotNull(scrollingPresenterUIObject, "Verifying that ScrollingPresenter was found");

                Verify.IsTrue(scrollingPresenterUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollingPresenterUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scrollingPresenterUIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Waiting for final layout");
                WaitForEditValue(editName: "txtLayoutCompleted", editValue: "Yes");

                Edit textBox = new Edit(FindElement.ById("txtViewport"));
                Log.Comment("Viewport: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtExtent"));
                Log.Comment("Extent: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtBarVisualWidth"));

                Log.Comment("Scrolling ScrollingPresenter horizontally and vertically");
                scrollingPresenterUIObject.SetScrollPercent(10.0, 20.0);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scrollingPresenterUIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scrollingPresenterUIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scrollingPresenterUIObject.HorizontalScrollPercent);
                Log.Comment("Final VerticalScrollPercent={0}", scrollingPresenterUIObject.VerticalScrollPercent);

                if (Math.Abs(scrollingPresenterUIObject.HorizontalScrollPercent - 10.0) >= 0.0001 || 
                    Math.Abs(scrollingPresenterUIObject.VerticalScrollPercent - 20.0) >= 0.0001)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.IsTrue(scrollingPresenterUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollingPresenterUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollingPresenterUIObject.HorizontalScrollPercent - 10.0), 0.0001, "Verifying HorizontalScrollPercent is close to 10.0");

                Verify.IsTrue(scrollingPresenterUIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollingPresenterUIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollingPresenterUIObject.VerticalScrollPercent - 20.0), 0.0001, "Verifying VerticalScrollPercent is close to 20.0");

                Log.Comment("Returning to the main ScrollingPresenter test page");
                TestSetupHelper.GoBack();
                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollingPresenter test page.
            }
        }
    }
}
