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
                VerifyPageChangedEventOutput(0);

                SendValueToNumberBox("3"); // Note: Pager displays numbers starting at 1 but the page changed event sends 0-based numbers
                VerifyPageChangedEventOutput(2);

                SendValueToNumberBox("1");
                VerifyPageChangedEventOutput(0);

                SendValueToNumberBox("5");
                VerifyPageChangedEventOutput(4);

                SendValueToNumberBox("2");
                VerifyPageChangedEventOutput(1);

                SendValueToNumberBox("100");
                Verify.AreEqual("5", FindTextBox(elements.GetPagerNumberBox()).GetText()); // If over max, value should be clamped down to the max.
                VerifyPageChangedEventOutput(4);

                SendValueToNumberBox("-100");
                Verify.AreEqual("1", FindTextBox(elements.GetPagerNumberBox()).GetText()); // If under min, value should be clamped up to the min.
                VerifyPageChangedEventOutput(0);
                
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ComboBoxDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SelectValueInPagerComboBox(2);
                VerifyPageChangedEventOutput(2);

                SelectValueInPagerComboBox(4);
                VerifyPageChangedEventOutput(4);

                SelectValueInPagerComboBox(0);
                VerifyPageChangedEventOutput(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AutoDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SelectValueInPagerComboBox(2);
                VerifyPageChangedEventOutput(2);

                SelectValueInPagerComboBox(4);
                VerifyPageChangedEventOutput(4);

                SelectValueInPagerComboBox(0);
                VerifyPageChangedEventOutput(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void FirstPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);
                InputHelper.LeftClick(elements.GetLastPageButton());

                previousPage = 4;
                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChangedEventOutput(0);

                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;
                InputHelper.LeftClick(elements.GetFirstPageButton());
                VerifyPageChangedEventOutput(0);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void PreviousPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);
                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;
                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChangedEventOutput(0);

                InputHelper.LeftClick(elements.GetLastPageButton());

                previousPage = 4;
                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChangedEventOutput(3);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChangedEventOutput(2);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChangedEventOutput(1);

                InputHelper.LeftClick(elements.GetPreviousPageButton());
                VerifyPageChangedEventOutput(0);

            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NextPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(1);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(2);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(3);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(4);
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void LastPageButtonChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);
                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChangedEventOutput(4);

                InputHelper.LeftClick(elements.GetFirstPageButton());
                InputHelper.LeftClick(elements.GetNextPageButton());

                previousPage = 1;

                InputHelper.LeftClick(elements.GetLastPageButton());
                VerifyPageChangedEventOutput(4);
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

                VerifyPageChangedEventOutput(0);

                foreach (ButtonVisibilityModes visMode in Enum.GetValues(typeof(ButtonVisibilityModes)))
                {
                    SetButtonVisibilityMode(visMode);

                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        SelectValueInPagerComboBox(0);
                        VerifyPageChangedEventOutput(0);
                    }

                    var expectedVisibility = ((visMode == ButtonVisibilityModes.None) ||
                        (visMode == ButtonVisibilityModes.HiddenOnEdge &&
                        (buttonNamePrefix == "First" || buttonNamePrefix == "Previous"))) ? Visibility.Collapsed : Visibility.Visible;

                    var expectedIsEnableValue = (buttonNamePrefix == "First" || buttonNamePrefix == "Previous") ? false : true;

                    VerifyButton(buttonBeingTested, expectedVisibility, expectedIsEnableValue);

                    SelectValueInPagerComboBox(1);
                    VerifyPageChangedEventOutput(1);

                    expectedVisibility = (visMode == ButtonVisibilityModes.None) ? Visibility.Collapsed : Visibility.Visible;
                    expectedIsEnableValue = true;

                    VerifyButton(buttonBeingTested, expectedVisibility, expectedIsEnableValue);

                    SelectValueInPagerComboBox(4);
                    VerifyPageChangedEventOutput(4);

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

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SetAutoDisplayMode();
                VerifyAutoDisplayMode();
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

        void VerifyPageChangedEventOutput(int expectedPage)
        {
            string expectedText = $"Page changed from page {previousPage} to page {expectedPage}";
            Log.Comment($"Changing to page {expectedPage} from {previousPage}");
            previousPage = expectedPage;
            Verify.AreEqual(expectedText, elements.GetLastEventOutput());
        }

        void ClickButton(UIObject element, int expectedPageNumber)
        {
            InputHelper.LeftClick(element);
            VerifyPageChangedEventOutput(expectedPageNumber);
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
            Verify.AreEqual(expected.ToString(), elements.GetFirstPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyFirstPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetFirstPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyPreviousPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected.ToString(), elements.GetPreviousPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyPreviousPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetPreviousPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNextPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected.ToString(), elements.GetNextPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyNextPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNextPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyLastPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected.ToString(), elements.GetLastPageButtonVisibilityTextBlock().GetText());
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
            elements.ComboBoxDisplayModeActive = false;
            elements.NumberBoxDisplayModeActive = true;
            SetDisplayMode("NumberBoxDisplayModeItem");
        }
        
        void SetComboBoxDisplayMode()
        {
            elements.ComboBoxDisplayModeActive = true;
            elements.NumberBoxDisplayModeActive = false;
            SetDisplayMode("ComboBoxDisplayModeItem");
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

        void VerifyDisplayMode(DisplayModes mode)
        {
            switch (mode)
            {
                case DisplayModes.Auto:
                case DisplayModes.ComboBox:
                    VerifyComboBoxEnabled();
                    VerifyNumberBoxDisabled();
                    break;
                case DisplayModes.NumberBox:
                    VerifyComboBoxDisabled();
                    VerifyNumberBoxEnabled();
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

        void VerifyComboBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected.ToString(), elements.GetComboBoxVisibilityTextBlock().GetText());
        }

        void VerifyComboBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetComboBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNumberBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected.ToString(), elements.GetNumberBoxVisibilityTextBlock().GetText());
        }

        void VerifyNumberBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNumberBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }
    }
}
