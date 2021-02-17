// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class PipsPagerTests : PipsPagerTestBase
    {
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
        [TestProperty("TestSuite", "A")]
        public void PipsPagerChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);

                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(0);

                ChangeNumberOfPages(NumberOfPagesOptions.Five);
                VerifyNumberOfPages("5");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(3);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(2);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void PreviousPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);

                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetNextPageButton());

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetNextPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(1);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NextPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);

                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(3);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(4);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void PipsPagerInfinitePagesTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);

                VerifyPageChanged(0);

                ChangeNumberOfPages(NumberOfPagesOptions.Infinite);
                VerifyNumberOfPages("Infinite");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(3);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(4);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(5);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(6);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(7);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void PreviousPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);

                /* Test for Collapsed */
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Collapsed);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.Collapsed);
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.Collapsed);

                /* Test for Visible */
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.Visible);
                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.Collapsed);

                /* Test for VisibleOnPointerOver */
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.VisibleOnPointerOver);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.Collapsed);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.VisibleOnPointerOver, true);

                InputHelper.LeftClick(elements.GetCurrentNumberOfPagesTextBlock());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Previous, ButtonVisibilityMode.VisibleOnPointerOver);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void NextPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.VisibleOnPointerOver);

                ChangeNumberOfPages(NumberOfPagesOptions.Five);
                VerifyNumberOfPages("5");

                /* Test for Collapsed */
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Collapsed);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.Collapsed);

                /* Test for Visible */
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Visible);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.Visible);
                /* We step until the end of the list (4 times, since we have 5 pages) */
                InputHelper.LeftClick(elements.GetNextPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.Collapsed);

                /* Test for VisibleOnPointerOver */
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.VisibleOnPointerOver);
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.Collapsed);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.VisibleOnPointerOver, true);

                InputHelper.LeftClick(elements.GetCurrentNumberOfPagesTextBlock());
                VerifyPageButtonWithVisibilityModeSet(ButtonType.Next, ButtonVisibilityMode.VisibleOnPointerOver);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "E")]
        public void OrientationChangeTest()
        {
            using (var setup = new TestSetupHelper("PipsPager Tests"))
            {
                elements = new PipsPagerElements();
                SetOrientation(Microsoft.Windows.Apps.Test.Automation.OrientationType.Horizontal);
                VerifyOrientationChanged(Microsoft.Windows.Apps.Test.Automation.OrientationType.Horizontal);

                SetOrientation(Microsoft.Windows.Apps.Test.Automation.OrientationType.Vertical);
                VerifyOrientationChanged(Microsoft.Windows.Apps.Test.Automation.OrientationType.Vertical);

            }
        }
    }
}
