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
        public void ComboBoxDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();

                SelectValueInPagerCombBox(2);
                VerifyPageChangedEventOutput(2);

                SelectValueInPagerCombBox(4);
                VerifyPageChangedEventOutput(4);

                SelectValueInPagerCombBox(0);
                VerifyPageChangedEventOutput(0);
            }
        }

        [TestMethod]
        public void AutoDisplayChangingPageTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                VerifyPageChangedEventOutput(0);

                VerifyAutoDisplayMode();

                SelectValueInPagerCombBox(2);
                VerifyPageChangedEventOutput(2);

                SelectValueInPagerCombBox(4);
                VerifyPageChangedEventOutput(4);

                SelectValueInPagerCombBox(0);
                VerifyPageChangedEventOutput(0);
            }
        }

        [TestMethod]
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
        public void FirstPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetFirstPageButtonVisibilityMode(ButtonVisibilityMode.Auto);
                VerifyPageChangedEventOutput(0);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled(false);

                SetFirstPageButtonVisibilityMode(ButtonVisibilityMode.AlwaysVisible);

                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled(false);

                SetFirstPageButtonVisibilityMode(ButtonVisibilityMode.HiddenOnEdge);

                VerifyFirstPageButtonVisibility(Visibility.Collapsed);
                VerifyFirstPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyFirstPageButtonVisibility();
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyFirstPageButtonVisibility(Visibility.Collapsed);
                VerifyFirstPageButtonIsEnabled(false);

                SetFirstPageButtonVisibilityMode(ButtonVisibilityMode.None);

                VerifyFirstPageButtonVisibility(Visibility.Collapsed);
                VerifyFirstPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyFirstPageButtonVisibility(Visibility.Collapsed);
                VerifyFirstPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyFirstPageButtonVisibility(Visibility.Collapsed);
                VerifyFirstPageButtonIsEnabled();
            }
        }

        [TestMethod]
        public void PreviousPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.Auto);

                VerifyPageChangedEventOutput(0);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled(false);

                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.AlwaysVisible);

                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled(false);

                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.HiddenOnEdge);

                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyPreviousPageButtonVisibility();
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled(false);

                SetPreviousPageButtonVisibilityMode(ButtonVisibilityMode.None);

                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyPreviousPageButtonVisibility(Visibility.Collapsed);
                VerifyPreviousPageButtonIsEnabled(false);
            }
        }

        [TestMethod]
        public void NextPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.Auto);

                VerifyPageChangedEventOutput(0);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.AlwaysVisible);

                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.HiddenOnEdge);

                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyNextPageButtonVisibility(Visibility.Collapsed);
                VerifyNextPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyNextPageButtonVisibility();
                VerifyNextPageButtonIsEnabled();

                SetNextPageButtonVisibilityMode(ButtonVisibilityMode.None);

                VerifyNextPageButtonVisibility(Visibility.Collapsed);
                VerifyNextPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyNextPageButtonVisibility(Visibility.Collapsed);
                VerifyNextPageButtonIsEnabled(false);

                InputHelper.LeftClick(elements.GetPreviousPageButton());

                VerifyPageChangedEventOutput(3);
                VerifyNextPageButtonVisibility(Visibility.Collapsed);
                VerifyNextPageButtonIsEnabled();

            }
        }

        [TestMethod]
        public void LastPageButtonVisibilityOptionsTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetLastPageButtonVisibilityMode(ButtonVisibilityMode.Auto);
                VerifyPageChangedEventOutput(0);
                VerifyLastPageButtonVisibility();
                VerifyLastPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyLastPageButtonIsEnabled(true);
                VerifyLastPageButtonVisibility();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyLastPageButtonIsEnabled(false);
                VerifyLastPageButtonVisibility();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyLastPageButtonIsEnabled();
                VerifyLastPageButtonVisibility();

                SetLastPageButtonVisibilityMode(ButtonVisibilityMode.AlwaysVisible);

                VerifyLastPageButtonVisibility();
                VerifyLastPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyLastPageButtonIsEnabled(true);
                VerifyLastPageButtonVisibility();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyLastPageButtonIsEnabled(false);
                VerifyLastPageButtonVisibility();

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyLastPageButtonIsEnabled();
                VerifyLastPageButtonVisibility();

                SetLastPageButtonVisibilityMode(ButtonVisibilityMode.HiddenOnEdge);

                VerifyLastPageButtonVisibility();
                VerifyLastPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyLastPageButtonIsEnabled(true);
                VerifyLastPageButtonVisibility();

                InputHelper.LeftClick(elements.GetLastPageButton());

                VerifyPageChangedEventOutput(4);
                VerifyLastPageButtonIsEnabled(false);
                VerifyLastPageButtonVisibility(Visibility.Collapsed);

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyLastPageButtonIsEnabled();
                VerifyLastPageButtonVisibility();

                SetLastPageButtonVisibilityMode(ButtonVisibilityMode.None);

                VerifyLastPageButtonVisibility(Visibility.Collapsed);
                VerifyLastPageButtonIsEnabled();

                InputHelper.LeftClick(elements.GetNextPageButton());

                VerifyPageChangedEventOutput(1);
                VerifyLastPageButtonIsEnabled();
                VerifyLastPageButtonVisibility(Visibility.Collapsed);

                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(2);
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(3);
                InputHelper.LeftClick(elements.GetNextPageButton());
                VerifyPageChangedEventOutput(4);

                VerifyLastPageButtonIsEnabled(false);
                VerifyLastPageButtonVisibility(Visibility.Collapsed);

                InputHelper.LeftClick(elements.GetFirstPageButton());

                VerifyPageChangedEventOutput(0);
                VerifyLastPageButtonIsEnabled();
                VerifyLastPageButtonVisibility(Visibility.Collapsed);

            }
        }

        [TestMethod]
        public void AutoDisplayModeTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();
                SetAutoDisplayMode();
                VerifyAutoDisplayMode();
            }
        }

        [TestMethod]
        public void NumberBoxDisplayModeTest()
        {
            using (var setup = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetNumberBoxDisplayMode();
                VerifyNumberBoxDisplayMode();
            }
        }

        [TestMethod]
        public void ComboBoxDisplayModeTest()
        {
            using (var setupp = new TestSetupHelper("Pager Tests"))
            {
                elements = new PagerTestsPageElements();

                SetComboBoxDisplayMode();
                VerifyComboBoxDisplayMode();
            }
        }

        [TestMethod]
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

        void SelectValueInPagerCombBox(int index)
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
            previousPage = expectedPage;
            Verify.AreEqual(expectedText, elements.GetLastEventOutput());
        }

        void VerifyFirstPageButtonVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetFirstPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyFirstPageButtonIsEnabled(bool expected = true)
        {
            Verify.AreEqual(expected, elements.GetFirstPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyPreviousPageButtonVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetPreviousPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyPreviousPageButtonIsEnabled(bool expected = true)
        {
            Verify.AreEqual(expected, elements.GetPreviousPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNextPageButtonVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetNextPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyNextPageButtonIsEnabled(bool expected = true)
        {
            Verify.AreEqual(expected, elements.GetNextPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyLastPageButtonVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetLastPageButtonVisibilityTextBlock().GetText());
        }

        void VerifyLastPageButtonIsEnabled(bool expected = true)
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
            VerifyComboBoxVisibility();
            VerifyComboBoxIsEnabled();
        }

        void VerifyNumberBoxEnabled()
        {
            VerifyNumberBoxVisibility();
            VerifyNumberBoxIsEnabled();
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

        void VerifyComboBoxVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetComboBoxVisibilityTextBlock().GetText());
        }

        void VerifyComboBoxIsEnabled(bool expected = true)
        {
            Verify.AreEqual(expected, elements.GetComboBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        void VerifyNumberBoxVisibility(Visibility expected = Visibility.Visible)
        {
            Verify.AreEqual(expected.ToString(), elements.GetNumberBoxVisibilityTextBlock().GetText());
        }

        void VerifyNumberBoxIsEnabled(bool expected = true)
        {
            Verify.AreEqual(expected, elements.GetNumberBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }
    }
}
