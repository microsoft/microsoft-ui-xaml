// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.


using System;

using Common;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System.Collections.Generic;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using static Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.RadioButtonsTestPageElements;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
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

        // Scenario: RadioButton-element items (from Items, then ItemsSource); select index 1 via SelectedIndex, then item 3
        //     via SelectedItem.
        // Expected: SelectedIndex reads 1 and "Radio Button 1" is UIA-selected; then SelectedIndex reads 3, "Radio Button 3"
        //     is selected and "Radio Button 1" is no longer selected.
        // Failure means: RadioButtons::Select/UpdateSelectedIndex/UpdateSelectedItem no longer check the new container and
        //     uncheck the old one (GetDataAtIndex), so programmatic selection and the visible checked state diverge.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void SelectionTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: for Items and ItemsSource with string and RadioButton items (skipping Items + strings, issue #1674),
        //     set SelectedItem to item 1 and then to item 3.
        // Expected: the page's SelectedIndex reads 1, then 3.
        // Failure means: RadioButtons::UpdateSelectedItem no longer maps SelectedItem to its index (ItemsSourceView.IndexOf
        //     + Select), so selecting by item doesn't update SelectedIndex.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SelectByItem()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: RadioButtonsFocus page with two 3-item RadioButtons; select index 1 in the first and 2 in the second
        //     (via non-tab-stop buttons), then press Tab into the second group and Shift+Tab back to the first.
        // Expected: selections read 1 and 2 and the first group has focus; after Tab the first group loses focus and the
        //     second has focus on its selected item 2; after Shift+Tab the first group's selected item 1 has focus.
        // Failure means: RadioButtons::OnGettingFocus no longer redirects keyboard focus arriving from outside the repeater
        //     to the selected item (focus lands on another item), or the RadioButtonsFocusPage setup broke again.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void FocusComingFromAnotherRepeaterTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtonsFocus Test" }))
            {
                var elements = new RadioButtonsFocusTestPageElements();        

                // Select items
                SelectByIndex(1, false);
                SelectByIndex(2, true);

                // Verify selection
                VerifySelectedIndex(1, false);
                VerifySelectedIndex(2, true);

                VerifyRadioButtonsHasFocus(true, false);

                // Move focus to second RadioButtons control by pressing TAB
                KeyboardHelper.PressKey(Key.Tab);

                // Verify that 
                // - the first RadioButtons control no longer has focus and
                // - the third radio button of the second RadioButtons control now has focus
                VerifyRadioButtonsHasFocus(false, false);
                VerifySelectedFocusedIndex(2, true);

                // Move focus to the first RadioButtons control by pressing TAB again
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);

                // Verify that the second radio button is still selected and now has focus
                VerifySelectedFocusedIndex(1, false);

                void SelectByIndex(int index, bool checkRadioButtons2)
                {
                    if (checkRadioButtons2)
                    {
                        elements.GetIndexToSelectTextBox2().SetValue(index.ToString());
                        elements.GetSelectByIndexButton2().Click();
                    }
                    else
                    {
                        elements.GetIndexToSelectTextBox1().SetValue(index.ToString());
                        elements.GetSelectByIndexButton1().Click();
                    }
                }

                void VerifySelectedIndex(int index, bool checkRadioButtons2)
                {
                    if (checkRadioButtons2)
                    {
                        Verify.AreEqual(index, Int32.Parse(elements.GetSelectedIndexTextBlock2().DocumentText), $"RadioButton with index {index} should have been selected.");
                    }
                    else
                    {
                        Verify.AreEqual(index, Int32.Parse(elements.GetSelectedIndexTextBlock1().DocumentText), $"RadioButton with index {index} should have been selected.");
                    }
                }

                void VerifyRadioButtonsHasFocus(bool hasFocus, bool checkRadioButtons2)
                {
                    if (checkRadioButtons2)
                    {
                        Verify.AreEqual(hasFocus, elements.GetRadioButtons2HasFocusCheckBox().ToggleState == ToggleState.On);
                    }
                    else
                    {
                        Verify.AreEqual(hasFocus, elements.GetRadioButtons1HasFocusCheckBox().ToggleState == ToggleState.On);
                    }
                }

                void VerifyFocusedIndex(int index, bool checkRadioButtons2)
                {
                    if (checkRadioButtons2)
                    {
                        Verify.AreEqual(index, Int32.Parse(elements.GetFocusedIndexTextBlock2().DocumentText), $"RadioButton with index {index} should have focus.");
                    }
                    else
                    {
                        Verify.AreEqual(index, Int32.Parse(elements.GetFocusedIndexTextBlock1().DocumentText), $"RadioButton with index {index} should have focus.");
                    }
                    
                }

                void VerifySelectedFocusedIndex(int index, bool checkRadioButtons2)
                {
                    VerifySelectedIndex(index, checkRadioButtons2);
                    VerifyFocusedIndex(index, checkRadioButtons2);
                }
            }
        }

        // Scenario: default single column of 10 items, for Items/ItemsSource x string/RadioButton items; focus item 3 and
        //     press Down/Up/Left/Right, focus item 0 and press Up then Down, focus item 9 and press Down.
        // Expected: selection and focus move together: 3->4->3, Left/Right stay on 3; Up on 0 stays on 0 and Down from 0
        //     goes to 1; Down on 9 stays on 9.
        // Failure means: RadioButtons::OnChildPreviewKeyDown/MoveFocusNext/MoveFocusPrevious stop moving focus within the
        //     repeater, HandleEdgeCaseFocus lets focus escape at the ends, or OnGettingFocus no longer selects on focus.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void BasicKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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
                        // Down from the first item (index 0) moves to the second item.
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(1);

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(9);
                    }
                }
            }
        }

        // Scenario: 10 items with MaxColumns=3 (columns 0-3, 4-6, 7-9), for Items/ItemsSource x string/RadioButton items;
        //     from items 3, 0 and 9 press sequences of Down/Up/Left/Right.
        // Expected: Up/Down move by index across column boundaries (3->4); Right/Left move to the nearest item in the
        //     adjacent column (3->6->9, 6->2, 0->4, 9->6) and stop at the outer edges, with selection following focus.
        // Failure means: RadioButtons::OnChildPreviewKeyDown's XY navigation (TryMoveFocus scoped to the RadioButtons) or
        //     index navigation regressed, or HandleEdgeCaseFocus no longer keeps focus inside at the edges.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void MultiColumnKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: 3 items with MaxColumns=3 (one row), for Items/ItemsSource x string/RadioButton items; from item 0
        //     press Right, Right, Left, Left, then Down, Down, Up, Up.
        // Expected: selection and focus move 0->1->2->1->0 with Right/Left, and the same with Down/Up.
        // Failure means: Left/Right (XY focus) or Up/Down (MoveFocusNext/MoveFocusPrevious) in
        //     RadioButtons::OnChildPreviewKeyDown no longer move between items laid out in a single row.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // Task 26103056: unreliable; still fails when run (no reason streamed by TAEF).
        public void SingleRowKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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
                        SetNumberOfItems(3);
                        TapOnItem(0, useBackup);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(1);
                        KeyboardHelper.PressKey(Key.Right);
                        VerifySelectedFocusedIndex(2);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(1);
                        KeyboardHelper.PressKey(Key.Left);
                        VerifySelectedFocusedIndex(0);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(1);
                        KeyboardHelper.PressKey(Key.Down);
                        VerifySelectedFocusedIndex(2);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(1);
                        KeyboardHelper.PressKey(Key.Up);
                        VerifySelectedFocusedIndex(0);
                    }
                }
            }
        }

        // Scenario: 10 RadioButton items, MaxColumns=3, for Items/ItemsSource: insert a disabled RadioButton at index 10,
        //     later two more at index 6; tap items by name and press arrows (indices below are positions after inserts).
        // Expected: navigation skips disabled items, e.g. 7 Right->9, 9 Left->5, 6 Right->9; after the second insert
        //     1 Right->10, 2 Right->11, 5 Down->8 (past disabled 6 and 7), 10 Left->1, 11 Left->2.
        // Failure means: RadioButtons::MoveFocus no longer skips items that can't take focus, or XY Left/Right navigation
        //     lands on or stops at disabled items, so keyboard users get stuck.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void DisabledItemsKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: 10 RadioButton items, MaxColumns=3, three disabled RadioButtons inserted at index 5 so they fill the
        //     top of the middle column (columns 0-4, 5-8, 9-12); tap items and press Right or Left.
        // Expected: Right from rows blocked by a disabled item jumps to the third column (0->9, 1->10, 2->11), 3 and 4 go
        //     to 8; Left from 9/10/11 returns to 0/1/2 and from 12 goes to 8; selection follows focus.
        // Failure means: XY Left/Right navigation in RadioButtons::OnChildPreviewKeyDown no longer skips disabled items at
        //     the top of a column, leaving keyboard focus stuck or on a disabled RadioButton.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void DisabledItemsAtTopOfColumnKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: 10 items, MaxColumns=3, for Items/ItemsSource x string/RadioButton items; from items 9, 0 and 7 press
        //     DPadDown/DPadUp/DPadRight at the group's edges, then DPadDown and DPadLeft starting from item 7.
        // Expected: at the edges the gamepad moves focus out of the RadioButtons; inside, focus moves 7->8->5 while the
        //     selection stays on 7.
        // Failure means: OnChildPreviewKeyDown's gamepad paths (TryMoveFocus searching the whole XamlRoot) trap focus, or
        //     OnGettingFocus selects on non-keyboard focus, so gamepad users can't leave or select just by moving focus.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // Disabled due to #4576: Unreliable test
        public void GamepadCanEscapeAndDoesNotSelectWithFocus()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

                        TapOnItem(9, useBackup);
                        VerifySelectedFocusedIndex(9);
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

        // Scenario: for Items/ItemsSource x string/RadioButton items, focus item 3 and press Ctrl+Down.
        // Expected: focus moves to item 4 while the selection stays on item 3.
        // Failure means: RadioButtons::OnGettingFocus ignores the Control key state and selects on every focus move, so
        //     Ctrl+arrow can no longer move focus without changing the selection.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void ControlKeyKeyboardTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: 10 items with unconstrained width and MaxColumns 1, 3, 5, 7, 10, 20; then 77 items; then the host
        //     Border width set to 100, 200, 300, 500, 550 (for Items/ItemsSource x string/RadioButton items).
        // Expected: the test hooks report (rows, columns, larger columns) of (10,1,0), (3,3,1), (2,5,0), (1,7,3), (1,10,0),
        //     (1,10,0), (3,20,17), then (77,1,0), (77,1,0), (38,2,1), (25,3,2), (19,4,1).
        // Failure means: ColumnMajorUniformToLargestGridLayout::CalculateColumns/ArrangeOverride compute the wrong column
        //     count or distribution for MaxColumns, item count or available width (or the test hooks stopped reporting).
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Ignore", "True")] // Task 26103056: unreliable; still fails when run (no reason streamed by TAEF).
        public void ColumnsTest()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: RadioButton items via ItemsSource; find the "More Radio Buttons" group; select 1, set MaxColumns=3,
        //     insert enabled RadioButtons at 0 and 10, select 10, then reset to 17 items and select 16.
        // Expected: the group (found by its Header name) is ControlType Group; the selected index/PositionInSet/SizeOfSet
        //     are 1/2/10, unchanged by MaxColumns, then 2/3/11, 2/3/12, 10/11/12 and 16/17/17.
        // Failure means: RadioButtonsAutomationPeer naming/control type, or the RadioButtons ElementPrepared/IndexChanged/
        //     CollectionChanged handlers no longer keep selection and PositionInSet/SizeOfSet current as items are inserted.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void UIAProperties()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
            {
                elements = new RadioButtonsTestPageElements();
                SetItemType(RadioButtonsSourceType.RadioButton);
                SetSource(RadioButtonsSourceLocation.ItemSource);

                Log.Comment("Verify the header is set as the name for the RadioButtons control.");
                UIObject groupElement = TryFindElement.ByName("More Radio Buttons");
                Verify.IsTrue(groupElement.ControlType == ControlType.Group);

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

        // Scenario: RadioButton items via ItemsSource with item 3 selected; insert an enabled, already-checked RadioButton
        //     at index 6.
        // Expected: SelectedIndex moves from 3 to 6 (the inserted checked item).
        // Failure means: RadioButtons::OnRepeaterElementPrepared no longer selects a pre-checked RadioButton when it is
        //     realized, so the control's selection and the checked item disagree.
        [TestMethod]
        [TestProperty("TestSuite", "C")]
        public void InsertedCheckedRadioButtonGetsSelection()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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

        // Scenario: for Items/ItemsSource x string/RadioButton items, select item 3 programmatically (group unfocused), click
        //     ReproTextBlock so the ScrollViewer places focus on the first radio button, then press Down.
        // Expected: the click leaves the selection on 3 with focus on item 0; Down then selects and focuses item 1.
        // Failure means: RadioButtons::OnGettingFocus selects on non-keyboard (programmatic) focus, changing the selection
        //     when a ScrollViewer moves focus, or keyboard Down from that item no longer selects it.
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Ignore", "True")] // Task 26103056: intermittent; after clicking ReproTextBlock focus sometimes isn't on item 0 (VerifyFocusedIndex AreEqual(0, -1)).
        public void ScrollViewerSettingSelectionDoesNotMoveFocus()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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
        // Scenario: 10 items, AccessKey "r", for Items/ItemsSource x string/RadioButton items; press Alt+R with no
        //     selection, then select item 3, press Tab and press Alt+R twice.
        // Expected: with no selection Alt+R focuses item 0 without selecting it; after selecting 3 the group is unfocused,
        //     and each Alt+R puts focus on the selected item 3.
        // Failure means: RadioButtons::OnAccessKeyInvoked no longer focuses the selected item (or the first item when
        //     nothing is selected), so the access key doesn't take users to the current choice.
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        [TestProperty("Ignore", "True")] // Task 26103056: unreliable; fails in VerifyFocusedIndex with AreEqual(3, -1) after Alt+R with a selection.
        public void AccessKeys()
        {
            using (var setup = new TestSetupHelper(new[] { "RadioButtons Tests", "RadioButtons Test" }))
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
