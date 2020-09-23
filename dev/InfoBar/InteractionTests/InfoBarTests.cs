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
                //Log.Comment("Found events listbox with " + events.Items.Count + " items");
                Verify.AreEqual("CloseButtonClick", events.Items[0].Name, "First event should be the CloseButtonClick event");
                Verify.AreEqual("Closing: CloseButton", events.Items[1].Name, "Second event should be the Closing event, reason=CloseButton");
                Verify.AreEqual("Closed: CloseButton", events.Items[2].Name, "Third event should be the Closed event, reason=CloseButton");

                // ### PICK UP HERE
            }
        }

        // ### can I just make a common test library for some of these?

        void VerifyListBoxContents(string[] items)
        {

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
