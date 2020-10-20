// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class PagerControlTests : PagerControlTestBase
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
        public void NumberBoxDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();
                VerifyPageChanged(0);

                SendValueToNumberBox("3"); // Note: PagerControl displays numbers starting at 1 but the page changed event sends 0-based numbers
                VerifyPageChanged(2);

                SendValueToNumberBox("1");
                VerifyPageChanged(0);

                SendValueToNumberBox("5");
                VerifyPageChanged(4);

                SendValueToNumberBox("2");
                VerifyPageChanged(1);

                SendValueToNumberBox("100");
                Verify.AreEqual("5", FindTextBox(elements.GetPagerNumberBox()).GetText()); // If over max, value should be clamped down to the max.
                VerifyPageChanged(4);

                SendValueToNumberBox("-100");
                Verify.AreEqual("1", FindTextBox(elements.GetPagerNumberBox()).GetText()); // If under min, value should be clamped up to the min.
                VerifyPageChanged(0);

            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void NumberBoxInfinityModeTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();
                VerifyPageChanged(0);
                elements.GetEnterInfinityModeButton().Click();

                SendValueToNumberBox("3"); // Note: PagerControl displays numbers starting at 1 but the page changed event sends 0-based numbers
                VerifyPageChanged(2);

                SendValueToNumberBox("1");
                VerifyPageChanged(0);

                SendValueToNumberBox("5");
                VerifyPageChanged(4);

                SendValueToNumberBox("2");
                VerifyPageChanged(1);

                SendValueToNumberBox("100");
                Verify.AreEqual("100", FindTextBox(elements.GetPagerNumberBox()).GetText());
                VerifyPageChanged(99);

                SendValueToNumberBox("10000");
                Verify.AreEqual("10000", FindTextBox(elements.GetPagerNumberBox()).GetText());
                VerifyPageChanged(9999);

                SendValueToNumberBox("-100");
                Verify.AreEqual("1", FindTextBox(elements.GetPagerNumberBox()).GetText());
                VerifyPageChanged(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ComboBoxDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                VerifyPageChanged(0);

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SelectValueInPagerComboBox(2);
                VerifyPageChanged(2);

                SelectValueInPagerComboBox(4);
                VerifyPageChanged(4);

                SelectValueInPagerComboBox(0);
                VerifyPageChanged(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void AutoDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                VerifyPageChanged(0);

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SelectValueInPagerComboBox(2);
                VerifyPageChanged(2);

                SelectValueInPagerComboBox(4);
                VerifyPageChanged(4);

                SelectValueInPagerComboBox(0);
                VerifyPageChanged(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void NumberPanelChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();

                VerifyPageChanged(0);

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                SelectPageInNumberPanel(2);
                VerifyPageChanged(1);
                VerifyNumberPanelContent("12345");

                SelectPageInNumberPanel(5);
                VerifyPageChanged(4);
                VerifyNumberPanelContent("12345");

                SelectPageInNumberPanel(4);
                VerifyPageChanged(3);
                VerifyNumberPanelContent("12345");

                SelectPageInNumberPanel(3);
                VerifyPageChanged(2);
                VerifyNumberPanelContent("12345");

                ChangeNumberOfPages();
                VerifyNumberOfPages("100");


                SelectPageInNumberPanel(1);
                VerifyPageChanged(0);
                VerifyNumberPanelContent("12345More100");

                SelectPageInNumberPanel(2);
                VerifyPageChanged(1);
                VerifyNumberPanelContent("12345More100");

                SelectPageInNumberPanel(3);
                VerifyPageChanged(2);
                VerifyNumberPanelContent("12345More100");

                SelectPageInNumberPanel(4);
                VerifyPageChanged(3);
                VerifyNumberPanelContent("12345More100");

                SelectPageInNumberPanel(5);
                VerifyPageChanged(4);
                VerifyNumberPanelContent("1More456More100");

                SelectPageInNumberPanel(6);
                VerifyPageChanged(5);
                VerifyNumberPanelContent("1More567More100");

                SelectPageInNumberPanel(100);
                VerifyPageChanged(99);

                VerifyNumberPanelContent("1More96979899100");

                SelectPageInNumberPanel(99);
                VerifyPageChanged(98);
                VerifyNumberPanelContent("1More96979899100");

                SelectPageInNumberPanel(98);
                VerifyPageChanged(97);
                VerifyNumberPanelContent("1More96979899100");

                SelectPageInNumberPanel(97);
                VerifyPageChanged(96);
                VerifyNumberPanelContent("1More96979899100");

                SelectPageInNumberPanel(96);
                VerifyPageChanged(95);
                VerifyNumberPanelContent("1More959697More100");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void NumberPanelChangingPageTest2()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();

                VerifyPageChanged(0);

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);
                VerifyNumberPanelContent("12345");

                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(4);
                VerifyNumberPanelContent("12345");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(3);
                VerifyNumberPanelContent("12345");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(2);
                VerifyNumberPanelContent("12345");

                ChangeNumberOfPages();
                VerifyNumberOfPages("100");

                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChanged(0);
                VerifyNumberPanelContent("12345More100");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);
                VerifyNumberPanelContent("12345More100");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);
                VerifyNumberPanelContent("12345More100");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(3);
                VerifyNumberPanelContent("12345More100");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(4);
                VerifyNumberPanelContent("1More456More100");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(5);
                VerifyNumberPanelContent("1More567More100");

                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(99);

                VerifyNumberPanelContent("1More96979899100");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(98);
                VerifyNumberPanelContent("1More96979899100");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(97);
                VerifyNumberPanelContent("1More96979899100");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(96);
                VerifyNumberPanelContent("1More96979899100");

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(95);
                VerifyNumberPanelContent("1More959697More100");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite","D")]
        public void NumberPanelInfiniteItemsTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();

                VerifyPageChanged(0);

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                elements.GetEnterInfinityModeButton().Click();

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(1);
                VerifyNumberPanelContent("12345");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(2);
                VerifyNumberPanelContent("12345");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(3);
                VerifyNumberPanelContent("1More345");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(4);
                VerifyNumberPanelContent("1More456");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(5);
                VerifyNumberPanelContent("1More567");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(6);
                VerifyNumberPanelContent("1More678");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(7);
                VerifyNumberPanelContent("1More789");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(8);
                VerifyNumberPanelContent("1More8910");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(9);
                VerifyNumberPanelContent("1More91011");

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChanged(10);
                VerifyNumberPanelContent("1More101112");

            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void FirstPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetLastPageButton());

                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetNextPageButton());

                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChanged(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void PreviousPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetNextPageButton());

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetLastPageButton());

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(3);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(2);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(1);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(0);

            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NextPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
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
        [TestProperty("TestSuite", "A")]
        public void LastPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(4);

                InputHelper.LeftClick(elements.GetFirstPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());

                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(4);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "D")]
        public void FirstPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "First");
        }

        [TestMethod]
        [TestProperty("TestSuite", "E")]
        public void PreviousPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Previous");
        }

        [TestMethod]
        [TestProperty("TestSuite", "F")]
        public void NextPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Next");
        }

        [TestMethod]
        [TestProperty("TestSuite", "G")]
        public void LastPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Last");
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void ChangingDisplayModeTest()
        {
            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {
                elements = new PagerControlTestPageElements();
                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SetNumberPanelDisplayMode();
                VerifyNumberPanelDisplayMode();

                ChangeNumberOfPages();
                VerifyNumberOfPages("100");

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                ChangeNumberOfPages();
                VerifyNumberOfPages("5");

                VerifyAutoDisplayMode();

                IncrementNumberOfPages(4);
                VerifyNumberOfPages("9");

                VerifyAutoDisplayMode();

                IncrementNumberOfPages(1);
                VerifyNumberOfPages("10");

                VerifyAutoDisplayMode();

                IncrementNumberOfPages(1);
                VerifyNumberOfPages("11");

                VerifyAutoDisplayMode();
            }
        }
    }
}
