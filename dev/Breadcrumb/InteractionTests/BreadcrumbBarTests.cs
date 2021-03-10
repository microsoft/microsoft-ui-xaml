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
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Collections.ObjectModel;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{

    public class BreadcrumbBarItem : UIObject, IInvoke
    {
        public BreadcrumbBarItem(UIObject uiObject)
            : base(uiObject)
        {
            this.Initialize();
        }

        private void Initialize()
        {
            _invokePattern = new InvokeImplementation(this);
        }

        public void Invoke()
        {
            _invokePattern.Invoke();
        }

        public UIEventWaiter GetInvokedWaiter()
        {
            return _invokePattern.GetInvokedWaiter();
        }

        public void InvokeAndWait(TimeSpan? timeout = null)
        {
            using (var waiter = GetInvokedWaiter())
            {
                Invoke();
                if (timeout == null)
                {
                    waiter.Wait();
                }
                else
                {
                    waiter.Wait(timeout.Value);
                }
            }

            Wait.ForIdle();
        }

        new public static IFactory<BreadcrumbBarItem> Factory
        {
            get
            {
                if (null == BreadcrumbBarItem._factory)
                {
                    BreadcrumbBarItem._factory = new BreadcrumbBarItemFactory();
                }
                return BreadcrumbBarItem._factory;
            }
        }

        private IInvoke _invokePattern;
        private static IFactory<BreadcrumbBarItem> _factory = null;

        private class BreadcrumbBarItemFactory : IFactory<BreadcrumbBarItem>
        {
            public BreadcrumbBarItem Create(UIObject element)
            {
                return new BreadcrumbBarItem(element);
            }
        }
    }

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
            // This is a sanity test that verifies that the BreadcrumbBar control exists and the basic setup for a test is correct
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                UIObject breadcrumb = RetrieveBreadcrumbControl();
                var breadcrumbItems = breadcrumb.Children;

                Verify.AreEqual(2, breadcrumbItems.Count, "The breadcrumb should contain 2 items: 1 item and an ellipsis");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddItemsAndCompressBreadcrumbTest()
        {
            // In this test we add the nodes 'Node A', 'Node A_2', 'Node A_2_3', once the nodes have been added
            // we click on each item verify that the BreadcrumbBar contained the specified nodes. At the end, only the
            // ellipsis item and the 'Root' item should exist
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpTest();
                
                VerifyBreadcrumbBarItemsContain(breadcrumb.Children, new string[] { "Root", "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });
                
                Verify.AreEqual(2, breadcrumb.Children.Count, "The breadcrumb should contain 2 items: the root and an ellipsis");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void AddItemsAndInvokeBreadcrumbBarItemTest()
        {
            // In this test we add the breadcrumb Items 'Node A', 'Node A_2', 'Node A_2_3' & 'Node A_2_3_1'
            // Once those items have been added, we click on the 'Node A_2' item and verify that only the
            // ellipsis item, 'Root', 'Node A' and 'Node A_2' exist on the breadcrumb
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpTest();

                // Click on the Node A_2 BreadcrumbBarItem
                var NodeA_2BreadcrumbBarItem = breadcrumb.Children[3];
                NodeA_2BreadcrumbBarItem.Click();

                VerifyLastClickedItemIndexIs(2);
                VerifyLastClickedItemIs("Node A_2");

                Verify.AreEqual(4, breadcrumb.Children.Count, "The breadcrumb should contain 4 items: 3 items and an ellipsis");

                VerifyBreadcrumbBarItemsContain(breadcrumb.Children, new string[] { "Root", "Node A", "Node A_2" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void InvokeEllipsisItemTest()
        {
            // In this test we add some items to the BreadcrumbBar, once all items have been added
            // the slider is clicked so nodes 'Node A_2', 'Node A' and 'Root' are crumbled.
            // Finally, we invoke the Ellipsis item to verify the crumbled nodes exist inside the flyout
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();

                // Click on the Ellipsis BreadcrumbBarItem
                InvokeEllipsisItem(breadcrumb);

                VerifyDropDownItemContainsText("EllipsisItem1", "Node A_2");
                VerifyDropDownItemContainsText("EllipsisItem2", "Node A");
                VerifyDropDownItemContainsText("EllipsisItem3", "Root");

                Log.Comment("Verify only 3 ellipsis items exist");
                var ellipsisItem4 = FindElement.ByName("EllipsisItem4");
                Verify.IsNull(ellipsisItem4, "EllipsisItem4 was found");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void InvokeDropDownItemTest()
        {
            // All the steps performed in InvokeEllipsisItemTest are done, once that happens,
            // we invoke the 'Node A' item and we verify that only 'Root' and 'Node A' exist in the list
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();

                // Click on the Ellipsis BreadcrumbBarItem
                InvokeEllipsisItem(breadcrumb);

                var ellipsisItemNodeA = VerifyDropDownItemContainsText("EllipsisItem2", "Node A");
                ellipsisItemNodeA.Invoke();
                Thread.Sleep(500);

                VerifyBreadcrumbBarItemsContain(breadcrumb.Children, new string[] { "Root", "Node A" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationLeftToRightTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox();

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, false);

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should keep the focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[1].HasKeyboardFocus, "'Root' BreadcrumbBarItem should have focus");

                // Bug to solve here
                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[1].HasKeyboardFocus, "'Root' BreadcrumbBarItem should keep the focus");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationRightToLeftTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox(true);

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, false, true);

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should keep the focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[1].HasKeyboardFocus, "'Root' BreadcrumbBarItem should have focus");

                // Bug to solve here
                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[1].HasKeyboardFocus, "'Root' BreadcrumbBarItem should keep the focus");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationForCrumbledLeftToRightTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox();

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, true);

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus back");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[0].HasKeyboardFocus, "Ellipsis BreadcrumbBarItem should have focus back");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationForCrumbledRightToLeftTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox(true);

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, true);

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Left);
                Verify.IsTrue(breadcrumbItems[5].HasKeyboardFocus, "'Node A_2_3_1' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[4].HasKeyboardFocus, "'Node A_2_3' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[0].HasKeyboardFocus, "Ellipsis BreadcrumbBarItem should have focus");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationFlyoutItemsTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox();

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, true);

                KeyboardHelper.PressKey(Key.Enter);
                Thread.Sleep(1000);

                // Here we should verify that the first element in the flyout has focus and we can move up/down

                var dropDownItem = GetDropDownItemByName("EllipsisItem1");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem1 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Down);

                dropDownItem = GetDropDownItemByName("EllipsisItem2");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem2 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Down);

                dropDownItem = GetDropDownItemByName("EllipsisItem3");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem3 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Up);

                dropDownItem = GetDropDownItemByName("EllipsisItem2");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem2 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Up);

                dropDownItem = GetDropDownItemByName("EllipsisItem1");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem1 BreadcrumbBarItem should have focus");
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationItemInvokationTest()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox();

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, false);

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Right);
                Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Enter);
                
                VerifyLastClickedItemIndexIs(2);
                VerifyLastClickedItemIs("Node A_2");

                // Possible bug to solve here
                // Verify.IsTrue(breadcrumbItems[3].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should keep focus");

                Verify.AreEqual(4, breadcrumb.Children.Count, "The breadcrumb should contain 4 items: 3 items and an ellipsis");

                VerifyBreadcrumbBarItemsContain(breadcrumb.Children, new string[] { "Root", "Node A", "Node A_2" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void KeyboardNavigationDropDownItemInvokationTest()
        {
            using(var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                var breadcrumb = SetUpCrumbledTest();
                var breadcrumbItems = breadcrumb.Children;

                var rtlCheckbox = RetrieveRTLCheckBox();

                SetFocusToFirstBreadcrumbBarItem(breadcrumb, true);

                KeyboardHelper.PressKey(Key.Enter);
                Thread.Sleep(1000);

                // Here we should verify that the first element in the flyout has focus and we can move up/down
                var dropDownItem = GetDropDownItemByName("EllipsisItem1");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem1 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Down);

                dropDownItem = GetDropDownItemByName("EllipsisItem2");
                Verify.IsTrue(dropDownItem.HasKeyboardFocus, "EllipsisItem2 BreadcrumbBarItem should have focus");

                KeyboardHelper.PressKey(Key.Enter);

                VerifyLastClickedItemIndexIs(1);
                VerifyLastClickedItemIs("Node A");

                // Possible bug to solve here
                // Verify.IsTrue(breadcrumbItems[2].HasKeyboardFocus, "'Node A_2' BreadcrumbBarItem should keep focus");

                Verify.AreEqual(3, breadcrumb.Children.Count, "The breadcrumb should contain 3 items: 2 items and an ellipsis");

                VerifyBreadcrumbBarItemsContain(breadcrumb.Children, new string[] { "Root", "Node A" });
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void VerifyMulticlickCrash()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                UIObject breadcrumb = RetrieveBreadcrumbControl();
                ClickOnElements(new string[] { "Node A", "Node A_1" });

                UIObject slider = RetrieveWidthSlider();
                slider.Click(PointerButtons.Primary, 1, slider.BoundingRectangle.Height / 2);

                InvokeEllipsisItem(breadcrumb);

                var ellipsisItemNodeA = VerifyDropDownItemContainsText("EllipsisItem2", "Root");
                ellipsisItemNodeA.Invoke();
                Thread.Sleep(500);

                for (int i = 0; i < 5; ++i)
                {
                    breadcrumb.Children[1].Click();

                    VerifyLastClickedItemIndexIs(0);
                    VerifyLastClickedItemIs("Root");
                }
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void VerifyFlyoutRecycleCrash()
        {
            using (var setup = new TestSetupHelper("BreadcrumbBar Tests"))
            {
                UIObject breadcrumb = SetUpCrumbledTest();

                UIObject slider = RetrieveWidthSlider();
                slider.Click(PointerButtons.Primary, 1, slider.BoundingRectangle.Height / 2);

                InvokeEllipsisItem(breadcrumb);

                var ellipsisItemNodeA_2_3 = VerifyDropDownItemContainsText("EllipsisItem1", "Node A_2_3");
                ellipsisItemNodeA_2_3.Invoke();
                Thread.Sleep(500);

                InvokeEllipsisItem(breadcrumb);
            }
        }

        private UIObject SetUpTest()
        {
            UIObject breadcrumb = RetrieveBreadcrumbControl();
            ClickOnElements(new string[] { "Node A", "Node A_2", "Node A_2_3", "Node A_2_3_1" });

            var breadcrumbItems = breadcrumb.Children;
            Verify.AreEqual(6, breadcrumbItems.Count, "The breadcrumb should contain 6 items: 5 items and an ellipsis");

            return breadcrumb;
        }

        private UIObject SetUpCrumbledTest()
        {
            var breadcrumb = SetUpTest();

            UIObject slider = RetrieveWidthSlider();
            slider.Click(PointerButtons.Primary, (slider.BoundingRectangle.Width / 3) - 10, slider.BoundingRectangle.Height / 2);

            return breadcrumb;
        }

        private void InvokeEllipsisItem(UIObject breadcrumb)
        {
            var ellipsisItem = ConvertTo<BreadcrumbBarItem>(breadcrumb.Children[0]);
            ellipsisItem.Invoke();

            Thread.Sleep(1000);
        }

        private bool TryGetFocusForRs2(UIObject breadcrumb, int indexToFocus, bool isEllipsisVisible)
        {
            Log.Comment("Try Set focus for RS2 build");

            UIObject anchor = RetrieveRTLCheckBox();
            FocusHelper.SetFocus(anchor);

            // For RS2 we need two Tab if the ellipsis is onscreen and 3 if it's not
            KeyboardHelper.PressKey(Key.Tab);
            KeyboardHelper.PressKey(Key.Tab);

            if (!isEllipsisVisible)
            {
                KeyboardHelper.PressKey(Key.Tab);
            }

            bool elementGotFocus = breadcrumb.Children[indexToFocus].HasKeyboardFocus;

            if (elementGotFocus)
            {
                Log.Comment("Got focus with RS2 way");
            }
            else
            {
                Log.Comment("Didn't got focus with RS2 way");
            }

            return elementGotFocus;
        }

        private bool TryGetFocusForRs3(UIObject breadcrumb, int indexToFocus, bool isEllipsisVisible, bool isRightToLeft)
        {
            Log.Comment("Try Set focus for RS3 build");

            UIObject anchor = RetrieveRTLCheckBox();
            FocusHelper.SetFocus(anchor);

            // For RS2 we need two Tab if the ellipsis is onscreen and 3 if it's not
            KeyboardHelper.PressKey(Key.Tab);
            KeyboardHelper.PressKey(Key.Tab);

            if (!isEllipsisVisible)
            {
                if (isRightToLeft)
                {
                    KeyboardHelper.PressKey(Key.Left);
                }
                else
                {
                    KeyboardHelper.PressKey(Key.Right);
                }
            }

            bool elementGotFocus = breadcrumb.Children[indexToFocus].HasKeyboardFocus;

            if (elementGotFocus)
            {
                Log.Comment("Got focus with RS3 way");
            }
            else
            {
                Log.Comment("Didn't got focus with RS3 way");
            }

            return elementGotFocus;
        }

        private bool TryGetFocusByPressingTab(UIObject breadcrumb, int indexToFocus)
        {
            Log.Comment("Try Set focus by just pressing tab a bunch of times");

            UIObject anchor = RetrieveRTLCheckBox();
            FocusHelper.SetFocus(anchor);

            int tabCount = 0;
            while ((!breadcrumb.Children[indexToFocus].HasKeyboardFocus) && tabCount < 10)
            {
                KeyboardHelper.PressKey(Key.Tab);
                ++tabCount;
            }

            Log.Comment(tabCount + " times was pressed the <TAB> key before the control gained focus");

            bool elementGotFocus = breadcrumb.Children[indexToFocus].HasKeyboardFocus;

            if (elementGotFocus)
            {
                Log.Comment("Got focus pressing <TAB> key");
            }
            else
            {
                Log.Comment("Didn't got focus pressing <TAB> key");
            }

            return elementGotFocus;
        }

        private void SetFocusToFirstBreadcrumbBarItem(UIObject breadcrumb, bool isEllipsisVisible = false, bool isRightToLeft = false)
        {
            UIObject anchor = RetrieveRTLCheckBox();
            FocusHelper.SetFocus(anchor);

            int indexToFocus = isEllipsisVisible ? 0 : 1;

            // The RS3 and RS2 behaviours seem a little odd on how many tabs need to be pressed 
            bool gotFocus = TryGetFocusForRs2(breadcrumb, indexToFocus, isEllipsisVisible);
            if (!gotFocus)
            {
                gotFocus = TryGetFocusForRs3(breadcrumb, indexToFocus, isEllipsisVisible, isRightToLeft);
                if (!gotFocus)
                {
                    gotFocus = TryGetFocusByPressingTab(breadcrumb, indexToFocus);
                }
            }

            if (isEllipsisVisible)
            {
                Verify.IsTrue(gotFocus, "Ellipsis BreadcrumbBarItem should have focus");
            }
            else
            {
                Verify.IsTrue(gotFocus, "'Root' BreadcrumbBarItem should have focus");
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

        private UIObject RetrieveBreadcrumbControl()
        {
            Log.Comment("Retrieve breadcrumb control as generic UIElement");
            UIObject breadcrumb = FindElement.ByName("BreadcrumbControl");
            Verify.IsNotNull(breadcrumb, "Verifying that we found a UIElement called BreadcrumbControl");

            return breadcrumb;
        }

        private UIObject RetrieveWidthSlider()
        {
            UIObject slider = FindElement.ByName("WidthSlider");
            Verify.IsNotNull(slider, "WidthSlider not found");

            return slider;
        }

        private UIObject RetrieveRTLCheckBox(bool mustClickCheckBox = false)
        {
            var rtlCheckbox = FindElement.ByName<CheckBox>("RightToLeftCheckbox");
            Verify.IsNotNull(rtlCheckbox, "Verifying that we found a UIElement called RightToLeftCheckbox");

            if (mustClickCheckBox)
            {
                rtlCheckbox.Click();
            }

            return rtlCheckbox;
        }

        private void VerifyBreadcrumbBarItemsContain(UICollection<UIObject> breadcrumbItems, string[] expectedItemValues)
        {
            // WARNING: this method clicks on each breadcrumb so once the verification has finished, 
            // only the ellipsis item and 'Root' should exist

            // As recycled breadcrumbs are not deleted, then we compare that the breadcrumb count is always bigger 
            // than the expected values count
            Verify.IsTrue(breadcrumbItems.Count > expectedItemValues.Length, 
                "The expected values count should at least be one less than the BreadcrumbBarItems count");

            // To verify the existence of the expected nodes we click on each of them and verify agains the strings
            // in LastClickedItemIndex and LastClickedItem textboxes. 
            for (int i = expectedItemValues.Length - 1; i >= 0; --i)
            {
                var currentItem = breadcrumbItems[i + 1];
                Verify.IsNotNull(currentItem, "Current BreadcrumbBarItem should not be null");

                var currentBreadcrumbBarItem = ConvertTo<BreadcrumbBarItem>(currentItem);
                Verify.IsNotNull(currentBreadcrumbBarItem, "UIElement should be a BreadcrumbBarItem");

                currentBreadcrumbBarItem.Click();

                VerifyLastClickedItemIndexIs(i);
                VerifyLastClickedItemIs(expectedItemValues[i]);
            }
        }

        private Button VerifyDropDownItemContainsText(string dropDownItemName, string expectedText)
        {
            Log.Comment("Retrieve the ellipsis item: " + expectedText);
            var dropDownItem = GetDropDownItemByName(dropDownItemName);

            VerifyDropDownItemContainsText(dropDownItem, expectedText);

            return dropDownItem;
        }

        private Button GetDropDownItemByName(string dropDownItemName)
        {
            var dropDownItem = FindElement.ByName<Button>(dropDownItemName);
            Verify.IsNotNull(dropDownItem, dropDownItemName + " not found");
            return dropDownItem;
        }

        private void VerifyDropDownItemContainsText(Button dropDownItem, string expectedEllipsisItemText)
        {
            var ellipsisItemTextBlock = ConvertTo<TextBlock>(dropDownItem.FirstChild);
            Verify.IsNotNull(ellipsisItemTextBlock, "The ellipsis Item should contain a Textblock as first item");

            Verify.AreEqual(expectedEllipsisItemText, ellipsisItemTextBlock.GetText(),
                "The ellipsis item doesn't match " + expectedEllipsisItemText);
        }

        private void VerifyLastClickedItemIs(string expected)
        {
            var lastClickedItem = GetLastClickedItem();
            Verify.AreEqual(expected, lastClickedItem,
                    "The text in the button " + lastClickedItem + "doesn't match the expected '" + expected + "'");
        }

        private void VerifyLastClickedItemIndexIs(int expected)
        {
            var lastClickedItemIndex = GetLastClickedItemIndex();
            Verify.AreEqual(expected, lastClickedItemIndex,
                    "The last clicked item index " + lastClickedItemIndex + "doesn't match the expected " + expected);
        }

        private string GetLastClickedItem()
        {
            var lastItemTextBlock = FindElement.ByName<TextBlock>("LastClickedItem");
            return lastItemTextBlock.DocumentText;
        }

        private int GetLastClickedItemIndex()
        {
            var lastItemTextBlock = FindElement.ByName<TextBlock>("LastClickedItemIndex");
            return Int32.Parse(lastItemTextBlock.DocumentText);
        }
    }
}
