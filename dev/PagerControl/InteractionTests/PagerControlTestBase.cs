// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;

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
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    public class PagerControlTestBase
    {
        protected PagerControlTestPageElements elements;
        protected int AutoDisplayModeThresholdValue = 10;
        protected delegate void SetButtonVisibilityModeFunction(ButtonVisibilityModes mode);

        protected void SelectValueInPagerComboBox(int index)
        {
            // Open ComboBox.
            InputHelper.LeftClick(elements.GetPagerComboBox());
            Wait.ForIdle();
            // Index is the actual PagerControl SelectedIndex, so we need to add 1 here 
            // to result in the user friendly name being displayed in the ComboBox.
            elements.GetPagerComboBox().SelectItemByName((index + 1).ToString());
        }

        protected void SendValueToNumberBox(string value)
        {
            Edit textbox = FindTextBox(elements.GetPagerNumberBox());

            Verify.IsNotNull(textbox);

            KeyboardHelper.EnterText(textbox, value);
            KeyboardHelper.PressKey(Key.Enter);
            Wait.ForIdle();
        }

        protected void SelectPageInNumberPanel(int index)
        {
            InputHelper.LeftClick(elements.GetNumberPanelButton("Page " + index.ToString()));
            Wait.ForIdle();
        }

        protected Edit FindTextBox(UIObject parent)
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

        protected int GetLastPageAsInt()
        {
            return Convert.ToInt32(GetLastPage());
        }
        protected string GetLastPage()
        {
            return elements.GetNumberOfPagesTextBlock().GetText();
        }

        protected int GetPreviousPageAsInt()
        {
            return Convert.ToInt32(GetPreviousPage());
        }
        protected string GetPreviousPage()
        {
            return elements.GetPreviousPageTextBlock().GetText();
        }

        protected int GetCurrentPageAsInt()
        {
            return Convert.ToInt32(GetCurrentPage());
        }
        protected string GetCurrentPage()
        {
            return elements.GetCurrentPageTextBlock().GetText();
        }

        protected void ChangeNumberOfPages()
        {
            InputHelper.LeftClick(elements.GetNumberOfPagesSetterButton());
        }

        protected void IncrementNumberOfPages(int numberOfPagesToAdd)
        {
            for (int i = 0; i < numberOfPagesToAdd; i++)
            {
                InputHelper.LeftClick(elements.GetIncreaseNumberOfPagesButton());
            }
        }

        protected void VerifyNumberOfPages(string expectedPages)
        {
            Verify.AreEqual(expectedPages, elements.GetNumberOfPagesTextBlock().GetText());
        }

        protected void VerifyNumberPanelContent(string expectedContent)
        {
            Verify.AreEqual(expectedContent, elements.GetNumberPanelContentTextBlock().GetText());
        }

        protected void VerifyPageChanged(int expectedPage)
        {
            Verify.AreEqual(expectedPage, GetCurrentPageAsInt());
            Log.Comment($"Changing to page {expectedPage}");
        }

        protected void VerifyButton(UIObject button, Visibility expectedVisibility, bool shouldBeEnabled)
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

        protected void VerifyFirstPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetFirstPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyFirstPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetFirstPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyPreviousPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetPreviousPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyPreviousPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetPreviousPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyNextPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNextPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyNextPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNextPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyLastPageButtonVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetLastPageButtonVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyLastPageButtonIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetLastPageButtonIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void SetFirstPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.AlwaysVisible)
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

        protected void SetPreviousPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.AlwaysVisible)
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

        protected void SetNextPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.AlwaysVisible)
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

        protected void SetLastPageButtonVisibilityMode(ButtonVisibilityModes mode)
        {
            if (mode == ButtonVisibilityModes.AlwaysVisible)
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

        protected void SetAutoDisplayMode()
        {
            SetDisplayMode("AutoDisplayModeItem");
        }

        protected void SetNumberBoxDisplayMode()
        {
            SetDisplayMode("NumberBoxDisplayModeItem");
        }

        protected void SetComboBoxDisplayMode()
        {
            SetDisplayMode("ComboBoxDisplayModeItem");
        }

        protected void SetNumberPanelDisplayMode()
        {
            SetDisplayMode("NumberPanelDisplayModeItem");
        }

        protected void SetDisplayMode(string mode)
        {
            elements.GetDisplayModeComboBox().SelectItemByName(mode);
            Wait.ForIdle();
        }

        protected void VerifyAutoDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.Auto);
        }

        protected void VerifyNumberBoxDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.NumberBox);
        }

        protected void VerifyComboBoxDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.ComboBox);
        }

        protected void VerifyNumberPanelDisplayMode()
        {
            VerifyDisplayMode(DisplayModes.NumberPanel);
        }

        protected void VerifyDisplayMode(DisplayModes mode)
        {
            switch (mode)
            {
                case DisplayModes.Auto:
                    if (Convert.ToInt32(elements.GetNumberOfPagesTextBlock().GetText()) < AutoDisplayModeThresholdValue)
                    {
                        VerifyComboBoxEnabled();
                        VerifyNumberBoxDisabled();
                        VerifyNumberPanelDisabled();
                    }
                    else
                    {
                        VerifyNumberBoxEnabled();
                        VerifyComboBoxDisabled();
                        VerifyNumberPanelDisabled();
                    }
                    break;
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

        protected void VerifyComboBoxEnabled()
        {
            VerifyComboBoxVisibility(Visibility.Visible);
            VerifyComboBoxIsEnabled(true);
        }

        protected void VerifyNumberBoxEnabled()
        {
            VerifyNumberBoxVisibility(Visibility.Visible);
            VerifyNumberBoxIsEnabled(true);
        }

        protected void VerifyNumberPanelEnabled()
        {
            VerifyNumberPanelVisibility(Visibility.Visible);
        }

        protected void VerifyComboBoxDisabled()
        {
            VerifyComboBoxVisibility(Visibility.Collapsed);
        }

        protected void VerifyNumberBoxDisabled()
        {
            VerifyNumberBoxVisibility(Visibility.Collapsed);
        }

        protected void VerifyNumberPanelDisabled()
        {
            VerifyNumberPanelVisibility(Visibility.Collapsed);
        }

        protected void VerifyComboBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetComboBoxVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyComboBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetComboBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyNumberBoxVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNumberBoxVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyNumberBoxIsEnabled(bool expected)
        {
            Verify.AreEqual(expected, elements.GetNumberBoxIsEnabledCheckBox().ToggleState == ToggleState.On);
        }

        protected void VerifyNumberPanelVisibility(Visibility expected)
        {
            Verify.AreEqual(expected == Visibility.Visible, elements.GetNumberPanelVisibilityCheckBox().ToggleState == ToggleState.On);
        }

        protected void ButtonVisibilityOptionsTest(string buttonNamePrefix)
        {
            SetButtonVisibilityModeFunction SetButtonVisibilityMode;
            UIObject buttonBeingTested;

            using (var setup = new TestSetupHelper("PagerControl Tests"))
            {

                elements = new PagerControlTestPageElements();
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
                    if (GetCurrentPageAsInt() != 0)
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

        public enum DisplayModes
        {
            Auto,
            NumberBox,
            ComboBox,
            NumberPanel
        };

        public enum ButtonVisibilityModes
        {
            AlwaysVisible,
            HiddenOnEdge,
            None
        }

        public enum Visibility
        {
            Visible,
            Collapsed
        }
    }
}
