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

        //[TestMethod] Failing with a crash, todo: investigate.
        public void BasicKeyboardTest()
        {
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
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetSource(RadioButtonsSourceLocation.Items);

                SetNumberOfColumns(3);
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
                //There is a known bug here where pressing down will put focus
                //On the first item because RadioButton handles the event.
                //RadioButtons doesn't catch this because it only catches when the
                //Last item is focused, but that item is disabled here...
                //Bug #....
                //KeyboardHelper.PressKey(Key.Down);
                //VerifySelectedFocusedIndex(9);

                InsertDisabledRadioButton(6);
                InsertDisabledRadioButton(6);

                TapOnItem(1);
                VerifySelectedFocusedIndex(1);
                KeyboardHelper.PressKey(Key.Right);
                VerifySelectedFocusedIndex(5);
                TapOnItem(2);
                VerifySelectedFocusedIndex(2);
                KeyboardHelper.PressKey(Key.Right);
                VerifySelectedFocusedIndex(8);
                KeyboardHelper.PressKey(Key.Up);
                VerifySelectedFocusedIndex(5);
                KeyboardHelper.PressKey(Key.Down);
                VerifySelectedFocusedIndex(8);
                TapOnItem(8);
                VerifySelectedFocusedIndex(10);
                KeyboardHelper.PressKey(Key.Left);
                VerifySelectedFocusedIndex(5);
                TapOnItem(9);
                VerifySelectedFocusedIndex(11);
                KeyboardHelper.PressKey(Key.Left);
                VerifySelectedFocusedIndex(8);
            }
        }

        [TestMethod]
        public void DisabledItemsAtTopOfColumnKeyboardTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetSource(RadioButtonsSourceLocation.ItemSource);

                SetNumberOfColumns(3); 
                InsertDisabledRadioButton(5);
                InsertDisabledRadioButton(5);
                InsertDisabledRadioButton(5);

                TapOnItem(0);
                VerifySelectedFocusedIndex(0);
                KeyboardHelper.PressKey(Key.Right);
                VerifySelectedFocusedIndex(8);
                TapOnItem(1);
                VerifySelectedFocusedIndex(1);
                KeyboardHelper.PressKey(Key.Right);
                VerifySelectedFocusedIndex(8);
                TapOnItem(2);
                VerifySelectedFocusedIndex(2);
                KeyboardHelper.PressKey(Key.Right);
                VerifySelectedFocusedIndex(8);
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
                VerifySelectedFocusedIndex(4);
                TapOnItem(7);
                VerifySelectedFocusedIndex(10);
                KeyboardHelper.PressKey(Key.Left);
                VerifySelectedFocusedIndex(8);
                TapOnItem(8);
                VerifySelectedFocusedIndex(11);
                KeyboardHelper.PressKey(Key.Left);
                VerifySelectedFocusedIndex(8);
            }
        }

        [TestMethod]
        public void ControlKeyKeyboardTest()
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
                        VerifyLayoutData(0, 10, 10);

                        SetNumberOfItems(77);
                        VerifyLayoutData(3, 20, 17);
                    }
                }
            }
        }

        void SetNumberOfColumns(int columns)
        {
            elements.GetMaximumColumnsTextBlock().SetValue(columns.ToString());
            elements.GetSetMaximumColumnsButton().Click();
        }

        void SetNumberOfItems(int items)
        {
            elements.GetNumberOfItemsTextBlock().SetValue(items.ToString());
            elements.GetSetNumberOfItemsButton().Click();
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

        void InsertDisabledRadioButton(int index, string content = "Custom")
        {
            InsertRadioButton(index, true, content);
        }

        void InsertEnabledRadioButton(int index, string content = "Custom")
        {
            InsertRadioButton(index, false, content);
        }

        void InsertRadioButton(int index, bool disabled, string content = "Custom")
        {
            elements.GetCustomIndexTextBox().SetValue(index.ToString());
            if (disabled)
            {
                elements.GetCustomDisabledCheckBox().Check();
            }
            else
            {
                elements.GetCustomDisabledCheckBox().Uncheck();
            }
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
                item.Tap();
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

        void VerifySelectedFocusedIndex(int index)
        {
            VerifySelectedIndex(index);
            VerifyFocusedIndex(index);
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