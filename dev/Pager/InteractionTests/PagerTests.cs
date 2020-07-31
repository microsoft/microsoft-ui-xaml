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
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                foreach (ButtonVisibilityMode visMode in Enum.GetValues(typeof(ButtonVisibilityMode)))
                {
                    SetFirstPageButtonVisibilityMode(visMode);

                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        if (visMode != ButtonVisibilityMode.None)
                        {
                            ClickButton(elements.GetFirstPageButton(), 0);
                        }
                        else // If the first page button is not visible then we resort to using the combobox to navigate to the first page.
                        {
                            SelectValueInPagerComboBox(0);
                            VerifyPageChangedEventOutput(0);
                        }
                    }

                    // Check the button when on the first page.
                    var shouldBeEnabled = false;
                    var expectedVisMode = (visMode == ButtonVisibilityMode.None || (visMode == ButtonVisibilityMode.HiddenOnEdge && previousPage == 0))
                        ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetFirstPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetNextPageButton(), 1);
                    
                    // Check the button when on the second page (not on an edge in the page range).
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetFirstPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetLastPageButton(), 4);

                    // Check the button when on the last page.
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetFirstPageButton(), expectedVisMode, shouldBeEnabled);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void PreviousPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                foreach (ButtonVisibilityMode visMode in Enum.GetValues(typeof(ButtonVisibilityMode)))
                {
                    SetPreviousPageButtonVisibilityMode(visMode);

                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        ClickButton(elements.GetFirstPageButton(), 0);
                    }

                    // Check the button when on the first page.
                    var shouldBeEnabled = false;
                    var expectedVisMode = (visMode == ButtonVisibilityMode.None || (visMode == ButtonVisibilityMode.HiddenOnEdge && previousPage == 0))
                        ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetPreviousPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetNextPageButton(), 1);

                    // Check the button when on the second page (not on an edge in the page range).
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetPreviousPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetLastPageButton(), 4);

                    // Check the button when on the last page.
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetPreviousPageButton(), expectedVisMode, shouldBeEnabled);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void NextPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                foreach (ButtonVisibilityMode visMode in Enum.GetValues(typeof(ButtonVisibilityMode)))
                {
                    SetNextPageButtonVisibilityMode(visMode);

                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        ClickButton(elements.GetFirstPageButton(), 0);
                    }

                    // Check the button when on the first page.
                    var shouldBeEnabled = true;
                    var expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetNextPageButton(), expectedVisMode, shouldBeEnabled);
                    if (visMode != ButtonVisibilityMode.None)
                    {
                        ClickButton(elements.GetNextPageButton(), 1);
                    }
                    else
                    {
                        // Since the next page button is disabled due to the visibility mode, we resort to using the combobox to go to the next page.
                        SelectValueInPagerComboBox(1);
                        VerifyPageChangedEventOutput(1);
                    }

                    // Check the button when on the second page (not on an edge in the page range).
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetNextPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetLastPageButton(), 4);

                    // Check the button when on the last page.
                    shouldBeEnabled = false;
                    expectedVisMode = (visMode == ButtonVisibilityMode.None || (visMode == ButtonVisibilityMode.HiddenOnEdge && previousPage == 4))
                        ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetNextPageButton(), expectedVisMode, shouldBeEnabled);
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void LastPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                foreach (ButtonVisibilityMode visMode in Enum.GetValues(typeof(ButtonVisibilityMode)))
                {
                    SetLastPageButtonVisibilityMode(visMode);

                    // If we're not on the first page then navigate to the first page.
                    if (previousPage != 0)
                    {
                        ClickButton(elements.GetFirstPageButton(), 0);
                    }

                    // Check the button when on the first page.
                    var shouldBeEnabled = true;
                    var expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetLastPageButton(), expectedVisMode, shouldBeEnabled);
                    ClickButton(elements.GetNextPageButton(), 1);

                    // Check the button when on the second page (not on an edge in the page range).
                    shouldBeEnabled = true;
                    expectedVisMode = visMode == ButtonVisibilityMode.None ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetLastPageButton(), expectedVisMode, shouldBeEnabled);
                    if (visMode != ButtonVisibilityMode.None)
                    {
                        ClickButton(elements.GetLastPageButton(), 4);
                    } 
                    else
                    {
                        // If the Last Page button is not visible, then we resort to using the combobox to go to the last page.
                        SelectValueInPagerComboBox(4);
                        VerifyPageChangedEventOutput(4);
                    }

                    // Check the button when on the last page.
                    shouldBeEnabled = false;
                    expectedVisMode = (visMode == ButtonVisibilityMode.None || (visMode == ButtonVisibilityMode.HiddenOnEdge && previousPage == 4))
                        ? Visibility.Collapsed : Visibility.Visible;
                    VerifyButton(elements.GetLastPageButton(), expectedVisMode, shouldBeEnabled);
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

        void SetFirstPageButtonVisibilityMode(ButtonVisibilityMode mode)
        {
            if (mode == ButtonVisibilityMode.Auto)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("AutoFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.AlwaysVisible)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.HiddenOnEdge)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeFirstPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.None)
            {
                elements.GetFirstPageButtonVisibilityComboBox().SelectItemByName("NoneFirstPageButtonVisibilityItem");
            }
        }

        void SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode mode)
        {
            if (mode == ButtonVisibilityMode.Auto)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("AutoPreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.AlwaysVisible)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisiblePreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.HiddenOnEdge)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgePreviousPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.None)
            {
                elements.GetPreviousPageButtonVisibilityComboBox().SelectItemByName("NonePreviousPageButtonVisibilityItem");
            }
        }
        
        void SetNextPageButtonVisibilityMode(ButtonVisibilityMode mode)
        {
            if (mode == ButtonVisibilityMode.Auto)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("AutoNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.AlwaysVisible)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.HiddenOnEdge)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeNextPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.None)
            {
                elements.GetNextPageButtonVisibilityComboBox().SelectItemByName("NoneNextPageButtonVisibilityItem");
            }
        }
        
        void SetLastPageButtonVisibilityMode(ButtonVisibilityMode mode)
        {
            if (mode == ButtonVisibilityMode.Auto)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("AutoLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.AlwaysVisible)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("AlwaysVisibleLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.HiddenOnEdge)
            {
                elements.GetLastPageButtonVisibilityComboBox().SelectItemByName("HiddenOnEdgeLastPageButtonVisibilityItem");
            }
            else if (mode == ButtonVisibilityMode.None)
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
