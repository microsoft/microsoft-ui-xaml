// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

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

using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class BreadcrumbTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
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
        public void NoInteractionTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                var breadcrumbItems = breadcrumb.Children;

                Verify.AreEqual(2, breadcrumbItems.Count, "The breadcrumb should contain 2 items: 1 item and an ellipsis");

                var ellipsisItemButton = ConvertTo<Button>(breadcrumbItems[0]);
                Verify.IsNotNull(ellipsisItemButton, "Ellipsis button should not be null");

                var ellipsisItemTextBlock = ConvertTo<TextBlock>(ellipsisItemButton.FirstChild);
                Verify.IsNotNull(ellipsisItemTextBlock, "Ellipsis TextBlock should not be null");

                var ellipsisItemText = ellipsisItemTextBlock.GetText();
                Verify.AreEqual(0xE712, ellipsisItemText[0], "The text in the root button is not '&#xE712;'");

                var rootItemButton = ConvertTo<Button>(breadcrumbItems[1]);
                Verify.IsNotNull(rootItemButton, "Root button should not be null");

                var rootItemTextBlock = ConvertTo<TextBlock>(rootItemButton.FirstChild);
                Verify.IsNotNull(rootItemTextBlock, "Root TextBlock should not be null");

                var breadcrumbItemText = rootItemTextBlock.GetText();
                Verify.AreEqual("Root", breadcrumbItemText, "The text in the root button is not 'Root'");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddItemsToBreadcrumbTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3" });

                var breadcrumbItems = breadcrumb.Children;

                Verify.AreEqual(5, breadcrumbItems.Count, "The breadcrumb should contain 5 items: 4 items and an ellipsis");

                VerifyBreadcrumbItemsContain(breadcrumbItems, new string[] { "Root", "Node A", "Node A_2", "Node A_2_3" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddItemsAndInvokeBreadcrumbItemTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });

                var breadcrumbItems = breadcrumb.Children;
                Verify.AreEqual(6, breadcrumbItems.Count, "The breadcrumb should contain 6 items: 5 items and an ellipsis");

                // Click on the Node A_2 BreadcrumbItem
                var NodeA_2BreadcrumbItem = breadcrumbItems[3];
                NodeA_2BreadcrumbItem.Click();

                Verify.AreEqual(4, breadcrumbItems.Count, "The breadcrumb should contain 4 items: 3 items and an ellipsis");

                VerifyBreadcrumbItemsContain(breadcrumbItems, new string[] { "Root", "Node A", "Node A_2" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddItemsAndCompressBreadcrumbTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });

                var breadcrumbItems = breadcrumb.Children;
                Verify.AreEqual(6, breadcrumbItems.Count, "The breadcrumb should contain 6 items: 5 items and an ellipsis");

                UIObject slider = FindElement.ByName("WidthSlider");
                Verify.IsNotNull(slider, "WidthSlider not found");
                slider.Click(PointerButtons.Primary, slider.BoundingRectangle.Width / 2, slider.BoundingRectangle.Height / 2);

                // Click on the Node A_2 BreadcrumbItem
                var ellipsisItem = ConvertTo<Button>(breadcrumbItems[0]);
                ellipsisItem.InvokeAndWait(TimeSpan.FromSeconds(2));

                Log.Comment("Retrieve the first ellipsis item: Node A_2");
                var ellipsisItem1 = FindElement.ByName<Button>("EllipsisItem1");
                Verify.IsNotNull(ellipsisItem1, "EllipsisItem1 not found");

                VerifyEllipsisItemContainsText(ellipsisItem1, "Node A_2");

                Log.Comment("Retrieve the second ellipsis item: Node A");
                var ellipsisItem2 = FindElement.ByName<Button>("EllipsisItem2");
                Verify.IsNotNull(ellipsisItem2, "EllipsisItem2 not found");

                VerifyEllipsisItemContainsText(ellipsisItem2, "Node A");

                Log.Comment("Retrieve the third ellipsis item: Root");
                var ellipsisItem3 = FindElement.ByName<Button>("EllipsisItem3");
                Verify.IsNotNull(ellipsisItem3, "EllipsisItem3 not found");

                VerifyEllipsisItemContainsText(ellipsisItem3, "Root");

                Log.Comment("Verify only 3 ellipsis items exist");
                var ellipsisItem4 = FindElement.ByName("EllipsisItem4");
                Verify.IsNull(ellipsisItem4, "EllipsisItem4 was found");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void InvokeEllipsisItemTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });

                var breadcrumbItems = breadcrumb.Children;
                Verify.AreEqual(6, breadcrumbItems.Count, "The breadcrumb should contain 6 items: 5 items and an ellipsis");

                UIObject slider = FindElement.ByName("WidthSlider");
                Verify.IsNotNull(slider, "WidthSlider not found");
                slider.Click(PointerButtons.Primary, slider.BoundingRectangle.Width / 2, slider.BoundingRectangle.Height / 2);

                // Click on the Node A_2 BreadcrumbItem
                var ellipsisItem = ConvertTo<Button>(breadcrumbItems[0]);
                ellipsisItem.InvokeAndWait(TimeSpan.FromSeconds(2));

                Log.Comment("Retrieve the second ellipsis item: Node A");
                var ellipsisItem2 = FindElement.ByName<Button>("EllipsisItem2");
                Verify.IsNotNull(ellipsisItem2, "EllipsisItem2 not found");

                VerifyEllipsisItemContainsText(ellipsisItem2, "Node A");

                ellipsisItem2.Invoke();

                VerifyBreadcrumbItemsContain(breadcrumb.Children, new string[] { "Root", "Node A" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void LeftRightKeyboardNavigationTest()
        {
            using (var setup = new TestSetupHelper("Breadcrumb Tests"))
            {
                Log.Comment("Retrieve breadcrumb control as generic UIElement");
                UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
                Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

                ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });               

                var breadcrumbItems = breadcrumb.Children;
                Verify.AreEqual(6, breadcrumbItems.Count, "The breadcrumb should contain 6 items: 5 items and an ellipsis");

                UIObject rtlCheckbox = FindElement.ByName("RightToLeftCheckbox");
                Verify.IsNotNull(rtlCheckbox, "Verifying that we found a UIElement called RightToLeftCheckbox");

                FocusHelper.SetFocus(rtlCheckbox);
                KeyboardHelper.PressKey(Key.Tab);

                Log.Comment("Verify root node is focused");

                UIObject bi1 = FindElement.ByName("BreadcrumbItem1");
                Verify.IsNotNull(bi1, "BreadcrumbItem1 does not exist");

                var i1 = breadcrumbItems[1];
                Verify.IsTrue(i1.HasKeyboardFocus, "'Root' BreadcrumbItem doesn't have focus");

                KeyboardHelper.PressKey(Key.Right);
                KeyboardHelper.PressKey(Key.Tab);

                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Root' BreadcrumbItem doesn't have focus");

            }
        }

        private T ConvertTo<T>(UIObject uiObject)
        {
            if (uiObject != null)
            {
                return (T)Activator.CreateInstance(typeof(T), uiObject);
            }
            else
            {
                return default(T);
            }
        }

        private void ClickOnElements(string[] elementNames)
        {
            foreach (var elementName in elementNames)
            {
                UIObject itemToAddButton = FindElement.ByName(elementName);
                Verify.IsNotNull(itemToAddButton, "Verifying that we found a button with " + elementName + " text");
                itemToAddButton.Click();
            }
        }

        private void VerifyBreadcrumbItemsContain(UICollection<UIObject> breadcrumbItems, string[] expectedItemValues)
        {
            // As recycled breadcrumbs are not deleted, then we compare that the breadcrumb count is always bigger 
            // than the expected values count
            Verify.IsTrue(breadcrumbItems.Count > expectedItemValues.Length, 
                "The expected values count should at least be one less than the BreadcrumbItems count");

            for (int i = 0; i < expectedItemValues.Length; ++i)
            {
                var breadcrumbItemButton = ConvertTo<Button>(breadcrumbItems[i + 1]);
                Verify.IsNotNull(breadcrumbItemButton, "Button should not be null");

                var breadcrumbItemTextBlock = ConvertTo<TextBlock>(breadcrumbItemButton.FirstChild);
                Verify.IsNotNull(breadcrumbItemTextBlock, "TextBlock should not be null");

                var breadcrumbItemText = breadcrumbItemTextBlock.GetText();
                Verify.AreEqual(expectedItemValues[i], breadcrumbItemText,
                        "The text in the button " + breadcrumbItemText + "doesn't match the expected " + expectedItemValues[i]);
            }
        }

        private void VerifyEllipsisItemContainsText(Button ellipsisItem, string expectedEllipsisItemText)
        {
            var ellipsisItemTextBlock = ConvertTo<TextBlock>(ellipsisItem.FirstChild);
            Verify.IsNotNull(ellipsisItemTextBlock, "The ellipsis Item should contain a Textblock as first item");

            Verify.AreEqual(expectedEllipsisItemText, ellipsisItemTextBlock.GetText(), 
                "The ellipsis item doesn't match " + expectedEllipsisItemText);
        }
    }
}
