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
using static Windows.UI.Xaml.Tests.MUXControls.InteractionTests.RadioButtonsTestPageElements;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RadioButtonsTests
    {
        RadioButtonsTestPageElements elements;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void SelectionTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);

                    SelectByIndex(1);
                    VerifySelectedIndex(1);
                    RadioButton item1 = FindElement.ByName<RadioButton>("Radio Button 1");
                    Verify.IsTrue(item1.IsSelected);

                    SelectByItem(3);
                    VerifySelectedIndex(3);
                    RadioButton item3 = FindElement.ByName<RadioButton>("Radio Button 3");
                    Verify.IsTrue(item3.IsSelected);
                    Verify.IsFalse(item1.IsSelected);
                }
            }
        }

        [TestMethod]
        public void SelectByItem()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
            {
                Log.Warning("This test requires TrySetNewFocusedElement from RS4");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        if (location == RadioButtonsSourceLocation.Items && type == RadioButtonsSourceType.String)
                        {
                            // This is broken due to issue #1674. The items collection does not have a way to compare
                            // boxed strings for equality.
                            continue;
                        }

                        SetItemType(type);

                        SelectByItem(1);
                        VerifySelectedIndex(1);

                        SelectByItem(3);
                        VerifySelectedIndex(3);
                    }
                }
            }
        }

        [TestMethod]
        public void FocusComingFromAnotherRepeaterTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone4))
            {
                Log.Warning("This test requires TrySetNewFocusedElement from RS4");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);

                    TapOnItem(3);
                    VerifySelectedFocusedIndex(3);
                    RadioButton item3 = FindElement.ByName<RadioButton>("Radio Button 3");
                    Verify.IsTrue(item3.IsSelected);

                    KeyboardHelper.PressKey(Key.Tab);
                    VerifyRadioButtonsHasFocus(false);
                    Verify.IsTrue(item3.IsSelected);

                    KeyboardHelper.PressDownModifierKey(ModifierKey.Shift);
                    KeyboardHelper.PressKey(Key.Tab);
                    KeyboardHelper.ReleaseModifierKey(ModifierKey.Shift);
                    VerifySelectedFocusedIndex(3);
                    Verify.IsTrue(item3.IsSelected);

                }
            }
        }

        [TestMethod]
        public void BasicKeyboardTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        bool useBackup = type == RadioButtonsSourceType.String;
                        SetItemType(type);
                        TapOnItem(3, useBackup);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(4);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(3);

                        TapOnItem(0, useBackup);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(0);

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(9);
                    }
                }
            }
        }

        [TestMethod]
        public void MultiColumnKeyboardTest()
        {
            if(!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        bool useBackup = type == RadioButtonsSourceType.String;
                        SetNumberOfColumns(3);
                        TapOnItem(3, useBackup);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(4);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(6);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(6);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(2);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(2);

                        TapOnItem(0, useBackup);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(4);

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(6);
                    }
                }
            }
        }

        [TestMethod]
        public void DisabledItemsKeyboardTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetNumberOfColumns(3);
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);

                    SetNumberOfItems(10);
                    InsertDisabledRadioButton(10);
                    TapOnItem(7);
                    VerifySelectedFocusedIndex(7);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(9);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(5);
                    KeyboardHelper.PressKey(Key.Down);
                    VerifySelectedFocusedIndex(6);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(9);
                    //There is a known bug here where pressing down or right will put focus
                    //On the first item because RadioButton handles the event.
                    //RadioButtons doesn't catch this because it only catches when the
                    //Last item is focused, but that item is disabled here...
                    //Bug #1654
                    //KeyboardHelper.PressKey(Key.Down);
                    //VerifySelectedFocusedIndex(9);
                    //KeyboardHelper.PressKey(Key.Right);
                    //VerifySelectedFocusedIndex(9);

                    InsertDisabledRadioButton(6);
                    InsertDisabledRadioButton(6);

                    TapOnItem(1);
                    VerifySelectedFocusedIndex(1);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(10);
                    TapOnItem(2);
                    VerifySelectedFocusedIndex(2);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(11);
                    TapOnItem(5);
                    KeyboardHelper.PressKey(Key.Up);
                    VerifySelectedFocusedIndex(4);
                    KeyboardHelper.PressKey(Key.Down);
                    VerifySelectedFocusedIndex(5);
                    KeyboardHelper.PressKey(Key.Down);
                    VerifySelectedFocusedIndex(8);
                    TapOnItem(8);
                    VerifySelectedFocusedIndex(10);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(1);
                    TapOnItem(9);
                    VerifySelectedFocusedIndex(11);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(2);

                    ElementCache.Clear();
                }
            }
        }

        [TestMethod]
        public void DisabledItemsAtTopOfColumnKeyboardTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetNumberOfColumns(3);
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);

                    SetNumberOfItems(10);
                    InsertDisabledRadioButton(5);
                    InsertDisabledRadioButton(5);
                    InsertDisabledRadioButton(5);

                    TapOnItem(0);
                    VerifySelectedFocusedIndex(0);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(9);
                    TapOnItem(1);
                    VerifySelectedFocusedIndex(1);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(10);
                    TapOnItem(2);
                    VerifySelectedFocusedIndex(2);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(11);
                    TapOnItem(3);
                    VerifySelectedFocusedIndex(3);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(8);
                    TapOnItem(4);
                    VerifySelectedFocusedIndex(4);
                    KeyboardHelper.PressKey(Key.Right);
                    VerifySelectedFocusedIndex(8);
                    TapOnItem(6);
                    VerifySelectedFocusedIndex(9);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(0);
                    TapOnItem(7);
                    VerifySelectedFocusedIndex(10);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(1);
                    TapOnItem(8);
                    VerifySelectedFocusedIndex(11);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(2);
                    TapOnItem(9);
                    VerifySelectedFocusedIndex(12);
                    KeyboardHelper.PressKey(Key.Left);
                    VerifySelectedFocusedIndex(8);

                    ElementCache.Clear();
                }
            }
        }

        public void RS2MultiColumnKeyboardTest()
        {
            if (!PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS2 keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        bool useBackup = type == RadioButtonsSourceType.String;
                        SetNumberOfColumns(3);
                        TapOnItem(3, useBackup);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifyFocusedIndex(4);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifyFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifyFocusedIndex(2);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifyFocusedIndex(3);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifyFocusedIndex(4);
                        VerifySelectedIndex(3);

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifyFocusedIndex(8);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifyFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifyFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Tab);
                        VerifyFocusedIndex(1);
                        VerifySelectedIndex(9);

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Tab);
                        VerifyRadioButtonsHasFocus(false);
                    }
                }
            }
        }

        [TestMethod]
        [TestProperty("Ignore", "True")] // Disabled due to: RadioButtonsTests.GamepadCanEscapeAndDoesNotSelectWithFocus is unreliable. #2202
        public void GamepadCanEscapeAndDoesNotSelectWithFocus()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        bool useBackup = type == RadioButtonsSourceType.String;
                        SetNumberOfColumns(3);

                        // On RS2 the keyboarding model is different which results in different behavior here.
                        if (PlatformConfiguration.IsOSVersionLessThan(OSVersion.Redstone3))
                        {
                            TapOnItem(3, useBackup);
                            VerifySelectedFocusedIndex(3);
                        }
                        else
                        {
                            TapOnItem(9, useBackup);
                            VerifySelectedFocusedIndex(9);
                        }
                        GamepadHelper.PressButton(null, GamepadButton.DPadDown);
                        VerifyRadioButtonsHasFocus(false);

                        TapOnItem(0, useBackup);
                        VerifySelectedFocusedIndex(0);
                        GamepadHelper.PressButton(null, GamepadButton.DPadUp);
                        VerifyRadioButtonsHasFocus(false);

                        TapOnItem(7, useBackup);
                        VerifySelectedFocusedIndex(7);
                        GamepadHelper.PressButton(null, GamepadButton.DPadRight);
                        VerifyRadioButtonsHasFocus(false);

                        TapOnItem(7, useBackup);
                        VerifySelectedFocusedIndex(7);
                        GamepadHelper.PressButton(null, GamepadButton.DPadDown);
                        VerifySelectedIndex(7);
                        VerifyFocusedIndex(8);

                        GamepadHelper.PressButton(null, GamepadButton.DPadLeft);
                        VerifySelectedIndex(7);
                        VerifyFocusedIndex(5);
                    }
                }
            }
        }

        [TestMethod]
        public void ControlKeyKeyboardTest()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test fails on RS2 because of a repeater bug: #1447");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        bool useBackup = type == RadioButtonsSourceType.String;
                        TapOnItem(3, useBackup);
                        VerifySelectedFocusedIndex(3);
                        KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                        KeyboardHelper.PressKey(Key.Down);
                        KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                        VerifySelectedIndex(3);
                        VerifyFocusedIndex(4);
                    }
                }
            }
        }

        [TestMethod]
        public void ColumnsTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        SetNumberOfColumns(1);
                        SetNumberOfItems(10);
                        SetBorderWidthToInf();

                        VerifyLayoutData(10, 1, 0);
                        SetNumberOfColumns(3);
                        VerifyLayoutData(3, 3, 1);
                        SetNumberOfColumns(5);
                        VerifyLayoutData(2, 5, 0);
                        SetNumberOfColumns(7);
                        VerifyLayoutData(1, 7, 3);
                        SetNumberOfColumns(10);
                        VerifyLayoutData(1, 10, 0);
                        SetNumberOfColumns(20);
                        VerifyLayoutData(1, 10, 0);

                        SetNumberOfItems(77);
                        VerifyLayoutData(3, 20, 17);

                        SetBorderWidth(100);
                        VerifyLayoutData(77, 1, 0);
                        SetBorderWidth(200);
                        VerifyLayoutData(77, 1, 0);
                        SetBorderWidth(300);
                        VerifyLayoutData(38, 2, 1);
                        SetBorderWidth(500);
                        VerifyLayoutData(25, 3, 2);
                        SetBorderWidth(550);
                        VerifyLayoutData(19, 4, 1);
                    }
                }
            }
        }

        [TestMethod]
        public void UIAProperties()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetSource(RadioButtonsSourceLocation.ItemSource);

                SelectByIndex(1);
                VerifySelectedIndex(1);
                VerifySelectedPositionInSet(2);
                VerifySelectedSizeOfSet(10);

                SetNumberOfColumns(3);
                VerifySelectedIndex(1);
                VerifySelectedPositionInSet(2);
                VerifySelectedSizeOfSet(10);

                InsertEnabledRadioButton(0);
                VerifySelectedIndex(2);
                VerifySelectedPositionInSet(3);
                VerifySelectedSizeOfSet(11);

                InsertEnabledRadioButton(10);
                VerifySelectedIndex(2);
                VerifySelectedPositionInSet(3);
                VerifySelectedSizeOfSet(12);

                SelectByIndex(10);
                VerifySelectedIndex(10);
                VerifySelectedPositionInSet(11);
                VerifySelectedSizeOfSet(12);

                SetNumberOfItems(17);
                SelectByIndex(16);
                VerifySelectedIndex(16);
                VerifySelectedPositionInSet(17);
                VerifySelectedSizeOfSet(17);
            }
        }

        [TestMethod]
        public void InsertedCheckedRadioButtonGetsSelection()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetSource(RadioButtonsSourceLocation.ItemSource);

                SelectByIndex(3);
                VerifySelectedIndex(3);

                InsertEnabledRadioButton(6, /*isChecked*/true);
                VerifySelectedIndex(6);
            }
        }

        [TestMethod]
        public void ScrollViewerSettingSelectionDoesNotMoveFocus()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test is disabled on RS2 because it requires RS3+ keyboarding behavior.");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        SelectByIndex(3);
                        VerifySelectedIndex(3);
                        VerifyRadioButtonsHasFocus(false);

                        elements.GetReproTextBlock().Click();
                        VerifySelectedIndex(3);
                        // This behavior is probably wrong, it is probably more correct for focus to be on the selected item...
                        VerifyFocusedIndex(0);

                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(1);
                    }
                }
            }
        }
        [TestMethod]
        public void AccessKeys()
        { 
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone3))
            {
                Log.Warning("This test requires RS3+ keyboarding behavior");
                return;
            }
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                foreach (RadioButtonsSourceLocation location in Enum.GetValues(typeof(RadioButtonsSourceLocation)))
                {
                    SetSource(location);
                    foreach (RadioButtonsSourceType type in Enum.GetValues(typeof(RadioButtonsSourceType)))
                    {
                        SetItemType(type);
                        SetNumberOfItems(10);

                        VerifyRadioButtonsHasFocus(false);
                        UseAccessKey();
                        VerifyFocusedIndex(0);
                        VerifySelectedIndex(-1);

                        SelectByIndex(3);
                        VerifySelectedIndex(3);
                        VerifyFocusedIndex(-1);

                        KeyboardHelper.PressKey(Key.Tab);
                        VerifyRadioButtonsHasFocus(false);
                        UseAccessKey();
                        VerifySelectedFocusedIndex(3);
                        UseAccessKey();
                        VerifySelectedFocusedIndex(3);
                    }
                }
            }
        }

        void SetNumberOfColumns(int columns)
        {
            elements.GetMaxColumnsTextBlock().SetValue(columns.ToString());
            elements.GetSetMaxColumnsButton().Click();
        }

        void SetNumberOfItems(int items)
        {
            elements.GetNumberOfItemsTextBlock().SetValue(items.ToString());
            elements.GetSetNumberOfItemsButton().Click();
        }

        void SetBorderWidth(float width)
        {
            elements.GetBorderWidthTextBox().SetValue(width.ToString());
            elements.GetSetBorderWidthButton().Click();
        }

        void SetBorderWidthToInf()
        {
            elements.GetBorderWidthTextBox().SetValue("inf");
            elements.GetSetBorderWidthButton().Click();
        }

        void SetSource(RadioButtonsSourceLocation location)
        {
            switch(location)
            {
                case RadioButtonsSourceLocation.Items:
                    elements.GetSourceComboBox().SelectItemByName("ItemsComboBoxItem");
                    break;
                case RadioButtonsSourceLocation.ItemSource:
                    elements.GetSourceComboBox().SelectItemByName("ItemsSourceComboBoxItem");
                    break;
            }
        }

        void SetItemType(RadioButtonsSourceType type)
        {
            switch(type)
            {
                case RadioButtonsSourceType.String:
                    elements.GetItemTypeComboBox().SelectItemByName("StringsComboBoxItem");
                    break;
                case RadioButtonsSourceType.RadioButton:
                    elements.GetItemTypeComboBox().SelectItemByName("RadioButtonElementsComboBoxItem");
                    break;
            }
        }

        void InsertDisabledRadioButton(int index, bool isChecked = false, string content = "Custom")
        {
            InsertRadioButton(index, true, isChecked, content);
        }

        void InsertEnabledRadioButton(int index, bool isChecked = false, string content = "Custom")
        {
            InsertRadioButton(index, false, isChecked, content);
        }

        void InsertRadioButton(int index, bool disabled, bool isChecked = false, string content = "Custom")
        {
            if (disabled)
            {
                elements.GetCustomDisabledCheckBox().Check();
            }
            else
            {
                elements.GetCustomDisabledCheckBox().Uncheck();
            }

            if(isChecked)
            {
                elements.GetCustomCheckedCheckBox().Check();
            }
            else
            {
                elements.GetCustomCheckedCheckBox().Uncheck();
            }

            elements.GetCustomIndexTextBox().SetValue(index.ToString());
            elements.GetCustomContentTextBox().SetValue(content);
            elements.GetInsertDisplayRadioButtonButton().Click();
        }

        int GetSelectedIndex()
        {
            return Int32.Parse(elements.GetSelectedIndexTextBlock().DocumentText);
        }

        string GetSelectedItem()
        {
            return elements.GetSelectedItemTextBlock().DocumentText;
        }

        int GetFocusedIndex()
        {
            return Int32.Parse(elements.GetFocusedIndexTextBlock().DocumentText);
        }

        string GetFocusedItem()
        {
            return elements.GetFocusedItemTextBlock().DocumentText;
        }

        Tuple<int, int, int> GetLayoutData()
        {
            return new Tuple<int, int, int>(
                Int32.Parse(elements.GetLayoutNumberOfRowsTextBlock().DocumentText),
                Int32.Parse(elements.GetLayoutNumberOfColumnsTextBlock().DocumentText),
                Int32.Parse(elements.GetLayoutNumberOfLargerColumnsTextBlock().DocumentText)
            );
        }

        void VerifyLayoutData(int rows, int columns, int largerColumns)
        {
            var data = GetLayoutData();
            Verify.AreEqual(rows, data.Item1);
            Verify.AreEqual(columns, data.Item2);
            Verify.AreEqual(largerColumns, data.Item3);
        }

        void ClickOnItem(int index)
        {
            Log.Comment("Clicking on item 'Radio Button " + index + "'");
            RadioButton item = FindElement.ByName<RadioButton>("Radio Button " + index);
            item.Click();
        }

        void UseAccessKey()
        {
            KeyboardHelper.PressDownModifierKey(ModifierKey.Alt);
            KeyboardHelper.PressKey(Key.R);
            KeyboardHelper.ReleaseModifierKey(ModifierKey.Alt);
        }

        void TapOnItem(int index, bool useBackup = false)
        {
            if (useBackup)
            {
                BackupTap(index);
            }
            else
            {
                Log.Comment("Clicking on item 'Radio Button " + index + "'");
                RadioButton item = FindElement.ByName<RadioButton>("Radio Button " + index);
                item.Click();
            }
        }

        void BackupTap(int index)
        {
            SelectByIndex(index);
            elements.GetFocusSelectedItemButton().Click();
        }

        void SelectByIndex(int index)
        {
            SetIndexToSelect(index);
            elements.GetSelectByIndexButton().Click();
        }

        void SelectByItem(int index)
        {
            SetIndexToSelect(index);
            elements.GetSelectByItemButton().Click();
        }

        void SetIndexToSelect(int index)
        {
            elements.GetIndexToSelectTextBlock().SetValue(index.ToString());
        }

        void VerifySelectedPositionInSet(int index)
        {
            Verify.AreEqual(index, Int32.Parse(elements.GetSelectedPositionInSetTextBlock().DocumentText));
        }
        void VerifySelectedSizeOfSet(int index)
        {
            Verify.AreEqual(index, Int32.Parse(elements.GetSelectedSizeOfSetTextBlock().DocumentText));
        }

        void VerifySelectedFocusedIndex(int index)
        {
            VerifySelectedIndex(index);
            VerifyFocusedIndex(index);
        }

        void VerifyRadioButtonsHasFocus(bool hasFocus)
        {
            Verify.AreEqual(hasFocus, elements.GetRadioButtonsHasFocusCheckBox().ToggleState == ToggleState.On);
        }

        void VerifySelectedIndex(int index)
        {
            Verify.AreEqual(index, Int32.Parse(elements.GetSelectedIndexTextBlock().DocumentText));
        }

        void VerifyFocusedIndex(int index)
        {
            Verify.AreEqual(index, Int32.Parse(elements.GetFocusedIndexTextBlock().DocumentText));
        }
    }
}