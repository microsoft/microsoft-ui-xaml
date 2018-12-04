// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS || USING_TESTNET
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
    public class ScrollBar2TestsWithInputHelper
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
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
        [TestProperty("Description", "Sets various ScrollBar2 properties and invokes SetValues.")]
        public void ExerciseScrollBar2APIs()
        {
            Log.Comment("Selecting ScrollBar2 tests");

            using (var setup = new TestSetupHelper("ScrollBar2 Tests"))
            {
                Log.Comment("Navigating to ScrollBar2DynamicPage");
                UIObject navigateToDynamicUIObject = FindElement.ByName("navigateToDynamic");
                Verify.IsNotNull(navigateToDynamicUIObject, "Verifying that navigateToDynamic Button was found");

                Button navigateToDynamicButton = new Button(navigateToDynamicUIObject);
                navigateToDynamicButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving cmbIndicatorMode");
                ComboBox cmbIndicatorMode = new ComboBox(FindElement.ByName("cmbIndicatorMode"));
                Verify.IsNotNull(cmbIndicatorMode, "Verifying that cmbIndicatorMode was found");

                Log.Comment("Changing IndicatorMode to MouseIndicator");
                cmbIndicatorMode.SelectItemByName("MouseIndicator");
                Log.Comment("Selection is now {0}", cmbIndicatorMode.Selection[0].Name);

                Log.Comment("Retrieving btnSetIndicatorMode");
                UIObject btnSetIndicatorModeUIObject = FindElement.ByName("btnSetIndicatorMode");
                Verify.IsNotNull(btnSetIndicatorModeUIObject, "Verifying that btnSetIndicatorMode Button was found");

                Button btnSetIndicatorMode = new Button(btnSetIndicatorModeUIObject);
                Log.Comment("Tapping btnSetIndicatorMode");
                InputHelper.Tap(btnSetIndicatorMode);

                Log.Comment("Retrieving cmbScrollMode");
                ComboBox cmbScrollMode = new ComboBox(FindElement.ByName("cmbScrollMode"));
                Verify.IsNotNull(cmbScrollMode, "Verifying that cmbScrollMode was found");

                Log.Comment("Changing ScrollMode to Enabled");
                cmbScrollMode.SelectItemByName("Enabled");
                Log.Comment("Selection is now {0}", cmbScrollMode.Selection[0].Name);

                Log.Comment("Retrieving btnSetScrollMode");
                UIObject btnSetScrollModeUIObject = FindElement.ByName("btnSetScrollMode");
                Verify.IsNotNull(btnSetScrollModeUIObject, "Verifying that btnSetScrollMode Button was found");

                Button btnSetScrollMode = new Button(btnSetScrollModeUIObject);
                Log.Comment("Tapping btnSetScrollMode");
                InputHelper.Tap(btnSetScrollMode);

                Log.Comment("Retrieving cmbStyle");
                ComboBox cmbStyle = new ComboBox(FindElement.ByName("cmbStyle"));
                Verify.IsNotNull(cmbStyle, "Verifying that cmbStyle was found");

                Log.Comment("Changing Style to Hair");
                cmbStyle.SelectItemByName("Hair");
                Log.Comment("Selection is now {0}", cmbStyle.Selection[0].Name);

                Log.Comment("Retrieving btnSetStyle");
                UIObject btnSetStyleUIObject = FindElement.ByName("btnSetStyle");
                Verify.IsNotNull(btnSetStyleUIObject, "Verifying that btnSetStyle Button was found");

                Button btnSetStyle = new Button(btnSetStyleUIObject);
                Log.Comment("Tapping btnSetStyle");
                InputHelper.Tap(btnSetStyle);

                Log.Comment("Retrieving cmbOrientation");
                ComboBox cmbOrientation = new ComboBox(FindElement.ByName("cmbOrientation"));
                Verify.IsNotNull(cmbOrientation, "Verifying that cmbOrientation was found");

                Log.Comment("Changing Orientation to Horizontal");
                cmbOrientation.SelectItemByName("Horizontal");
                Log.Comment("Selection is now {0}", cmbOrientation.Selection[0].Name);

                Log.Comment("Retrieving btnSetOrientation");
                UIObject btnSetOrientationUIObject = FindElement.ByName("btnSetOrientation");
                Verify.IsNotNull(btnSetOrientationUIObject, "Verifying that btnSetOrientation Button was found");

                Button btnSetOrientation = new Button(btnSetOrientationUIObject);
                Log.Comment("Tapping btnSetOrientation");
                InputHelper.Tap(btnSetOrientation);

                Log.Comment("Retrieving btnSetValues");
                UIObject btnSetValuesUIObject = FindElement.ByName("btnSetValues");
                Verify.IsNotNull(btnSetValuesUIObject, "Verifying that btnSetValues Button was found");

                Button btnSetValues = new Button(btnSetValuesUIObject);
                Log.Comment("Tapping btnSetValues");
                InputHelper.Tap(btnSetValues);

                Log.Comment("Retrieving cmbIsEnabled");
                ComboBox cmbIsEnabled = new ComboBox(FindElement.ByName("cmbIsEnabled"));
                Verify.IsNotNull(cmbIsEnabled, "Verifying that cmbIsEnabled was found");

                Log.Comment("Changing IsEnabled to False");
                cmbIsEnabled.SelectItemByName("False");
                Log.Comment("Selection is now {0}", cmbIsEnabled.Selection[0].Name);

                Log.Comment("Retrieving btnSetIsEnabled");
                UIObject btnSetIsEnabledUIObject = FindElement.ByName("btnSetIsEnabled");
                Verify.IsNotNull(btnSetIsEnabledUIObject, "Verifying that btnSetIsEnabled Button was found");

                Button btnSetIsEnabled = new Button(btnSetIsEnabledUIObject);
                Log.Comment("Tapping btnSetIsEnabled");
                InputHelper.Tap(btnSetIsEnabled);

                Log.Comment("Changing IndicatorMode to TouchIndicator");
                cmbIndicatorMode.SelectItemByName("TouchIndicator");
                Log.Comment("Selection is now {0}", cmbIndicatorMode.Selection[0].Name);

                Log.Comment("Tapping btnSetIndicatorMode");
                InputHelper.Tap(btnSetIndicatorMode);

                Wait.ForIdle();

                Log.Comment("Returning to the main ScrollBar2 test page");
                TestSetupHelper.GoBack();
            }
        }
    }
}
