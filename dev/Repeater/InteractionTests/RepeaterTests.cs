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

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Patterns;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class RepeaterTests
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
        public void VerifyUniformGridLayoutDoesNotCreateHoles()
        {
            using (var setup = new TestSetupHelper("ItemsRepeater Tests"))
            {
                // Open page
                FindElement.ByName("UniformGridLayoutDemo").Click();
                
                // Scroll down
                var scrollviewer = FindElement.ByName("RepeaterScrollViewer");
                var repeaterHeightButton = FindElement.ByName("GetRepeaterActualHeightButton");
                repeaterHeightButton.Click();
                // Get actual repeater height
                var oldActualRepeaterHeight = double.Parse(FindElement.ByName("RepeaterActualHeightLabel").GetText());

                InputHelper.RotateWheel(scrollviewer, -2000);
                Wait.ForIdle();

                // Scroll back up again, users don't scroll large offsets at once but rather small chunks in succession.
                for(int i=0;i<5;i++)
                {
                    InputHelper.RotateWheel(scrollviewer, 50);
                    Wait.ForIdle();
                }

                repeaterHeightButton.Click();
                Verify.IsTrue(Math.Abs(oldActualRepeaterHeight - 
                    double.Parse(FindElement.ByName("RepeaterActualHeightLabel").GetText())) < 1,
                    "Repeater heights did not match. This indicates that there are holes in layout as the repeater now needed more/less space.");
            }
        }

        [TestMethod]
        public void VerifyUniformGridLayoutScrolling()
        {
            using (var setup = new TestSetupHelper("ItemsRepeater Tests"))
            {
                // Open page
                FindElement.ByName("UniformGridLayoutDemo").Click();

                // Set ScrollViewer HorizontalScrollBarVisibility to Disabled.
                Log.Comment("Retrieving CmbScrollViewerHorizontalScrollBarVisibility");
                UIObject CmbScrollViewerHorizontalScrollBarVisibilityUIObject = FindElement.ByName("CmbScrollViewerHorizontalScrollBarVisibility");
                ComboBox CmbScrollViewerHorizontalScrollBarVisibility = new ComboBox(CmbScrollViewerHorizontalScrollBarVisibilityUIObject);
                Verify.IsNotNull(CmbScrollViewerHorizontalScrollBarVisibility, "Verifying that CmbScrollViewerHorizontalScrollBarVisibility ComboBox was found");

                Log.Comment("Changing ScrollViewer HorizontalScrollBarVisibility to Disabled");
                CmbScrollViewerHorizontalScrollBarVisibility.SelectItemByName("Disabled");
                Log.Comment("Selection is now {0}", CmbScrollViewerHorizontalScrollBarVisibility.Selection[0].Name);

                // Set ScrollViewer MaxWidth to 500.
                Log.Comment("Retrieving ScrollViewerMaxWidth");
                UIObject ScrollViewerMaxWidthUIObject = FindElement.ByName("ScrollViewerMaxWidth");
                Edit ScrollViewerMaxWidthTextBox = new Edit(ScrollViewerMaxWidthUIObject);
                Verify.IsNotNull(ScrollViewerMaxWidthTextBox, "Verifying that ScrollViewerMaxWidth Edit was found");
                ScrollViewerMaxWidthTextBox.SetValue("500");
                Wait.ForIdle();
                Log.Comment("ScrollViewerMaxWidth: " + ScrollViewerMaxWidthTextBox.Value);

                Log.Comment("Retrieving SetScrollViewerMaxWidth");
                UIObject SetScrollViewerMaxWidthUIObject = FindElement.ByName("SetScrollViewerMaxWidth");
                Button SetScrollViewerMaxWidthButton = new Button(SetScrollViewerMaxWidthUIObject);
                Verify.IsNotNull(SetScrollViewerMaxWidthUIObject, "Verifying that SetScrollViewerMaxWidth Button was found");
                Log.Comment("Updating ScrollViewerMaxWidth");
                SetScrollViewerMaxWidthButton.Invoke();
                Wait.ForIdle();

                // Set UniformGridLayout ItemsStretch to Fill.
                Log.Comment("Retrieving CmbUniformGridLayoutItemsStretch");
                UIObject CmbUniformGridLayoutItemsStretchUIObject = FindElement.ByName("CmbUniformGridLayoutItemsStretch");
                ComboBox CmbUniformGridLayoutItemsStretch = new ComboBox(CmbUniformGridLayoutItemsStretchUIObject);
                Verify.IsNotNull(CmbUniformGridLayoutItemsStretch, "Verifying that CmbUniformGridLayoutItemsStretch ComboBox was found");

                Log.Comment("Changing UniformGridLayout ItemsStretch to Fill");
                CmbUniformGridLayoutItemsStretch.SelectItemByName("Fill");
                Log.Comment("Selection is now {0}", CmbUniformGridLayoutItemsStretch.Selection[0].Name);

                // Set UniformGridLayout MinColumnSpacing to 50.
                Log.Comment("Retrieving UniformGridLayoutMinColumnSpacing");
                UIObject UniformGridLayoutMinColumnSpacingUIObject = FindElement.ByName("UniformGridLayoutMinColumnSpacing");
                Edit UniformGridLayoutMinColumnSpacingTextBox = new Edit(UniformGridLayoutMinColumnSpacingUIObject);
                Verify.IsNotNull(UniformGridLayoutMinColumnSpacingTextBox, "Verifying that UniformGridLayoutMinColumnSpacing Edit was found");
                UniformGridLayoutMinColumnSpacingTextBox.SetValue("50");
                Wait.ForIdle();
                Log.Comment("UniformGridLayoutMinColumnSpacing: " + UniformGridLayoutMinColumnSpacingTextBox.Value);

                Log.Comment("Retrieving SetUniformGridLayoutMinColumnSpacing");
                UIObject SetUniformGridLayoutMinColumnSpacingUIObject = FindElement.ByName("SetUniformGridLayoutMinColumnSpacing");
                Button SetUniformGridLayoutMinColumnSpacingButton = new Button(SetUniformGridLayoutMinColumnSpacingUIObject);
                Verify.IsNotNull(SetUniformGridLayoutMinColumnSpacingUIObject, "Verifying that SetUniformGridLayoutMinColumnSpacing Button was found");
                Log.Comment("Updating UniformGridLayoutMinColumnSpacing");
                SetUniformGridLayoutMinColumnSpacingButton.Invoke();
                Wait.ForIdle();

                // Set ScrollViewer VerticalOffset to 1000.
                Log.Comment("Retrieving ScrollViewerVerticalOffset");
                UIObject ScrollViewerVerticalOffsetUIObject = FindElement.ByName("ScrollViewerVerticalOffset");
                Edit ScrollViewerVerticalOffsetTextBox = new Edit(ScrollViewerVerticalOffsetUIObject);
                Verify.IsNotNull(ScrollViewerVerticalOffsetTextBox, "Verifying that ScrollViewerVerticalOffset Edit was found");
                ScrollViewerVerticalOffsetTextBox.SetValue("1000");
                Wait.ForIdle();
                Log.Comment("ScrollViewerVerticalOffset: " + ScrollViewerVerticalOffsetTextBox.Value);

                Log.Comment("Retrieving SetScrollViewerVerticalOffset");
                UIObject SetScrollViewerVerticalOffsetUIObject = FindElement.ByName("SetScrollViewerVerticalOffset");
                Button SetScrollViewerVerticalOffsetButton = new Button(SetScrollViewerVerticalOffsetUIObject);
                Verify.IsNotNull(SetScrollViewerVerticalOffsetUIObject, "Verifying that SetScrollViewerVerticalOffset Button was found");
                Log.Comment("Scrolling to ScrollViewer VerticalOffset 1000 by invoking SetScrollViewerVerticalOffset Button");
                SetScrollViewerVerticalOffsetButton.Invoke();
                Wait.ForIdle();

                Log.Comment("Retrieving GetScrollViewerVerticalOffset");
                UIObject GetScrollViewerVerticalOffsetUIObject = FindElement.ByName("GetScrollViewerVerticalOffset");
                Button GetScrollViewerVerticalOffsetButton = new Button(GetScrollViewerVerticalOffsetUIObject);
                Verify.IsNotNull(GetScrollViewerVerticalOffsetUIObject, "Verifying that GetScrollViewerVerticalOffset Button was found");
                Log.Comment("Retrieving ScrollViewer VerticalOffset by invoking GetScrollViewerVerticalOffset Button");
                GetScrollViewerVerticalOffsetButton.Invoke();
                Wait.ForIdle();
                Log.Comment("ScrollViewerVerticalOffset: " + ScrollViewerVerticalOffsetTextBox.Value);

                Verify.AreEqual("1000", ScrollViewerVerticalOffsetTextBox.Value, "Verifying that final ScrollViewer VerticalOffset is requested 1000.");
            }
        }

        [TestMethod]
        public void InsertAtStartBehavior()
        {
            using (var setup = new TestSetupHelper(new[] { "ItemsRepeater Tests" }))
            {
                FindElement.ByName("Basic Demo").Click();
                var addItemButton = FindElement.ByName("InsertAtStartButton");
                var itemCountLabel = FindElement.ByName("InsertAtStartChildCountLabel");

                for (int i = 0; i < 10; i++)
                {
                    // For performance reasons, invoking the button also reevaluates the children count.
                    // Technically there are i+1 children, but the button is one item behind.
                    // Since i starts at 0, everything lines up correctly in here and we don't have to invoke two buttons.
                    addItemButton.Click();
                    Wait.ForIdle();
                    Verify.AreEqual(i.ToString(), itemCountLabel.GetText());
                }
            }
        }
    }
}
