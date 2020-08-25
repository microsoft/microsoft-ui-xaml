// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

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
using static Windows.UI.Xaml.Tests.MUXControls.InteractionTests.PagerTestsPageElements;
using Windows.UI.Xaml.Controls.Primitives;
using System.Diagnostics;
using Windows.UI.Xaml.Controls;
using Windows.UI.Core;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class PagerTests
    {
        PagerTestsPageElements elements;
        int previousPage = -1;
        delegate void SetButtonVisibilityModeFunction(ButtonVisibilityModes mode);

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
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();
                VerifyPageChanged(0);

                SendValueToNumberBox("3"); // Note: Pager displays numbers starting at 1 but the page changed event sends 0-based numbers
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
        public void ComboBoxDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
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
        [TestProperty("TestSuite", "A")]
        public void AutoDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
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
        [TestProperty("TestSuite", "C")]
        public void NumberPanelChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

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
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

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
        [TestProperty("TestSuite", "A")]
        public void FirstPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetLastPageButton());

                previousPage = 4;
                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;
                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChanged(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void PreviousPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;
                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChanged(0);

                InputHelper.LeftClick(elements.GetLastPageButton());

                previousPage = 4;
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
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
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
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChanged(0);
                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(4);

                InputHelper.LeftClick(elements.GetFirstPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;

                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChanged(4);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void FirstPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "First");
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void PreviousPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Previous");
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NextPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Next");
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void LastPageButtonVisibilityOptionsTest()
        {
            ButtonVisibilityOptionsTest(buttonNamePrefix: "Last");
        }

        void ButtonVisibilityOptionsTest(string buttonNamePrefix)
        {
            SetButtonVisibilityModeFunction SetButtonVisibilityMode;
            UIObject buttonBeingTested;

            using (var setup = new TestSetupHelper("Pager Tests"))
            {

                elements = new PagerTestsPageElements();
                switch (buttonNamePrefix)
                {
                    case "First":
                        SetButtonVisibilityMode = SetFirstPageButtonVisibilityMode;
                        buttonBeingTested = elements.GetFirstPageButton();
                        break;
                    case "Previous":
                        SetButtonVisibilityMode = SetPreviousPageButtonVisibilityMode;
                        buttonBeingTested = elements.GetPreviousPageButton();
                        break;
                    case "Next":
                        SetButtonVisibilityMode = SetNextPageButtonVisibilityMode;
                        buttonBeingTested = elements.GetNextPageButton();
                        break;
                    case "Last":
                        SetButtonVisibilityMode = SetLastPageButtonVisibilityMode;
                        buttonBeingTested = elements.GetLastPageButton();
                        break;
                    default:
                        Log.Warning("This test is being skipped because the button string was not one of these four strings: [First, Previous, Next, Last]");
                        return;
                }

                VerifyPageChanged(0);

                foreach (ButtonVisibilityModes visMode in Enum.GetValues(typeof(ButtonVisibilityModes)))
                {
                    SetButtonVisibilityMode(visMode);
                    GetLastPage();
                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        SelectValueInPagerComboBox(0);
                        VerifyPageChanged(0);
                    }

                    var expectedVisibility = ((visMode == ButtonVisibilityModes.None) ||
                        (visMode == ButtonVisibilityModes.HiddenOnEdge &&
                        (buttonNamePrefix == "First" || buttonNamePrefix == "Previous"))) ? Visibility.Collapsed : Visibility.Visible;

                    var expectedIsEnableValue = (buttonNamePrefix == "First" || buttonNamePrefix == "Previous") ? false : true;

                    VerifyButton(buttonBeingTested, expectedVisibility, expectedIsEnableValue);

                    SelectValueInPagerComboBox(1);
                    VerifyPageChanged(1);

                    expectedVisibility = (visMode == ButtonVisibilityModes.None) ? Visibility.Collapsed : Visibility.Visible;
                    expectedIsEnableValue = true;

                    VerifyButton(buttonBeingTested, expectedVisibility, expectedIsEnableValue);

                    SelectValueInPagerComboBox(GetLastPageAsInt() - 1);
                    VerifyPageChanged(GetLastPageAsInt() - 1);

                    expectedVisibility = ((visMode == ButtonVisibilityModes.None) ||
                        (visMode == ButtonVisibilityModes.HiddenOnEdge &&
                        (buttonNamePrefix == "Next" || buttonNamePrefix == "Last"))) ? Visibility.Collapsed : Visibility.Visible;

                    expectedIsEnableValue = (buttonNamePrefix == "Next" || buttonNamePrefix == "Last") ? false : true;

                    VerifyButton(buttonBeingTested, expectedVisibility, expectedIsEnableValue);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ChangingDisplayModeTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
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
            }
        }

        void SelectValueInPagerComboBox(int index)
        {
            InputHelper.LeftClick(elements.GetPagerComboBox());
            InputHelper.LeftClick(elements.GetPagerComboBox().Children[index]);
        }

        void SendValueToNumberBox(string value)
        {
            Edit textbox = FindTextBox(elements.GetPagerNumberBox());

            Verify.IsNotNull(textbox);

            KeyboardHelper.EnterText(textbox, value);
            KeyboardHelper.PressKey(Key.Enter);
            Wait.ForIdle();
        }

        void SelectPageInNumberPanel(int index)
        {
            InputHelper.LeftClick(elements.GetNumberPanelButton("Page Button " + index));
        }

        Edit FindTextBox(UIObject parent)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.ClassName.Equals("TextBox"))
                {
                    return new Edit(elem);
                }
            }
            Log.Comment("Did not find TextBox for object " + parent.Name);
            return null;
        }

        int GetLastPageAsInt()
        {
            return Convert.ToInt32(GetLastPage());
        }
        string GetLastPage()
        {
            return elements.GetNumberOfPagesTextBlock().GetText();
        }

        int GetPreviousPageAsInt()
        {
            return Convert.ToInt32(GetPreviousPage());
        }
         string GetPreviousPage()
        {
            return elements.GetPreviousPageTextBlock().GetText();
        }

        int GetCurrentPageAsInt()
        {
            return Convert.ToInt32(GetCurrentPage());
        }
        string GetCurrentPage()
        {
            return elements.GetCurrentPageTextBlock().GetText();
        }

        void ChangeNumberOfPages()
        {
            InputHelper.LeftClick(elements.GetNumberOfPagesSetterButton());
        }
        
        void VerifyNumberOfPages(string expectedPages)
        {
            Verify.AreEqual(expectedPages, elements.GetNumberOfPagesTextBlock().GetText());
        }

        void VerifyNumberPanelContent(string expectedContent)
        {
            Verify.AreEqual(expectedContent, elements.GetNumberPanelContentTextBlock().GetText());
        }

        void VerifyPageChanged(int expectedPage)
        {
            Verify.AreEqual(expectedPage, GetCurrentPageAsInt());
            Verify.AreEqual(previousPage, GetPreviousPageAsInt());
            Log.Comment($"Changing to page {expectedPage} from {previousPage}");
            previousPage = expectedPage;
        }

        void VerifyButton(UIObject button, Visibility expectedVisibility, bool shouldBeEnabled)
        {

            if (button == elements.GetFirstPageButton())
            {
                VerifyFirstPageButtonVisibility(expectedVisibility);
                VerifyFirstPageButtonIsEnabled(shouldBeEnabled);
            }
            else if (button == elements.GetPreviousPageButton())
            {
                VerifyPreviousPageButtonVisibility(expectedVisibility);
                VerifyPreviousPageButtonIsEnabled(shouldBeEnabled);
            }
            else if (button == elements.GetNextPageButton())
            {
                VerifyNextPageButtonVisibility(expectedVisibility);
                VerifyNextPageButtonIsEnabled(shouldBeEnabled);
            }
            else if (button == elements.GetLastPageButton())
            {
                VerifyLastPageButtonVisibility(expectedVisibility);
                VerifyLastPageButtonIsEnabled(shouldBeEnabled);
            }
        }

        void VerifyFirstPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetFirstPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyFirstPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetFirstPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyPreviousPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetPreviousPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyPreviousPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetPreviousPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNextPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNextPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNextPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNextPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyLastPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetLastPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyLastPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetLastPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void SetFirstPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.Auto)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("AutoFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.AlwaysVisible)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.HiddenOnEdge)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.None)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("NoneFirstPageButtonVisibilityItem");
            }
        }

        void SetPreviousPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.Auto)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("AutoPreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.AlwaysVisible)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisiblePreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.HiddenOnEdge)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgePreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.None)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("NonePreviousPageButtonVisibilityItem");
            }
        }
        
        void SetNextPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.Auto)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("AutoNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.AlwaysVisible)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.HiddenOnEdge)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.None)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("NoneNextPageButtonVisibilityItem");
            }
        }
        
        void SetLastPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.Auto)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("AutoLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.AlwaysVisible)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.HiddenOnEdge)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityModes.None)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("NoneLastPageButtonVisibilityItem");
            }
        }

        void SetAutoDisplayMode()
        {
            SetDisplayMode("AutoDisplayModeItem");
        }
        
        void SetNumberBoxDisplayMode()
        {
            SetDisplayMode("NumberBoxDisplayModeItem");
        }
        
        void SetComboBoxDisplayMode()
        {
            SetDisplayMode("ComboBoxDisplayModeItem");
        }

        void SetNumberPanelDisplayMode()
        {
            SetDisplayMode("NumberPanelDisplayModeItem");
        }
        
        void SetDisplayMode(string mode)
        {
            elements.GetDisplayModeComboBox().SelectItemByName(mode);
        }

        void VerifyAutoDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.Auto);
        }

        void VerifyNumberBoxDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.NumberBox);
        }

        void VerifyComboBoxDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.ComboBox);
        }

        void VerifyNumberPanelDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.NumberPanel);
        }

        void VerifyDisplayMode(DisplayModes mode)
        {
            switch (mode)
            {
                case DisplayModes.Auto:
                case DisplayModes.ComboBox:
                    VerifyComboBoxEnabled();
                    VerifyNumberBoxDisabled();
                    VerifyNumberPanelDisabled();
                    break;
                case DisplayModes.NumberBox:
                    VerifyComboBoxDisabled();
                    VerifyNumberBoxEnabled();
                    VerifyNumberPanelDisabled();
                    break;
                case DisplayModes.NumberPanel:
                    VerifyComboBoxDisabled();
                    VerifyNumberBoxDisabled();
                    VerifyNumberPanelEnabled();
                    break;
                default:
                    break;
            }
        }

        void VerifyComboBoxEnabled()
        {
            VerifyComboBoxVisibility(Visibility.Visible);
            VerifyComboBoxIsEnabled(true);
        }

        void VerifyNumberBoxEnabled()
        {
            VerifyNumberBoxVisibility(Visibility.Visible);
            VerifyNumberBoxIsEnabled(true);
        }

        void VerifyNumberPanelEnabled()
        {
            VerifyNumberPanelVisibility(Visibility.Visible);
        }

        void VerifyComboBoxDisabled()
        {
            VerifyComboBoxVisibility(Visibility.Collapsed);
            VerifyComboBoxIsEnabled(false);
        }

        void VerifyNumberBoxDisabled()
        {
            VerifyNumberBoxVisibility(Visibility.Collapsed);
            VerifyNumberBoxIsEnabled(false);
        }

        void VerifyNumberPanelDisabled()
        {
            VerifyNumberPanelVisibility(Visibility.Collapsed);
        }

        void VerifyComboBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetComboBoxVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyComboBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetComboBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNumberBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNumberBoxVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNumberBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNumberBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNumberPanelVisibility(Visibility expected) 
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNumberPanelVisibilityCheckBox().ToggleState == ToggleState.On);
        }
    }
}
