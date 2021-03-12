// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class CommonStylesTests
    { 
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
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
        public void SliderDensityTest()
        {
            RunDensityTests("SliderDensityTest");
        }

        [TestMethod]
        public void ToggleSwitchDensityTest()
        {
            RunDensityTests("ToggleSwitchDensityTest");
        }

        [TestMethod]
        public void DatePickerDensityTest()
        {
            RunDensityTests("DatePickerDensityTest");
        }

        [TestMethod]
        public void TimePickerDensityTest()
        {
            RunDensityTests("TimePickerDensityTest");
        }

        [TestMethod]
        public void AutoSuggestBoxDensityTest()
        {
            RunDensityTests("AutoSuggestBoxDensityTest");
        }

        [TestMethod]
        public void ListViewItemDensityTest()
        {
            RunDensityTests("ListViewItemDensityTest");
        }

        [TestMethod]
        public void TextBoxDensityTest()
        {
            RunDensityTests("TextBoxDensityTest");
        }

        [TestMethod]
        public void PasswordBoxDensityTest()
        {
            RunDensityTests("PasswordBoxDensityTest");
        }

        [TestMethod]
        public void ComboBoxDensityTest()
        {
            RunDensityTests("ComboBoxDensityTest");
        }

        [TestMethod]
        public void RichEditBoxDensityTest()
        {
            RunDensityTests("RichEditBoxDensityTest");
        }

        [TestMethod]
        public void AppBarToggleButtonDensityTest()
        {
            RunDensityTests("AppBarToggleButtonDensityTest");
        }

        [TestMethod]
        public void AppBarButtonDensityTest()
        {
            RunDensityTests("AppBarButtonDensityTest");
        }

        private void RunDensityTests(string buttonName)       
        {
            using (var setup = new TestSetupHelper("CommonStyles Tests"))
            {
                Log.Comment("Click on " + buttonName);
                var button = new Button(FindElement.ByName(buttonName));
                button.Invoke();
                Wait.ForIdle();

                var densityTestResult = new TextBlock(FindElement.ByName("DensityTestResult")).GetText();
                Verify.AreEqual(densityTestResult, "Pass", "We expect density test result is Pass");
            }
        }

        [TestMethod]
        public void RunCompactTests()        
        {
            using (var setup = new TestSetupHelper("Compact Tests"))
            {
                Log.Comment("Click on RunTest");
                var button = new Button(FindElement.ByName("RunTest"));
                button.Invoke();
                Wait.ForIdle();

                var testResult = new TextBlock(FindElement.ById("CompactTestResult")).GetText();
                Verify.AreEqual(testResult, "Pass", "We expect compact test result is Pass"); // "Pass" string matches value used by MUXControlsTestApp.SimpleVerify
            }
        }

        [TestMethod]
        public void CornerRadiusTest()
        {
            using (var setup = new TestSetupHelper("CornerRadius Tests"))
            {
                var textBlock = FindElement.ByName("CornerRadius");
                Verify.IsNotNull(textBlock, "Verify corner radius page doesn't crash");
            }
        }
    }
}
