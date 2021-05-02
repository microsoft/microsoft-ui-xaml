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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class InfoBarTests
    {
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
        public void IsClosableTest()
        {
            using (var setup = new TestSetupHelper("InfoBar Tests"))
            {
                StatusBar infoBar = FindElement.ByName<StatusBar>("TestInfoBar");

                Button closeButton = FindCloseButton(infoBar);
                Verify.IsNotNull(closeButton, "Close button should be visible by default");

                Uncheck("IsClosableCheckBox");
                ElementCache.Clear();
                closeButton = FindCloseButton(infoBar);
                Verify.IsNull(closeButton, "Close button should not be visible when IsClosable=false");

                Check("IsClosableCheckBox");
                ElementCache.Clear();
                closeButton = FindCloseButton(infoBar);
                Verify.IsNotNull(closeButton, "Close button should be visible when IsClosable=true");
            }
        }

        [TestMethod]
        public void CloseTest()
        {
            using (var setup = new TestSetupHelper("InfoBar Tests"))
            {
                StatusBar infoBar = FindElement.ByName<StatusBar>("TestInfoBar");

                Log.Comment("Clicking the close button");
                Button closeButton = FindCloseButton(infoBar);
                closeButton.InvokeAndWait();

                ListBox events = FindElement.ByName<ListBox>("EventListBox");
                Verify.AreEqual(3, events.Items.Count);
                Verify.AreEqual("CloseButtonClick", events.Items[0].Name, "First event should be the CloseButtonClick event");
                Verify.AreEqual("Closing: CloseButton", events.Items[1].Name, "Second event should be the Closing event, reason=CloseButton");
                Verify.AreEqual("Closed: CloseButton", events.Items[2].Name, "Third event should be the Closed event, reason=CloseButton");

                CheckBox isOpenCheckBox = FindElement.ByName<CheckBox>("IsOpenCheckBox");
                Verify.AreEqual(ToggleState.Off, isOpenCheckBox.ToggleState);

                // reopen
                isOpenCheckBox.Check();
                Check("CancelCheckBox");
                Button listBoxClearButton = FindElement.ByName<Button>("ClearButton");
                listBoxClearButton.InvokeAndWait();

                Log.Comment("Clicking the close button but cancel the closing event");
                closeButton.InvokeAndWait();

                Verify.AreEqual(2, events.Items.Count);
                Verify.AreEqual("CloseButtonClick", events.Items[0].Name, "First event should be the CloseButtonClick event");
                Verify.AreEqual("Closing: CloseButton", events.Items[1].Name, "Second event should be the Closing event, reason=CloseButton");
                Verify.AreEqual(ToggleState.On, isOpenCheckBox.ToggleState);

                Uncheck("CancelCheckBox");

                Log.Comment("Close programmatically");
                listBoxClearButton.InvokeAndWait();
                isOpenCheckBox.Uncheck();
                Wait.ForIdle();

                Verify.AreEqual(2, events.Items.Count);
                Verify.AreEqual("Closing: Programmatic", events.Items[0].Name, "Second event should be the Closing event, reason=Programmatic");
                Verify.AreEqual("Closed: Programmatic", events.Items[1].Name, "Third event should be the Closed event, reason=Programmatic");
                Verify.AreEqual(ToggleState.Off, isOpenCheckBox.ToggleState);
            }
        }

        [TestMethod]
        public void AccessibilityViewTest()
        {
            using (var setup = new TestSetupHelper("InfoBar Tests"))
            {
                StatusBar infoBar = FindElement.ByName<StatusBar>("TestInfoBar");
                Verify.IsNotNull(infoBar, "TestInfoBar should be visible by default");

                Log.Comment("Close InfoBar and make sure it can't be found.");
                Uncheck("IsOpenCheckBox");
                ElementCache.Clear();

                infoBar = FindElement.ByName<StatusBar>("TestInfoBar");
                Verify.IsNull(infoBar, "TestInfoBar should not be in the accessible tree");

                infoBar = FindElement.ByName<StatusBar>("DefaultInfoBar");
                Verify.IsNull(infoBar, "By default, Infobar should not be visible");
            }
        }

        [TestMethod]
        public void LayoutTest()
        {
            using (var setup = new TestSetupHelper("InfoBar Tests"))
            {
                ComboBox actionComboBox = FindElement.ByName<ComboBox>("ActionButtonComboBox");
                actionComboBox.SelectItemByName("Button");
                Check("HasCustomContentCheckBox");
                ElementCache.Clear();

                StatusBar infoBar = FindElement.ByName<StatusBar>("TestInfoBar");
                CheckBox customContent = FindElement.ByName<CheckBox>("CustomContentCheckBox");

                // 0: icon; 1: title; 2: message; 3: action button
                Log.Comment("Verify that title, message, and action button layout is left-to-right");
                Verify.IsGreaterThan(infoBar.Children[2].BoundingRectangle.X, infoBar.Children[1].BoundingRectangle.X, "Expect Message to be on the right of Title");
                Verify.IsGreaterThan(infoBar.Children[3].BoundingRectangle.X, infoBar.Children[2].BoundingRectangle.X, "Expect action button to be on the right of Message");
                VerifyIsPrettyClose(infoBar.Children[2].BoundingRectangle.Y, infoBar.Children[1].BoundingRectangle.Y, "Expect Message to be top-aligned with Title");

                Verify.IsGreaterThan(customContent.BoundingRectangle.Y, infoBar.Children[1].BoundingRectangle.Y, "Expect custom content to be on under all other things");
                VerifyIsPrettyClose(customContent.BoundingRectangle.X, infoBar.Children[1].BoundingRectangle.X, "Expect custom content to be left-aligned with title");

                Log.Comment("Change title and message to long strings");
                Edit editTitle = FindElement.ByName<Edit>("TitleTextBox");
                Edit editMessage = FindElement.ByName<Edit>("MessageTextBox");
                editTitle.SetValueAndWait("Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vivamus vel orci eros. Sed ut lectus ultrices quam hendrerit sagittis. Cras gravida eleifend eros, eu pulvinar lectus molestie dictum. Vivamus et tellus euismod, dapibus odio vel, volutpat risus.");
                editMessage.SetValueAndWait("Orci varius natoque penatibus et magnis dis parturient montes, nascetur ridiculus mus. Praesent vehicula mauris eu libero pretium ullamcorper.");

                Log.Comment("Verify that title, message, and action button layout is top-to-bottom");
                Verify.IsGreaterThan(infoBar.Children[2].BoundingRectangle.Y, infoBar.Children[1].BoundingRectangle.Y, "Expect Message to be below Title");
                Verify.IsGreaterThan(infoBar.Children[3].BoundingRectangle.Y, infoBar.Children[2].BoundingRectangle.Y, "Expect action button to be below Message");
                VerifyIsPrettyClose(infoBar.Children[2].BoundingRectangle.X, infoBar.Children[1].BoundingRectangle.X, "Expect Message to be left-aligned with Title");
                VerifyIsPrettyClose(infoBar.Children[3].BoundingRectangle.X, infoBar.Children[2].BoundingRectangle.X, "Expect action button to be left-aligned Message");

                Verify.IsGreaterThan(customContent.BoundingRectangle.Y, infoBar.Children[3].BoundingRectangle.Y, "Expect custom content to be on under all other things");
                VerifyIsPrettyClose(customContent.BoundingRectangle.X, infoBar.Children[1].BoundingRectangle.X, "Expect custom content to be left-aligned with everyone else");
            }
        }

        void VerifyIsPrettyClose(int a, int b, string message)
        {
            // Due to rounding between float and int, just make sure the values are pretty close
            Verify.IsTrue(a <= b + 1 && a >= b - 1, message + ": expected " + a + ", actual " + b);
        }

        Button FindCloseButton(UIObject parent)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.Name.Equals("Close"))
                {
                    return new Button(elem);
                }
            }
            Log.Comment("Did not find Close button for object " + parent.Name);
            return null;
        }

        void Check(string checkboxName)
        {
            CheckBox checkBox = FindElement.ByName<CheckBox>(checkboxName);
            checkBox.Check();
            Log.Comment("Checked " + checkboxName + " checkbox");
            Wait.ForIdle();
        }

        void Uncheck(string checkboxName)
        {
            CheckBox checkBox = FindElement.ByName<CheckBox>(checkboxName);
            checkBox.Uncheck();
            Log.Comment("Unchecked " + checkboxName + " checkbox");
            Wait.ForIdle();
        }
    }
}
