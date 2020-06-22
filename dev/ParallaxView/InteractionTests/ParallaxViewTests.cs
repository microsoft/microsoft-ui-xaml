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

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class ParallaxViewTests
    {
        private enum SourceType
        {
            ScrollViewer,
            ScrollPresenter,
        }

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        [TestProperty("MinVersion", "RS1")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        // Pans a ListView with a parallaxing background Image.
        [TestMethod]
        public void TargetListViewBackground()
        {
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to ListViewBackgroundPage");
                UIObject navigateToListViewBackgroundUIObject = FindElement.ByName("navigateToListViewBackground");
                Verify.IsNotNull(navigateToListViewBackgroundUIObject, "Verifying that navigateToListViewBackground Button was found");

                Button navigateToListViewBackgroundButton = new Button(navigateToListViewBackgroundUIObject);
                navigateToListViewBackgroundButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ListView");
                UIObject ingredientListUIObject = FindElement.ByName("IngredientList");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView was found");

                Log.Comment("Panning ListView");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }

        // Pans a ListView with a parallaxing Image in each item.
        [TestMethod]
        public void TargetListViewItem()
        {
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to ListViewItemPage");
                UIObject navigateToListViewItemUIObject = FindElement.ByName("navigateToListViewItem");
                Verify.IsNotNull(navigateToListViewItemUIObject, "Verifying that navigateToListViewItem Button was found");

                Button navigateToListViewItemButton = new Button(navigateToListViewItemUIObject);
                navigateToListViewItemButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ListView");
                UIObject ingredientListUIObject = FindElement.ByName("IngredientList");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView was found");

                Log.Comment("Panning ListView");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }

        // Pans a ListView with a parallaxing Image in the header.
        [TestMethod]
        public void TargetListViewHeader()
        {
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to ListViewHeaderPage");
                UIObject navigateToListViewHeaderUIObject = FindElement.ByName("navigateToListViewHeader");
                Verify.IsNotNull(navigateToListViewHeaderUIObject, "Verifying that navigateToListViewHeader Button was found");

                Button navigateToListViewHeaderButton = new Button(navigateToListViewHeaderUIObject);
                navigateToListViewHeaderButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ListView #1");
                UIObject ingredientListUIObject = FindElement.ByName("IngredientList1");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView #1 was found");

                Log.Comment("Panning ListView #1");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Retrieving ListView #2");
                ingredientListUIObject = FindElement.ByName("IngredientList2");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView #2 was found");

                Log.Comment("Panning ListView #2");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Retrieving ListView #3");
                ingredientListUIObject = FindElement.ByName("IngredientList3");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView #3 was found");

                Log.Comment("Panning ListView #3");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }

        // Pans a ListView with a parallaxing background Image, while the ItemsPanel is a VirtualizingStackPanel.
        [TestMethod]
        public void UseVirtualizingStackPanelSource()
        {
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to VirtualizingStackPanelPage");
                UIObject navigateToVSPUIObject = FindElement.ByName("navigateToVSP");
                Verify.IsNotNull(navigateToVSPUIObject, "Verifying that navigateToVSP Button was found");

                Button navigateToVSPButton = new Button(navigateToVSPUIObject);
                navigateToVSPButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving ListView");
                UIObject ingredientListUIObject = FindElement.ByName("IngredientList");
                Verify.IsNotNull(ingredientListUIObject, "Verifying that ingredientList ListView was found");

                Log.Comment("Panning ListView");
                InputHelper.Pan(ingredientListUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }


        // Pans a TextBox with a parallaxing background Image.
        [TestMethod]
        public void UseTextControlSource()
        {
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to TextPage");
                UIObject navigateToTextUIObject = FindElement.ByName("navigateToText");
                Verify.IsNotNull(navigateToTextUIObject, "Verifying that navigateToText Button was found");

                Button navigateToTextButton = new Button(navigateToTextUIObject);
                navigateToTextButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving TextBox");
                UIObject textBoxUIObject = FindElement.ByName("textBox");
                Verify.IsNotNull(textBoxUIObject, "Verifying that textBox was found");

                Log.Comment("Retrieving RichEditBox");
                UIObject richEditBoxUIObject = FindElement.ByName("richEditBox");
                Verify.IsNotNull(richEditBoxUIObject, "Verifying that RichEditBox was found");

                Log.Comment("Tapping RichEditBox");
                InputHelper.Tap(richEditBoxUIObject);

                Log.Comment("Panning TextBox");
                InputHelper.Pan(textBoxUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }

        // Sets ParallaxView's Source and Child properties and pans the ScrollViewer source.
        [TestMethod]
        public void ChangeSourceAndTargetPropertiesUsingScrollViewer()
        {
            ChangeSourceAndTargetProperties(SourceType.ScrollViewer);
        }

        // Sets ParallaxView's Source and Child properties and pans the ScrollPresenter source.
        [TestMethod]
        public void ChangeSourceAndTargetPropertiesUsingScrollPresenter()
        {
            ChangeSourceAndTargetProperties(SourceType.ScrollPresenter);
        }

        private void ChangeSourceAndTargetProperties(SourceType sourceType)
        { 
            Log.Comment("Selecting ParallaxView tests");

            using (var setup = new TestSetupHelper("ParallaxView Tests"))
            {
                Log.Comment("Navigating to DynamicPage");
                UIObject navigateToDynamicUIObject = FindElement.ByName("navigateToDynamic");
                Verify.IsNotNull(navigateToDynamicUIObject, "Verifying that navigateToDynamic Button was found");

                Button navigateToDynamicButton = new Button(navigateToDynamicUIObject);
                navigateToDynamicButton.Invoke();
                Wait.ForIdle();

                string sourceName = sourceType == SourceType.ScrollViewer ? "scrollViewer" : "scrollPresenter";

                SelectSource(sourceName);
                Wait.ForIdle();

                SelectTarget("img");
                Wait.ForIdle();

                WriteInTextBox("txtVerticalShift", "100.0");

                Log.Comment("Retrieving SetVerticalShift Button");
                Button setVerticalShiftButton = new Button(FindElement.ByName("btnSetVerticalShift"));

                Log.Comment("Invoke SetVerticalShift Button");
                setVerticalShiftButton.Invoke();

                Log.Comment("Retrieving " + sourceName);
                UIObject sourceUIObject = FindElement.ByName(sourceName);
                Verify.IsNotNull(sourceUIObject, "Verifying that " + sourceName + " was found");

                Log.Comment("Tapping " + sourceName);
                InputHelper.Tap(sourceUIObject);

                Log.Comment("Panning " + sourceName);
                InputHelper.Pan(sourceUIObject, 50, Direction.North);
                Wait.ForIdle();

                Log.Comment("Returning to the main ParallaxView test page");
                TestSetupHelper.GoBack();
            }
        }

        private void SelectSource(string sourceName)
        {
            Log.Comment("Retrieving Source ComboBox");
            ComboBox cmbSource = new ComboBox(FindElement.ByName("cmbSource"));

            Log.Comment("Changing the Source ComboBox selection to {0}", sourceName);
            cmbSource.SelectItemByName(sourceName);

            ComboBoxItem selectedComboBoxItem = cmbSource.Selection[0];
            Log.Comment("Source ComboBox selection is now {0}", selectedComboBoxItem.Name);
        }

        private void SelectTarget(string targetName)
        {
            Log.Comment("Retrieving Target ComboBox");
            ComboBox cmbTargetElement = new ComboBox(FindElement.ByName("cmbTargetElement"));

            Log.Comment("Changing the Target ComboBox selection to {0}", targetName);
            cmbTargetElement.SelectItemByName(targetName);

            ComboBoxItem selectedComboBoxItem = cmbTargetElement.Selection[0];
            Log.Comment("Target ComboBox selection is now {0}", selectedComboBoxItem.Name);
        }

        private void WriteInTextBox(string textBoxName, string text)
        {
            Log.Comment("Retrieve text box with name '{0}'.", textBoxName);
            Edit textBox = new Edit(FindElement.ById(textBoxName));

            KeyboardHelper.EnterText(textBox, text);
        }
    }
}
