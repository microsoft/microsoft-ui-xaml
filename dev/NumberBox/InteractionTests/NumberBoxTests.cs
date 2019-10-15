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

        public string GetTextBoxString()
        {
            UIObject numberBox = FindElement.ByName("TestNumberBox");
            foreach (UIObject elem in numberBox.Children)
            {
                if (elem.ClassName.Equals("TextBox"))
                {
                    return elem.GetText();
                }
            }
            return null;
        }

        [TestMethod]
        public void DefaultStateTest()
        {
            using (var setup = new TestSetupHelper("NumberBox Tests"))
            {
                Log.Comment("Verify NumberBox exists");
                UIObject Numbox = FindElement.ByName("TestNumberBox");
                Verify.IsNotNull(Numbox);
                Log.Comment("NumberBox Exists");

                Log.Comment("Verifying Default Text is 0");
                Verify.AreEqual("0", GetTextBoxString());
                Log.Comment("Text is 0");
            }
        }
    }
}
