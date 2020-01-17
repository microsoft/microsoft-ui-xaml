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
    public class ScrollPresenterTestsWithAutomationPeer : ScrollPresenterTestsBase
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
        [TestProperty("Description", "Vertically scrolls a Rectangle in a ScrollPresenter, using its automation peer.")]
        public void ScrollVertically()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                Log.Comment("Navigating to ScrollPresentersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ScrollPresenter");
                ScrollPresenter scrollPresenter11UIObject = new ScrollPresenter(FindElement.ByName("ScrollPresenter11"));
                Verify.IsNotNull(scrollPresenter11UIObject, "Verifying that ScrollPresenter was found");

                Verify.IsTrue(scrollPresenter11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollPresenter11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.AreEqual(scrollPresenter11UIObject.VerticalScrollPercent, 0.0, "Verifying VerticalScrollPercent is zero");

                Log.Comment("Scrolling ScrollPresenter vertically");
                scrollPresenter11UIObject.ScrollVertical(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for VerticalScrollPercent={0} to change", scrollPresenter11UIObject.VerticalScrollPercent);
                Wait.ForScrollChanged(scrollPresenter11UIObject, ScrollProperty.VerticalScrollPercent);

                Log.Comment("Final VerticalScrollPercent={0}", scrollPresenter11UIObject.VerticalScrollPercent);

                Verify.IsTrue(scrollPresenter11UIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollPresenter11UIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsTrue(scrollPresenter11UIObject.VerticalScrollPercent > 0.0, "Verifying VerticalScrollPercent is positive");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Horizontally scrolls an Image in a ScrollPresenter, using its automation peer.")]
        public void ScrollHorizontally()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                Log.Comment("Navigating to ScrollPresentersWithSimpleContentsPage");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ScrollPresenter");
                ScrollPresenter scrollPresenter31UIObject = new ScrollPresenter(FindElement.ByName("ScrollPresenter31"));
                Verify.IsNotNull(scrollPresenter31UIObject, "Verifying that ScrollPresenter was found");

                Verify.IsTrue(scrollPresenter31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollPresenter31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scrollPresenter31UIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Scrolling ScrollPresenter horizontally");
                scrollPresenter31UIObject.ScrollHorizontal(ScrollAmount.LargeIncrement);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scrollPresenter31UIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scrollPresenter31UIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scrollPresenter31UIObject.HorizontalScrollPercent);

                Verify.IsTrue(scrollPresenter31UIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollPresenter31UIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsTrue(scrollPresenter31UIObject.HorizontalScrollPercent > 0.0, "Verifying HorizontalScrollPercent is positive");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "Scrolls an Image, using the ScrollPresenter automation peer, while its ExpressionAnimationSources property is being consumed.")]
        public void ScrollWhileUsingExpressionAnimationSources()
        {
            Log.Comment("Selecting ScrollPresenter tests");

            using (var setup = new TestSetupHelper("ScrollPresenter Tests"))
            {
                SetOutputDebugStringLevel("Verbose");

                Log.Comment("Navigating to ScrollPresenterExpressionAnimationSourcesPage");
                UIObject navigateToExpressionAnimationSourcesUIObject = FindElement.ByName("navigateToExpressionAnimationSources");
                Verify.IsNotNull(navigateToExpressionAnimationSourcesUIObject, "Verifying that navigateToExpressionAnimationSources Button was found");

                Button navigateToExpressionAnimationSourcesButton = new Button(navigateToExpressionAnimationSourcesUIObject);
                navigateToExpressionAnimationSourcesButton.Invoke();
                Wait.ForIdle();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Retrieving ScrollPresenter");
                ScrollPresenter scrollPresenterUIObject = new ScrollPresenter(FindElement.ByName("scrollPresenter"));
                Verify.IsNotNull(scrollPresenterUIObject, "Verifying that ScrollPresenter was found");

                Verify.IsTrue(scrollPresenterUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollPresenterUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.AreEqual(scrollPresenterUIObject.HorizontalScrollPercent, 0.0, "Verifying HorizontalScrollPercent is zero");

                Log.Comment("Waiting for final layout");
                WaitForEditValue(editName: "txtLayoutCompleted", editValue: "Yes");

                Edit textBox = new Edit(FindElement.ById("txtViewport"));
                Log.Comment("Viewport: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtExtent"));
                Log.Comment("Extent: " + textBox.Value);
                textBox = new Edit(FindElement.ById("txtBarVisualWidth"));

                Log.Comment("Scrolling ScrollPresenter horizontally and vertically");
                scrollPresenterUIObject.SetScrollPercent(10.0, 20.0);

                Log.Comment("Waiting for HorizontalScrollPercent={0} to change", scrollPresenterUIObject.HorizontalScrollPercent);
                Wait.ForScrollChanged(scrollPresenterUIObject, ScrollProperty.HorizontalScrollPercent);

                Log.Comment("Final HorizontalScrollPercent={0}", scrollPresenterUIObject.HorizontalScrollPercent);
                Log.Comment("Final VerticalScrollPercent={0}", scrollPresenterUIObject.VerticalScrollPercent);

                if (Math.Abs(scrollPresenterUIObject.HorizontalScrollPercent - 10.0) >= 0.0001 || 
                    Math.Abs(scrollPresenterUIObject.VerticalScrollPercent - 20.0) >= 0.0001)
                {
                    LogAndClearTraces();
                }

                SetLoggingLevel(isPrivateLoggingEnabled: false);

                Verify.IsTrue(scrollPresenterUIObject.HorizontallyScrollable, "Verifying HorizontallyScrollable is true");
                Verify.IsTrue(scrollPresenterUIObject.HorizontalViewSize > 0.0, "Verifying HorizontalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollPresenterUIObject.HorizontalScrollPercent - 10.0), 0.0001, "Verifying HorizontalScrollPercent is close to 10.0");

                Verify.IsTrue(scrollPresenterUIObject.VerticallyScrollable, "Verifying VerticallyScrollable is true");
                Verify.IsTrue(scrollPresenterUIObject.VerticalViewSize > 0.0, "Verifying VerticalViewSize is positive");
                Verify.IsLessThan(Math.Abs(scrollPresenterUIObject.VerticalScrollPercent - 20.0), 0.0001, "Verifying VerticalScrollPercent is close to 20.0");

                Log.Comment("Returning to the main ScrollPresenter test page");
                TestSetupHelper.GoBack();
                // Output-debug-string-level "None" is automatically restored when landing back on the ScrollPresenter test page.
            }
        }
    }
}
