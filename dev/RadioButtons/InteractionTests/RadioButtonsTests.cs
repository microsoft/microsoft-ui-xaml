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

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
using MS.Internal.Mita.Foundation.Patterns;
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RadioButtonsTests
    {
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

        public List<string> m_colors = new List<string>
        {
            "Red",
            "Orange",
            "Yellow",
            "Green",
            "Blue",
            "Indigo",
            "Violet"
        };

        [TestMethod]
        public void SelectionTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                Log.Comment("Testing initial selection");
                VerifyColorSelected("Orange");

                Select("Red");

                VerifyColorSelected("Red");
                
                Log.Comment("Select item by changing SelectedItem");
                Button selectItemBlue = FindElement.ByName<Button>("SelectItemBlue");
                selectItemBlue.Click();
                Wait.ForIdle();

                VerifyColorSelected("Blue");

                Log.Comment("Select item by changing SelectedIndex");
                Button selectIndex5 = FindElement.ByName<Button>("SelectIndex5");
                selectIndex5.Click();
                Wait.ForIdle();

                VerifyColorSelected(5);
            }
        }

        [TestMethod]
        public void KeyboardTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                Select("Red");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                VerifyColorSelected("Orange");

                KeyboardHelper.PressKey(Key.Down);
                Wait.ForIdle();
                VerifyColorSelected("Green");

                KeyboardHelper.PressKey(Key.Up);
                Wait.ForIdle();
                VerifyColorSelected("Orange");
                
                KeyboardHelper.PressDownModifierKey(ModifierKey.Control);
                KeyboardHelper.PressKey(Key.Up);
                KeyboardHelper.ReleaseModifierKey(ModifierKey.Control);
                Wait.ForIdle();

                VerifyColorSelected("Orange");

                KeyboardHelper.PressKey(Key.Space);
                Wait.ForIdle();

                VerifyColorSelected("Red");
            }
        }

        [TestMethod]
        public void ColumnsTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                ListViewItem item0 = FindElement.ByName<ListViewItem>("Red");
                ListViewItem item1 = FindElement.ByName<ListViewItem>("Orange");
                
                Log.Comment("Verify Orange is to the right of Red");
                Verify.AreEqual(item1.BoundingRectangle.Top, item0.BoundingRectangle.Top);
                Verify.IsGreaterThanOrEqual(item1.BoundingRectangle.Left, item0.BoundingRectangle.Right);

                Log.Comment("Changing to a single column");
                Button oneColumn = FindElement.ByName<Button>("OneColumn");
                oneColumn.Click();
                Wait.ForIdle();

                Log.Comment("Verify Orange is under Red");
                Verify.IsGreaterThanOrEqual(item1.BoundingRectangle.Top, item0.BoundingRectangle.Bottom);
                Verify.AreEqual(item1.BoundingRectangle.Left, item0.BoundingRectangle.Left);
            }
        }

        [TestMethod]
        public void TabIntoTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                Log.Comment("The group should start with no selection");
                ListViewItem item = FindElement.ByName<ListViewItem>("Item 1");
                Verify.IsFalse(item.IsSelected);

                Log.Comment("Move focus over the unselected radio group");
                KeyboardHelper.PressKey(Key.Tab, numPresses: 7);

                Log.Comment("Now the first item should be selected");
                Verify.IsTrue(item.IsSelected);
            }
        }

        public void ItemsSourceTest()
        {
            using (var setup = new TestSetupHelper("RadioButtons Tests"))
            {
                Log.Comment("Verify ItemsSource items exist");
                ListViewItem item = FindElement.ByName<ListViewItem>("Middle");
                Verify.IsNotNull(item);
            }
        }

        public void Select(string itemString)
        {
            Log.Comment("Clicking on item '" + itemString + "'");
            ListViewItem item = FindElement.ByName<ListViewItem>(itemString);
            item.Click();
            Wait.ForIdle();
        }

        public void VerifyColorSelected(string color)
        {
            VerifyColorSelected(m_colors.IndexOf(color));
        }

        public void VerifyColorSelected(int colorIndex)
        {
            TextBlock selectedChangedTextBlock = FindElement.ByName<TextBlock>("SelectedChangedTextBlock");
            TextBlock selectedIndexTextBlock = FindElement.ByName<TextBlock>("SelectedIndexTextBlock");
            TextBlock selectedItemTextBlock = FindElement.ByName<TextBlock>("SelectedItemTextBlock");

            Verify.AreEqual(m_colors[colorIndex],  selectedChangedTextBlock.DocumentText);
            Verify.AreEqual(colorIndex.ToString(), selectedIndexTextBlock.DocumentText);
            Verify.AreEqual(m_colors[colorIndex],  selectedItemTextBlock.DocumentText);
        }
    }
}