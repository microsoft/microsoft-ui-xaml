// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;
using System.Threading;

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;
using Common;

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
    public class SliderInteractionTests
    {
        public const string HorizontalElementAutomationId = "HorizontalElement";
        public const string VerticalElementAutomationId = "VerticalElement";
        public const string HorizontalPositionTextBlockAutomationId = "HorizontalElementPositionTextBlock";
        public const string HorizontalMinimumTextBlockAutomationId = "HorizontalElementMinimumTextBlock";
        public const string HorizontalMaximumTextBlockAutomationId = "HorizontalElementMaximumTextBlock";
        public const string HorizontalSmallChangeTextBlockAutomationId = "HorizontalElementSmallChangeTextBlock";
        public const string HorizontalLargeChangeTextBlockAutomationId = "HorizontalElementLargeChangeTextBlock";
        public const string VerticalPositionTextBlockAutomationId = "VerticalElementPositionTextBlock";
        public const string VerticalMinimumTextBlockAutomationId = "VerticalElementMinimumTextBlock";
        public const string VerticalMaximumTextBlockAutomationId = "VerticalElementMaximumTextBlock";
        public const string VerticalSmallChangeTextBlockAutomationId = "VerticalElementSmallChangeTextBlock";
        public const string VerticalLargeChangeTextBlockAutomationId = "VerticalElementLargeChangeTextBlock";

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
        public void CanSlideHorizontally()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(HorizontalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(HorizontalPositionTextBlockAutomationId));

                const int dragDistance = 200;
                var startPosition = targetElement.BoundingRectangle;

                Log.Comment("Drag the target element diagonally.");
                InputHelper.Pan(targetElement, dragDistance, Direction.SouthEast);

                var endPosition = targetElement.BoundingRectangle;

                Log.Comment("Validate that the target element only moved horizontally.");
                Verify.IsGreaterThan(endPosition.X, startPosition.X);
                Verify.AreEqual((int)(double.Parse(positionTextBlock.DocumentText)), endPosition.X - startPosition.X);

                Log.Comment("Validate that the target element didn't move vertically.");
                Verify.AreEqual(startPosition.Y, endPosition.Y);
            }
        }

        [TestMethod]
        public void CanSlideVertically()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(VerticalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(VerticalPositionTextBlockAutomationId));

                const int dragDistance = 200;
                var startPosition = targetElement.BoundingRectangle;

                Log.Comment("Drag the target element diagonally.");
                InputHelper.Pan(targetElement, dragDistance, Direction.SouthEast);

                var endPosition = targetElement.BoundingRectangle;

                Log.Comment("Validate that the target element only moved vertically.");
                Verify.IsGreaterThan(endPosition.Y, startPosition.Y);
                Verify.AreEqual((int)(double.Parse(positionTextBlock.DocumentText)), endPosition.Y - startPosition.Y);

                Log.Comment("Validate that the target element didn't move horizontally.");
                Verify.AreEqual(startPosition.X, endPosition.X);
            }
        }

        [TestMethod]
        public void DoesNotSlidePastMinimum()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(HorizontalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(HorizontalPositionTextBlockAutomationId));
                var minimumTextBlock = new TextBlock(FindElement.ById(HorizontalMinimumTextBlockAutomationId));

                const int dragDistance = 300;

                Verify.AreEqual(minimumTextBlock.DocumentText, positionTextBlock.DocumentText);

                Log.Comment("Drag the target element far to the left.");
                InputHelper.Pan(targetElement, dragDistance, Direction.West);

                var endPosition = targetElement.BoundingRectangle;

                Log.Comment("Validate that the target element didn't move after the second drag.");
                Verify.AreEqual(minimumTextBlock.DocumentText, positionTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void DoesNotSlidePastMaximum()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(HorizontalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(HorizontalPositionTextBlockAutomationId));
                var maximumTextBlock = new TextBlock(FindElement.ById(HorizontalMaximumTextBlockAutomationId));

                const int dragDistance = 300;

                Log.Comment("Drag the target element far to the right.");
                InputHelper.Pan(targetElement, dragDistance, Direction.East);

                Verify.AreEqual(maximumTextBlock.DocumentText, positionTextBlock.DocumentText);

                Log.Comment("Drag the target element to the right again.");
                InputHelper.Pan(targetElement, dragDistance, Direction.East);

                Log.Comment("Validate that the target element didn't move after the second drag.");
                Verify.AreEqual(maximumTextBlock.DocumentText, positionTextBlock.DocumentText);
            }
        }

        [TestMethod]
        public void CanSlideHorizontallyWithKeyboard()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(HorizontalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(HorizontalPositionTextBlockAutomationId));
                var smallIncrTextBlock = new TextBlock(FindElement.ById(HorizontalSmallChangeTextBlockAutomationId));
                var largeChangeTextBlock = new TextBlock(FindElement.ById(HorizontalLargeChangeTextBlockAutomationId));
                var minimumTextBlock = new TextBlock(FindElement.ById(HorizontalMinimumTextBlockAutomationId));
                var maximumTextBlock = new TextBlock(FindElement.ById(HorizontalMaximumTextBlockAutomationId));

                var minimum = (int)(double.Parse(minimumTextBlock.DocumentText));
                var maximum = (int)(double.Parse(maximumTextBlock.DocumentText));
                var smallIncr = (int)(double.Parse(smallIncrTextBlock.DocumentText));
                var largeIncr = (int)(double.Parse(largeChangeTextBlock.DocumentText));

                Log.Comment("Focus the target element.");
                targetElement.SetFocus();
                Wait.ForIdle();

                var startPosition = targetElement.BoundingRectangle;

                Log.Comment("Inject RIGHT to move the element right.");
                targetElement.SendKeys("{RIGHT}");
                Wait.ForIdle();

                var endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(smallIncr, endPosition.X - startPosition.X);
                Verify.AreEqual(smallIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject PAGE DOWN to move the element right.");
                targetElement.SendKeys("{PGDN}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(largeIncr, endPosition.X - startPosition.X);
                Verify.AreEqual(smallIncr + largeIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject LEFT to move the element left.");
                targetElement.SendKeys("{LEFT}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(-smallIncr, endPosition.X - startPosition.X);
                Verify.AreEqual(largeIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject PAGE UP to move the element left.");
                targetElement.SendKeys("{PGUP}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(-largeIncr, endPosition.X - startPosition.X);
                Verify.AreEqual(0, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject END to move the element to the maximum position.");
                targetElement.SendKeys("{END}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(maximum - minimum, endPosition.X - startPosition.X);
                Verify.AreEqual(maximum, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject HOME to move the element to the minimum position.");
                targetElement.SendKeys("{HOME}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(minimum - maximum, endPosition.X - startPosition.X);
                Verify.AreEqual(minimum, (int)(double.Parse(positionTextBlock.DocumentText)));
            }
        }

        [TestMethod]
        public void CanSlideVerticallyWithKeyboard()
        {
            if (!ApiInformation.IsPropertyPresent("Windows.UI.Xaml.UIElement", "Interactions"))
            {
                Log.Warning("UIElement.Interactions not supported on this build.");
                return;
            }

            using (var setup = new TestSetupHelper("SliderInteraction Tests"))
            {
                var targetElement = FindElement.ById(VerticalElementAutomationId);
                var positionTextBlock = new TextBlock(FindElement.ById(VerticalPositionTextBlockAutomationId));
                var smallIncrTextBlock = new TextBlock(FindElement.ById(VerticalSmallChangeTextBlockAutomationId));
                var largeChangeTextBlock = new TextBlock(FindElement.ById(VerticalLargeChangeTextBlockAutomationId));
                var minimumTextBlock = new TextBlock(FindElement.ById(VerticalMinimumTextBlockAutomationId));
                var maximumTextBlock = new TextBlock(FindElement.ById(VerticalMaximumTextBlockAutomationId));

                var minimum = (int)(double.Parse(minimumTextBlock.DocumentText));
                var maximum = (int)(double.Parse(maximumTextBlock.DocumentText));
                var smallIncr = (int)(double.Parse(smallIncrTextBlock.DocumentText));
                var largeIncr = (int)(double.Parse(largeChangeTextBlock.DocumentText));

                Log.Comment("Focus the target element.");
                targetElement.SetFocus();
                Wait.ForIdle();

                var startPosition = targetElement.BoundingRectangle;

                Log.Comment("Inject DOWN to move the element right.");
                targetElement.SendKeys("{DOWN}");
                Wait.ForIdle();

                var endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(smallIncr, endPosition.Y - startPosition.Y);
                Verify.AreEqual(smallIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject PAGE DOWN to move the element right.");
                targetElement.SendKeys("{PGDN}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(largeIncr, endPosition.Y - startPosition.Y);
                Verify.AreEqual(smallIncr + largeIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject UP to move the element left.");
                targetElement.SendKeys("{UP}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(-smallIncr, endPosition.Y - startPosition.Y);
                Verify.AreEqual(largeIncr, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject PAGE UP to move the element left.");
                targetElement.SendKeys("{PGUP}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(-largeIncr, endPosition.Y - startPosition.Y);
                Verify.AreEqual(0, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject END to move the element to the maximum position.");
                targetElement.SendKeys("{END}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(maximum - minimum, endPosition.Y - startPosition.Y);
                Verify.AreEqual(maximum, (int)(double.Parse(positionTextBlock.DocumentText)));

                startPosition = endPosition;

                Log.Comment("Inject HOME to move the element to the minimum position.");
                targetElement.SendKeys("{HOME}");
                Wait.ForIdle();

                endPosition = targetElement.BoundingRectangle;

                Verify.AreEqual(minimum - maximum, endPosition.Y - startPosition.Y);
                Verify.AreEqual(minimum, (int)(double.Parse(positionTextBlock.DocumentText)));
            }
        }
    }
}