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
    public class NumberBoxTests
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
        public void UpDownTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                RangeValueSpinner numBox = FindElement.ByName<RangeValueSpinner>("TestNumberBox");
                Verify.AreEqual(0, numBox.Value);

                ComboBox spinModeComboBox = FindElement.ByName<ComboBox>("SpinModeComboBox");
                spinModeComboBox.SelectItemByName("Inline");
                Wait.ForIdle();

                Button upButton = FindButton(numBox, "Increase");
                Button downButton = FindButton(numBox, "Decrease");

                Log.Comment("Verify that up button increases value by 1");
                upButton.InvokeAndWait();
                Verify.AreEqual(1, numBox.Value);

                Log.Comment("Verify that down button decreases value by 1");
                downButton.InvokeAndWait();
                Verify.AreEqual(0, numBox.Value);

                Log.Comment("Change Step value to 5");
                RangeValueSpinner stepNumBox = FindElement.ByName<RangeValueSpinner>("StepNumberBox");
                stepNumBox.SetValue(5);
                Wait.ForIdle();

                Log.Comment("Verify that up button increases value by 5");
                upButton.InvokeAndWait();
                Verify.AreEqual(5, numBox.Value);
            }
        }

        Button FindButton(UIObject parent, string buttonName)
        {
            foreach (UIObject elem in parent.Children)
            {
                if (elem.Name.Equals(buttonName))
                {
                    Log.Comment("Found " + buttonName + " button for object " + parent.Name);
                    return new Button(elem);
                }
            }
            Log.Comment("Did not find " + buttonName + " button for object " + parent.Name);
            return null;
        }
    }
}
