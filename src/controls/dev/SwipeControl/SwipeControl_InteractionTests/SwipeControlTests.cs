﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.System.Threading;
using Windows.Foundation;
using Windows.Foundation.Metadata;
using Common;
using System;
using System.Collections.Generic;
using System.Threading;
using System.Threading.Tasks;

using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Microsoft.UI.Xaml.Tests.MUXControls.InteractionTests
{
    using Window = Microsoft.Windows.Apps.Test.Foundation.Controls.Window;

    [TestClass]
    public class SwipeControlTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("TestPass:MinOSVer", WindowsOSVersion.RS5)] // Touch input injection is only supported on RS5 and up
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }
        
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeToExecuteHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl1");
                var SwipeControl1 = FindElement.ByName("SwipeControl1");
                Verify.IsNotNull(SwipeControl1);
                Log.Comment("Find the SwipeItem1IdleCheckBox");
                CheckBox SwipeItem1IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem1IdleCheckBox"));
                Verify.IsNotNull(SwipeItem1IdleCheckBox, "Verifying that SwipeItem1IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem1OpenCheckBox");
                CheckBox SwipeItem1OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem1OpenCheckBox"));
                Verify.IsNotNull(SwipeItem1OpenCheckBox, "Verifying that SwipeItem1OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl1, Direction.West);
                WaitForChecked(SwipeItem1OpenCheckBox);
                WaitForChecked(SwipeItem1IdleCheckBox);
                WaitForUnchecked(SwipeItem1OpenCheckBox);
                WaitForChecked(SwipeItem1IdleCheckBox);
                Verify.AreEqual("command's text", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeToExecuteVertical()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl8");
                var SwipeControl8 = FindElement.ByName("SwipeControl8");
                Verify.IsNotNull(SwipeControl8);
                Log.Comment("Find the SwipeItem8IdleCheckBox");
                CheckBox SwipeItem8IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem8IdleCheckBox"));
                Verify.IsNotNull(SwipeItem8IdleCheckBox, "Verifying that SwipeItem8IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem8OpenCheckBox");
                CheckBox SwipeItem8OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem8OpenCheckBox"));
                Verify.IsNotNull(SwipeItem8OpenCheckBox, "Verifying that SwipeItem8OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl8, Direction.North);
                WaitForChecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                WaitForUnchecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                Verify.AreEqual("ExecuteItem", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanClearItemsWithoutCrashing()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to clear items test page");
                UIObject navigateToClearPageObject = FindElement.ByName("navigateToClear");
                Verify.IsNotNull(navigateToClearPageObject, "Verifying that navigateToClear Button was found");

                Button navigateToClearPageButton = new Button(navigateToClearPageObject);
                navigateToClearPageButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find FindItemsSum textblock");
                TextBlock sumOfSwipeItemsCount = new TextBlock(FindElement.ByName("SwipeItemsChildSum"));
                Verify.IsNotNull(sumOfSwipeItemsCount);
                Verify.AreEqual("2", sumOfSwipeItemsCount.GetText());

                Log.Comment("Find clear SwipeItems button");
                Button clearItemsButton = new Button(FindElement.ByName("ClearItemsButton"));
                Verify.IsNotNull(clearItemsButton);
                clearItemsButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("0", sumOfSwipeItemsCount.GetText());


                Log.Comment("Find add SwipeItem button");
                Button addItemsButton = new Button(FindElement.ByName("AddItemsButton"));
                Verify.IsNotNull(addItemsButton);
                addItemsButton.Invoke();
                Wait.ForIdle();
                // Only adds horizontal items, see test app for explanation
                Verify.AreEqual("1", sumOfSwipeItemsCount.GetText());

                Log.Comment("clearing items again");
                clearItemsButton.Invoke();
                Wait.ForIdle();
                Verify.AreEqual("0", sumOfSwipeItemsCount.GetText());


            }
        }


        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeAndTapFirstRevealedItemHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeAndTapSecondtRevealedItemHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeAndTapFirstRevealedItemVertical()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl8");
                var SwipeControl8 = FindElement.ByName("SwipeControl8");
                Verify.IsNotNull(SwipeControl8);
                Log.Comment("Find the SwipeItem8IdleCheckBox");
                CheckBox SwipeItem8IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem8IdleCheckBox"));
                Verify.IsNotNull(SwipeItem8IdleCheckBox, "Verifying that SwipeItem8IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem8OpenCheckBox");
                CheckBox SwipeItem8OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem8OpenCheckBox"));
                Verify.IsNotNull(SwipeItem8OpenCheckBox, "Verifying that SwipeItem8OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl8, Direction.South);
                WaitForChecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem8OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem8IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }
        
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeAndTapSecondRevealedItemVertical()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl8");
                var SwipeControl8 = FindElement.ByName("SwipeControl8");
                Verify.IsNotNull(SwipeControl8);
                Log.Comment("Find the SwipeItem8IdleCheckBox");
                CheckBox SwipeItem8IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem8IdleCheckBox"));
                Verify.IsNotNull(SwipeItem8IdleCheckBox, "Verifying that SwipeItem8IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem8OpenCheckBox");
                CheckBox SwipeItem8OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem8OpenCheckBox"));
                Verify.IsNotNull(SwipeItem8OpenCheckBox, "Verifying that SwipeItem8OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl8, Direction.South);
                WaitForChecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]// replace with Dmitry's more reliable method
        [TestProperty("Ignore", "True")]  // Disabled as per tracking issue #3125
        [TestProperty("TestSuite", "A")]
        public void SwipeDoesntJumpWhenItReverts()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                var XPosition = new TextBlock(FindElement.ByName("PositionX"));
                Verify.IsNotNull(XPosition);
                //SwipeTapAndVerify("SwipeControl4", Direction.East, "RevealItem1", "RevealItem1");
                Verify.AreEqual("146", XPosition.DocumentText);
                KeyboardHelper.PressKey(Key.PageDown);

                bool pass = false;
                while (XPosition.DocumentText != 0.ToString())
                {
                    double xPoistion = 0;
                    Double.TryParse(XPosition.DocumentText.ToString(), out xPoistion);

                    // ensures that we get an intermediate value at least once.
                    if (xPoistion < 146 && xPoistion > 0)
                    {
                        pass = true;
                    }
                }

                Verify.AreEqual(true, pass);
                //EnsureAllSwipesAreDismissed();

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }


        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeAndTapBothItemsHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void CanSwipeMultipleItemsWithSameSwipeService()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl2");
                var SwipeControl2 = FindElement.ByName("SwipeControl2");
                Verify.IsNotNull(SwipeControl2);
                Log.Comment("Find the SwipeItem2IdleCheckBox");
                CheckBox SwipeItem2IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem2IdleCheckBox"));
                Verify.IsNotNull(SwipeItem2IdleCheckBox, "Verifying that SwipeItem2IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem2OpenCheckBox");
                CheckBox SwipeItem2OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem2OpenCheckBox"));
                Verify.IsNotNull(SwipeItem2OpenCheckBox, "Verifying that SwipeItem2OpenCheckBox Button was found");
                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                PerformSwipe(SwipeControl2, Direction.East);
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                WaitForChecked(SwipeItem2OpenCheckBox);
                WaitForChecked(SwipeItem2IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem2OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem2IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
         public void CanAddSameSwipeItemsInBothSidesHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                PerformSwipe(SwipeControl4, Direction.West);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }
        
        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateItemsLessThanThresholdDontRevertHorizontal() // add a vertical equivalent
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl2");
                var SwipeControl2 = FindElement.ByName("SwipeControl2");
                Verify.IsNotNull(SwipeControl2);
                Log.Comment("Find the SwipeItem2IdleCheckBox");
                CheckBox SwipeItem2IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem2IdleCheckBox"));
                Verify.IsNotNull(SwipeItem2IdleCheckBox, "Verifying that SwipeItem2IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem2OpenCheckBox");
                CheckBox SwipeItem2OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem2OpenCheckBox"));
                Verify.IsNotNull(SwipeItem2OpenCheckBox, "Verifying that SwipeItem2OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl2, Direction.East);
                WaitForChecked(SwipeItem2OpenCheckBox);
                WaitForChecked(SwipeItem2IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem2OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem2IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void ValidateSwipeDoesntCrashOnAddingMoreItemsThanItsSizeHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl3");
                var SwipeControl3 = FindElement.ByName("SwipeControl3");
                Verify.IsNotNull(SwipeControl3);
                Log.Comment("Find the SwipeItem3IdleCheckBox");
                CheckBox SwipeItem3IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem3IdleCheckBox"));
                Verify.IsNotNull(SwipeItem3IdleCheckBox, "Verifying that SwipeItem3IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem3OpenCheckBox");
                CheckBox SwipeItem3OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem3OpenCheckBox"));
                Verify.IsNotNull(SwipeItem3OpenCheckBox, "Verifying that SwipeItem3OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl3, Direction.East);
                WaitForChecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                TapItem("RevealItem3");
                Verify.AreEqual(ToggleState.On, SwipeItem3OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem3IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem3", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("Description", "When swiping items not set on one side, the main content shouldn't move, and it shouldn't throw an access violation exception.")]
        [TestProperty("TestSuite", "A")]
        public void ValidateSwipeCanHaveNoItemsOnASideHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Get the two swipe controls.");
                var swipeControl8 = FindElement.ByName("SwipeControl8");

                Verify.IsNotNull(swipeControl8);

                Log.Comment("Get the two swipe controls.");
                var swipeControl9 = FindElement.ByName("SwipeControl9");

                Verify.IsNotNull(swipeControl8);
                Verify.IsNotNull(swipeControl9);

                // Verifying it doesn't crash 
                // same note as before, it might not open, and we still pass the test.
                PerformSwipe(swipeControl8, Direction.East);
                PerformSwipe(swipeControl9, Direction.West);
                
                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void PressingAnyKeyDismissesSwipe()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                KeyboardHelper.PressKey(Key.PageDown);
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        [TestProperty("Ignore", "True")] // TODO 29556945: Disabled after converting MUXControlsTestApp to a desktop .NET 5 app.  Re-enable when fixed.
        public void TogglingTheWindowStateDismissesSwipe()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();
                
                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                Window window = new Window(TestEnvironment.Application.ApplicationFrameWindow);
                WindowVisualState initialVisualState = window.WindowVisualState;
                WindowVisualState desiredVisualState;

                if(initialVisualState == WindowVisualState.Normal)
                {
                    desiredVisualState = WindowVisualState.Maximized;
                }
                else
                {
                    desiredVisualState = WindowVisualState.Normal;
                }

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                // Maximize the app, which will also trigger it to supsend. Wait for Suspending event from app.
                Log.Comment("Toggling the window's state...");
                window.SetWindowVisualState(desiredVisualState);
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);


                window.SetWindowVisualState(WindowVisualState.Maximized);
                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "A")]
        public void SwipeIsNotTabStop()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Getting swipe controls.");
                var swipeControl1 = FindElement.ByName("SwipeControl1");
                var swipeControl2 = FindElement.ByName("SwipeControl2");
                Verify.IsNotNull(swipeControl1);
                Verify.IsNotNull(swipeControl2);

                FocusHelper.SetFocus(swipeControl1);
                Verify.IsTrue(swipeControl1.HasKeyboardFocus);
                KeyboardHelper.PressKey(Key.Tab);
                Verify.IsTrue(swipeControl2.HasKeyboardFocus);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void AdditionalGridDoesntOverExtendHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                var textbox = new TextBlock(FindElement.ByName("TextBox"));
                Verify.IsNotNull(textbox);

                PerformSwipe(FindElement.ByName("SwipeControl3"), Direction.West);

                Verify.AreEqual("372, 284", textbox.DocumentText);
                TapItem("Scale Up");
                Verify.AreEqual("672, 284", textbox.DocumentText);
                TapItem("Scale Down");
                Verify.AreEqual("372, 284", textbox.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ContentGridDoesntOverExtendHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl1");
                var SwipeControl1 = FindElement.ByName("SwipeControl1");
                Verify.IsNotNull(SwipeControl1);
                Log.Comment("Find the SwipeItem1IdleCheckBox");
                CheckBox SwipeItem1IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem1IdleCheckBox"));
                Verify.IsNotNull(SwipeItem1IdleCheckBox, "Verifying that SwipeItem1IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem1OpenCheckBox");
                CheckBox SwipeItem1OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem1OpenCheckBox"));
                Verify.IsNotNull(SwipeItem1OpenCheckBox, "Verifying that SwipeItem1OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl1, Direction.East);
                WaitForChecked(SwipeItem1OpenCheckBox);
                WaitForChecked(SwipeItem1IdleCheckBox);
                WaitForUnchecked(SwipeItem1OpenCheckBox);
                WaitForChecked(SwipeItem1IdleCheckBox);
                Verify.AreEqual("Scale Up", textblock.DocumentText);

                var textbox = new TextBlock(FindElement.ByName("TextBox"));
                Verify.IsNotNull(textbox);

                Verify.AreEqual("672, 672", textbox.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CantOverSwipeToTheOtherSideWhenSwipeItemsAreRevealedHorizontal()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();
                
                Log.Comment("Find the SwipeControl2");
                var SwipeControl2 = FindElement.ByName("SwipeControl2");
                Verify.IsNotNull(SwipeControl2);
                Log.Comment("Find the SwipeItem2IdleCheckBox");
                CheckBox SwipeItem2IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem2IdleCheckBox"));
                Verify.IsNotNull(SwipeItem2IdleCheckBox, "Verifying that SwipeItem2IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem2OpenCheckBox");
                CheckBox SwipeItem2OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem2OpenCheckBox"));
                Verify.IsNotNull(SwipeItem2OpenCheckBox, "Verifying that SwipeItem2OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl2, Direction.East);
                WaitForChecked(SwipeItem2OpenCheckBox);
                WaitForChecked(SwipeItem2IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem2OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem2IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                PerformSwipe(SwipeControl2, Direction.West);
                WaitForUnchecked(SwipeItem2OpenCheckBox);
                WaitForChecked(SwipeItem2IdleCheckBox);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CantOverSwipeToTheOtherSideWhenSwipeItemsAreRevealedVertical()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl8");
                var SwipeControl8 = FindElement.ByName("SwipeControl8");
                Verify.IsNotNull(SwipeControl8);
                Log.Comment("Find the SwipeItem8IdleCheckBox");
                CheckBox SwipeItem8IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem8IdleCheckBox"));
                Verify.IsNotNull(SwipeItem8IdleCheckBox, "Verifying that SwipeItem8IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem8OpenCheckBox");
                CheckBox SwipeItem8OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem8OpenCheckBox"));
                Verify.IsNotNull(SwipeItem8OpenCheckBox, "Verifying that SwipeItem8OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl8, Direction.South);
                WaitForChecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem8OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem8IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                PerformSwipe(SwipeControl8, Direction.North);
                WaitForUnchecked(SwipeItem8OpenCheckBox);
                WaitForChecked(SwipeItem8IdleCheckBox);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SwipeDismissThenSwipe()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                TapItem("ResetButton");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CanSwipeBackFromAnOpenedSwipe()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);
                PerformSwipe(SwipeControl4, Direction.West);
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CanSwipeOtherDirectionAfterClosingAnOpenedSwipe()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem1");
                Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
                Verify.AreEqual("RevealItem1", textblock.DocumentText);

                TapItem("ResetButton");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);

                PerformSwipe(SwipeControl4, Direction.West);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);
                Verify.AreEqual("RevealItem2", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SwipingDoesntModifyTheSizeOfSwipeControl()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                var resetButton = new Button(FindElement.ByName("ResetButton"));
                Verify.IsNotNull(resetButton);

                // Horizontal, and Vertical
                var swipes = new[] { FindElement.ByName("SwipeControl2"), FindElement.ByName("SwipeControl8") };
                var directions = new[] { Direction.West, Direction.North };
                var idleCheckBoxes = new[] { new CheckBox(FindElement.ByName("SwipeItem2IdleCheckBox")), new CheckBox(FindElement.ByName("SwipeItem8IdleCheckBox")) };
                var openCheckBoxes = new[] { new CheckBox(FindElement.ByName("SwipeItem2OpenCheckBox")), new CheckBox(FindElement.ByName("SwipeItem8OpenCheckBox")) };
                for (int i = 0; i < swipes.Length; i++)
                {
                    var swipe = swipes[i];
                    var direction = directions[i];
                    var idleCheckBox = idleCheckBoxes[i];
                    var openCheckBox = openCheckBoxes[i];
                    Verify.IsNotNull(swipe);
                    int widthBefore = swipe.BoundingRectangle.Width;
                    int heightBefore = swipe.BoundingRectangle.Height;
                    PerformSwipe(swipe, direction);
                    int widthDuring = swipe.BoundingRectangle.Width;
                    int heightDuring = swipe.BoundingRectangle.Height;
                    WaitForChecked(openCheckBox);
                    WaitForChecked(idleCheckBox);
                    WaitForUnchecked(openCheckBox);
                    WaitForChecked(idleCheckBox);

                    int widthAfter = swipe.BoundingRectangle.Width;
                    int heightAfter = swipe.BoundingRectangle.Height;
                    Verify.AreEqual("ExecuteItem", textblock.DocumentText);
                    Verify.AreEqual(widthBefore, widthDuring);
                    Verify.AreEqual(heightBefore, heightDuring);
                    Verify.AreEqual(widthBefore, widthAfter);
                    Verify.AreEqual(heightBefore, heightAfter);
                    resetButton.Invoke();
                }

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }
        
        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SwipeItemsAreNullifiedWhenTheAnimationIsDone()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                // Bug: 12734833
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    return;
                }

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl4");
                var SwipeControl4 = FindElement.ByName("SwipeControl4");
                Verify.IsNotNull(SwipeControl4);
                Log.Comment("Find the SwipeItem4IdleCheckBox");
                CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
                Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem4OpenCheckBox");
                CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
                Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl4, Direction.East);
                WaitForChecked(SwipeItem4OpenCheckBox);
                WaitForChecked(SwipeItem4IdleCheckBox);

                using (Context.RawContext.Activate())
                {
                    var item1 = FindElement.ByName("RevealItem1");
                    var firstChild = SwipeControl4.FirstChild;

                    Verify.IsNotNull(item1);
                    Verify.IsNotNull(firstChild);
                    Verify.AreEqual(item1, firstChild);

                    PerformSwipe(SwipeControl4, Direction.West);
                    WaitForUnchecked(SwipeItem4OpenCheckBox);
                    WaitForChecked(SwipeItem4IdleCheckBox);

                    firstChild = SwipeControl4.FirstChild;
                    // The first child becomes the text block
                    Verify.AreNotEqual(item1, firstChild);
                }

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [Description("Regression test for MSFT:9096017 - Validates that after an initial swipe, SwipeControl doesn't offset to the point where you first released your finger on the start of the new interaction.")]
        [TestProperty("TestSuite", "B")]
        public void SwipingDoesntChangeTheBoundsOfSwipeControl()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                if (PlatformConfiguration.IsDevice(DeviceType.Phone))
                {
                    Log.Warning("Test is disabled on phone.");
                    return;
                }
                
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                var SwipeControl6 = FindElement.ByName("SwipeControl6");

                // Save off the original bounding rectangle so that we can validate that the
                // swipe container did not offset on the next interaction.
                var originalBounds = SwipeControl6.BoundingRectangle;

                Log.Comment("Initiate the first interaction (a swipe).");

                Verify.IsNotNull(SwipeControl6);
                PerformSwipe(SwipeControl6, Direction.West);

                AutoResetEvent autoEvent = new AutoResetEvent(false);

                // Execute the TapAndHold call on an off thread so that we can validate
                // the bounds hasn't changed during the hold.
                var op = global::System.Threading.ThreadPool.QueueUserWorkItem(new WaitCallback((object state) =>
                {
                    Log.Comment("Initiate a long tap and validate that the bounding rectangle doesn't change.");
                    Verify.IsNotNull(SwipeControl6);
                    InputHelper.TapAndHold(SwipeControl6, 5000);
                    autoEvent.Set();
                }));

                Log.Comment("Main thread start to validate the bounds");

                while (!autoEvent.WaitOne(250))
                {
                    Verify.AreEqual(originalBounds.X, SwipeControl6.BoundingRectangle.X);
                    Verify.AreEqual(originalBounds.Y, SwipeControl6.BoundingRectangle.Y);
                }

                Log.Comment("Main thread stop the validation");

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void TappingOutsideARevealedSwipeDismissesIt()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                DismissRevealedSwipe(true);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void ClickingOutsideARevealedSwipeDismissesIt()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                DismissRevealedSwipe(false);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SwipeItemInheritsICommand2Properties()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl10");
                var SwipeControl10 = FindElement.ByName("SwipeControl10");
                Verify.IsNotNull(SwipeControl10);
                Log.Comment("Find the SwipeItem10IdleCheckBox");
                CheckBox SwipeItem10IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem10IdleCheckBox"));
                Verify.IsNotNull(SwipeItem10IdleCheckBox, "Verifying that SwipeItem10IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem10OpenCheckBox");
                CheckBox SwipeItem10OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem10OpenCheckBox"));
                Verify.IsNotNull(SwipeItem10OpenCheckBox, "Verifying that SwipeItem10OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl10, Direction.East);
                WaitForChecked(SwipeItem10OpenCheckBox);
                WaitForChecked(SwipeItem10IdleCheckBox);
                TapItem("UICommand Label");
                Verify.AreEqual(ToggleState.On, SwipeItem10OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem10IdleCheckBox.ToggleState);
                Verify.AreEqual("UICommand Label", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void CloseOnOpenedAndClosedSwipeItemsWorks()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl3");
                var SwipeControl3 = FindElement.ByName("SwipeControl3");
                Verify.IsNotNull(SwipeControl3);
                Log.Comment("Find the SwipeItem3IdleCheckBox");
                CheckBox SwipeItem3IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem3IdleCheckBox"));
                Verify.IsNotNull(SwipeItem3IdleCheckBox, "Verifying that SwipeItem3IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem3OpenCheckBox");
                CheckBox SwipeItem3OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem3OpenCheckBox"));
                Verify.IsNotNull(SwipeItem3OpenCheckBox, "Verifying that SwipeItem3OpenCheckBox Button was found");
                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                PerformSwipe(SwipeControl3, Direction.East);
                WaitForChecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                TapItem("RevealItem5");
                Verify.AreEqual(ToggleState.On, SwipeItem3OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem3IdleCheckBox.ToggleState);
                Verify.AreEqual("sc11.Close()", textblock.DocumentText);
                TapItem("RevealItem4");
                WaitForUnchecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                Verify.AreEqual("sc3.Close()", textblock.DocumentText);

                Log.Comment("Find the SwipeControl9");
                var SwipeControl9 = FindElement.ByName("SwipeControl9");
                Verify.IsNotNull(SwipeControl9);
                Log.Comment("Find the SwipeItem9IdleCheckBox");
                CheckBox SwipeItem9IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem9IdleCheckBox"));
                Verify.IsNotNull(SwipeItem9IdleCheckBox, "Verifying that SwipeItem9IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem9OpenCheckBox");
                CheckBox SwipeItem9OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem9OpenCheckBox"));
                Verify.IsNotNull(SwipeItem9OpenCheckBox, "Verifying that SwipeItem9OpenCheckBox Button was found");

                PerformSwipe(SwipeControl9, Direction.South);
                WaitForChecked(SwipeItem9OpenCheckBox);
                WaitForChecked(SwipeItem9IdleCheckBox);
                WaitForUnchecked(SwipeItem9OpenCheckBox);
                WaitForChecked(SwipeItem9IdleCheckBox);
                Verify.AreEqual("sc9.Close()", textblock.DocumentText);

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        //[TestMethod] Bug 24048408: DCPP: Multiple MUXC interaction tests fail in CatGates due to taking too long to execute
        [TestProperty("TestSuite", "B")]
        public void OnlyProgramaticCloseWillCloseRemainOpenExecuteItems()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Find the SwipeControl11");
                var SwipeControl11 = FindElement.ByName("SwipeControl11");
                Verify.IsNotNull(SwipeControl11);
                Log.Comment("Find the SwipeItem11IdleCheckBox");
                CheckBox SwipeItem11IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem11IdleCheckBox"));
                Verify.IsNotNull(SwipeItem11IdleCheckBox, "Verifying that SwipeItem11IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem11OpenCheckBox");
                CheckBox SwipeItem11OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem11OpenCheckBox"));
                Verify.IsNotNull(SwipeItem11OpenCheckBox, "Verifying that SwipeItem11OpenCheckBox Button was found");

                Log.Comment("Find the SwipeControl3");
                var SwipeControl3 = FindElement.ByName("SwipeControl3");
                Verify.IsNotNull(SwipeControl3);
                Log.Comment("Find the SwipeItem3IdleCheckBox");
                CheckBox SwipeItem3IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem3IdleCheckBox"));
                Verify.IsNotNull(SwipeItem3IdleCheckBox, "Verifying that SwipeItem3IdleCheckBox Button was found");
                Log.Comment("Find the SwipeItem3OpenCheckBox");
                CheckBox SwipeItem3OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem3OpenCheckBox"));
                Verify.IsNotNull(SwipeItem3OpenCheckBox, "Verifying that SwipeItem3OpenCheckBox Button was found");

                Log.Comment("Getting the textblock");
                var textblock = new TextBlock(FindElement.ByName("TextBlock"));
                Verify.IsNotNull(textblock);

                Window window = new Window(TestEnvironment.Application.ApplicationFrameWindow);

                PerformSwipe(SwipeControl11, Direction.East);
                WaitForChecked(SwipeItem11OpenCheckBox);
                WaitForChecked(SwipeItem11IdleCheckBox);
                Verify.AreEqual("ExecuteRemainOpen", textblock.DocumentText);
                PerformSwipe(SwipeControl11, Direction.West);
                Verify.AreEqual(ToggleState.On, SwipeItem11OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem11IdleCheckBox.ToggleState);
                Verify.AreEqual("ExecuteRemainOpen", textblock.DocumentText);
                window.SetWindowVisualState(WindowVisualState.Normal);
                window.SetWindowVisualState(WindowVisualState.Maximized);
                Verify.AreEqual(ToggleState.On, SwipeItem11OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem11IdleCheckBox.ToggleState);
                Verify.AreEqual("ExecuteRemainOpen", textblock.DocumentText);
                TapItem("ExecuteRemainOpen");
                Verify.AreEqual(ToggleState.On, SwipeItem11OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem11IdleCheckBox.ToggleState);
                Verify.AreEqual("ExecuteRemainOpen", textblock.DocumentText);
                PerformSwipe(SwipeControl3, Direction.East);
                WaitForChecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                Verify.AreEqual(ToggleState.On, SwipeItem11OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem11IdleCheckBox.ToggleState);
                Verify.AreEqual("ExecuteRemainOpen", textblock.DocumentText);
                TapItem("RevealItem5");
                WaitForUnchecked(SwipeItem11OpenCheckBox);
                WaitForChecked(SwipeItem11IdleCheckBox);
                Verify.AreEqual(ToggleState.On, SwipeItem3OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem3IdleCheckBox.ToggleState);
                Verify.AreEqual("sc11.Close()", textblock.DocumentText);
                
                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        [TestMethod]
        [TestProperty("TestSuite", "B")]
        public void SwipeContentBackgroundResetsOnClose()
        {
            using (var setup = new TestSetupHelper("SwipeControl Tests"))
            {
                SetOutputDebugStringLevel("Info");

                Log.Comment("Navigating to List Items with simple swipe items");
                UIObject navigateToSimpleContentsUIObject = FindElement.ByName("navigateToSimpleContents");
                Verify.IsNotNull(navigateToSimpleContentsUIObject, "Verifying that navigateToSimpleContents Button was found");

                Button navigateToSimpleContentsButton = new Button(navigateToSimpleContentsUIObject);
                navigateToSimpleContentsButton.Invoke();
                Wait.ForIdle();

                SetLoggingLevel(isPrivateLoggingEnabled: true);

                Log.Comment("Find the SwipeControl3");
                var SwipeControl3 = FindElement.ByName("SwipeControl3");
                Verify.IsNotNull(SwipeControl3);
                Log.Comment("Find the SwipeItem3IdleCheckBox");
                CheckBox SwipeItem3IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem3IdleCheckBox"));
                Log.Comment("Find the SwipeItem3OpenCheckBox");
                CheckBox SwipeItem3OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem3OpenCheckBox"));

                Log.Comment("Getting the Swipe3BackgroundColorTextBlock");
                var textblock = new TextBlock(FindElement.ByName("Swipe3BackgroundColorTextBlock"));
                
                PerformSwipe(SwipeControl3, Direction.East);
                WaitForChecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                Verify.AreEqual(ToggleState.On, SwipeItem3OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem3IdleCheckBox.ToggleState);
                Verify.AreEqual("#FF008000", textblock.DocumentText);

                TapItem("RevealItem2");
                WaitForUnchecked(SwipeItem3OpenCheckBox);
                WaitForChecked(SwipeItem3IdleCheckBox);
                Verify.AreEqual(ToggleState.Off, SwipeItem3OpenCheckBox.ToggleState);
                Verify.AreEqual(ToggleState.On, SwipeItem3IdleCheckBox.ToggleState);
                Verify.AreEqual("null", textblock.DocumentText);

                SetLoggingLevel(isPrivateLoggingEnabled: false);
                LogTraces();

                Log.Comment("Returning to the main Swipe test page");
                TestSetupHelper.GoBack();
            }
        }

        private void DismissRevealedSwipe(bool withOutsideTap)
        {
            if (PlatformConfiguration.IsDevice(DeviceType.Phone))
            {
                Log.Warning("Test is disabled on phone.");
                return;
            }

            Log.Comment("Find the SwipeControl4");
            var SwipeControl4 = FindElement.ByName("SwipeControl4");
            Verify.IsNotNull(SwipeControl4);
            Log.Comment("Find the SwipeItem4IdleCheckBox");
            CheckBox SwipeItem4IdleCheckBox = new CheckBox(FindElement.ByName("SwipeItem4IdleCheckBox"));
            Verify.IsNotNull(SwipeItem4IdleCheckBox, "Verifying that SwipeItem4IdleCheckBox Button was found");
            Log.Comment("Find the SwipeItem4OpenCheckBox");
            CheckBox SwipeItem4OpenCheckBox = new CheckBox(FindElement.ByName("SwipeItem4OpenCheckBox"));
            Verify.IsNotNull(SwipeItem4OpenCheckBox, "Verifying that SwipeItem4OpenCheckBox Button was found");
            Log.Comment("Getting the textblock");
            var textblock = new TextBlock(FindElement.ByName("TextBlock"));
            Verify.IsNotNull(textblock);

            PerformSwipe(SwipeControl4, Direction.East);
            WaitForChecked(SwipeItem4OpenCheckBox);
            WaitForChecked(SwipeItem4IdleCheckBox);
            TapItem("RevealItem1");
            Verify.AreEqual(ToggleState.On, SwipeItem4OpenCheckBox.ToggleState);
            Verify.AreEqual(ToggleState.On, SwipeItem4IdleCheckBox.ToggleState);
            Verify.AreEqual("RevealItem1", textblock.DocumentText);
            
            var swipePage = FindElement.ByName("SwipePage");

            if (withOutsideTap)
            {
                InputHelper.Tap(swipePage, 20, 20);
            }
            else
            {
                InputHelper.LeftClick(swipePage, 20, 20);
            }

            WaitForUnchecked(SwipeItem4OpenCheckBox);
            WaitForChecked(SwipeItem4IdleCheckBox);
        }

        private void PerformSwipe(UIObject element, Direction direction)
        {
            int distance = 150;
            InputHelper.Pan(element, distance, direction, 1500, 0.001f, 100, false);
            Log.Comment("PerformSwipe completed.");
        }

        private bool WaitForChecked(CheckBox checkBox)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.On);
        }

        private bool WaitForUnchecked(CheckBox checkBox)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.Off);
        }

        // Swipe operation takes 100ms(swipe) + 1500ms(hold) = 1600ms to complete.
        // For IdleCheckbox, we also need to wait for the released item to go back to its original position and trigger IdleStatusChanged event.
        // Made the default timeout limit 4000ms to give it enough time to finish.
        private bool WaitForCheckBoxUpdated(CheckBox checkBox, ToggleState state, double millisecondsTimeout = 4000, bool throwOnError = true)
        {
            using (var waiter = checkBox.GetToggledWaiter())
            {
                Log.Comment(checkBox.Name + " Checked: " + checkBox.ToggleState);
                if (checkBox.ToggleState == state)
                {
                    return true;
                }
                else
                {
                    Log.Comment("Waiting for toggle state to change to {0} for {1}ms", state, millisecondsTimeout);
                    waiter.TryWait(TimeSpan.FromMilliseconds(millisecondsTimeout));
                }
                if (checkBox.ToggleState != state)
                {
                    Log.Warning(checkBox.Name + " value never changed");
                    if (throwOnError)
                    {
                        throw new WaiterException();
                    }
                    else
                    {
                        return false;
                    }
                }
                return true;
            }
        }

        private void TapItem(String item)
        {
            ElementCache.Clear();
            using (Context.RawContext.Activate())
            {
                Log.Comment("Tapping on " + item);
                var uiItem = FindElement.ByName(item);
                Verify.IsNotNull(uiItem);

                if (String.IsNullOrEmpty(uiItem.Name) || String.IsNullOrEmpty(uiItem.AutomationId))
                {
                    Log.Warning("Item {0} isn't null but doesn't exist anymore.", item);
                }

                InputHelper.Tap(uiItem);
            }
        }

        // outputDebugStringLevel can be "None", "Info" or "Verbose"
        private void SetOutputDebugStringLevel(string outputDebugStringLevel)
        {
            Log.Comment("Retrieving cmbSwipeControlOutputDebugStringLevel");
            ComboBox cmbSwipeControlOutputDebugStringLevel = new ComboBox(FindElement.ByName("cmbSwipeControlOutputDebugStringLevel"));
            Verify.IsNotNull(cmbSwipeControlOutputDebugStringLevel, "Verifying that cmbSwipeControlOutputDebugStringLevel was found");

            Log.Comment("Changing output-debug-string-level selection to " + outputDebugStringLevel);
            cmbSwipeControlOutputDebugStringLevel.SelectItemByName(outputDebugStringLevel);
            Log.Comment("Selection is now {0}", cmbSwipeControlOutputDebugStringLevel.Selection[0].Name);
        }

        private void SetLoggingLevel(bool isPrivateLoggingEnabled)
        {
            Log.Comment("Retrieving chkLogSwipeControlMessages");
            CheckBox chkLogSwipeControlMessages = new CheckBox(FindElement.ById("chkLogSwipeControlMessages"));
            Verify.IsNotNull(chkLogSwipeControlMessages, "Verifying that chkLogSwipeControlMessages was found");

            if (isPrivateLoggingEnabled && chkLogSwipeControlMessages.ToggleState != ToggleState.On ||
                !isPrivateLoggingEnabled && chkLogSwipeControlMessages.ToggleState != ToggleState.Off)
            {
                Log.Comment("Toggling chkLogSwipeControlMessages.IsChecked to " + isPrivateLoggingEnabled);
                chkLogSwipeControlMessages.Toggle();
                Wait.ForIdle();
            }
        }

        private void LogTraces()
        {
            LogTraces(recordWarning: false);
        }

        private void LogTraces(bool recordWarning)
        {
            List<string> traces = recordWarning ? new List<string>() : null;

            Log.Comment("Reading full log:");

            UIObject fullLogUIObject = FindElement.ById("cmbFullLog");
            Verify.IsNotNull(fullLogUIObject);
            ComboBox cmbFullLog = new ComboBox(fullLogUIObject);
            Verify.IsNotNull(cmbFullLog);

            UIObject getFullLogUIObject = FindElement.ById("btnGetFullLog");
            Verify.IsNotNull(getFullLogUIObject);
            Button getFullLogButton = new Button(getFullLogUIObject);
            Verify.IsNotNull(getFullLogButton);

            getFullLogButton.Invoke();
            Wait.ForIdle();

            foreach (ComboBoxItem item in cmbFullLog.AllItems)
            {
                string trace = item.Name;

                if (recordWarning)
                {
                    traces.Add(trace);
                }
                Log.Comment(trace);
            }

            if (recordWarning)
            {
                string warning = "Non-final test pass failed.";
                Log.Warning(warning);

                WarningReportHelper.Record(warning, traces);
            }
        }

        private void ClearTraces()
        {
            Log.Comment("Clearing full log.");

            UIObject clearFullLogUIObject = FindElement.ById("btnClearFullLog");
            Verify.IsNotNull(clearFullLogUIObject);
            Button clearFullLogButton = new Button(clearFullLogUIObject);
            Verify.IsNotNull(clearFullLogButton);

            clearFullLogButton.Invoke();
            Wait.ForIdle();
        }

        private void LogAndClearTraces()
        {
            LogAndClearTraces(recordWarning: false);
        }

        private void LogAndClearTraces(bool recordWarning)
        {
            LogTraces(recordWarning);
            ClearTraces();
        }
    }
}
