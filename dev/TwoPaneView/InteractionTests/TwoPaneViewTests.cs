// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;

using Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.Foundation.Metadata;

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
    public class TwoPaneViewTests
    {
        // Need to be the same as c_defaultMinWideModeWidth/c_defaultMinTallModeHeight in TwoPaneViewFactory.cpp
        private const double c_defaultMinWideModeWidth = 641.0;
        private const double c_defaultMinTallModeHeight = 641.0;

        // Need to be the same as c_simulatedPaneWidth/c_simulatedPaneHeight/c_simulatedMiddle in TwoPaneViewPage.xaml.cs
        private const double c_simulatedPaneWidth = 300.0;
        private const double c_simulatedPaneHeight = 400.0;
        private const double c_simulatedMiddle = 12.0;

        // Need to be the same as c_controlMargin in TwoPaneViewPage.xaml.cs
        private const double c_controlMargin_left = 40.0;
        private const double c_controlMargin_top = 10.0;
        private const double c_controlMargin_right = 30.0;
        private const double c_controlMargin_bottom = 20.0;

        enum ControlWidth { Default, Wide, Narrow }
        enum ControlHeight { Default, Tall, Short }
        enum WideModeConfiguration { LeftRight, RightLeft, SinglePane }
        enum TallModeConfiguration { TopBottom, BottomTop, SinglePane }
        enum PanePriority { Pane1, Pane2 }
        enum TwoPaneViewMode { SinglePane, Wide, Tall }

        enum ViewMode { Pane1Only, Pane2Only, LeftRight, RightLeft, TopBottom, BottomTop }

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
        public void ViewModeTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                SetControlWidth(ControlWidth.Wide);
                SetControlHeight(ControlHeight.Tall);

                Wait.ForSeconds(5);

                VerifyViewMode(ViewMode.LeftRight);

                Log.Comment("Verify changing wide behavior splits right/left");
                SetWideModeConfiguration(WideModeConfiguration.RightLeft);
                VerifyViewMode(ViewMode.RightLeft);

                Log.Comment("Verify narrow width splits top/bottom");
                SetControlWidth(ControlWidth.Narrow);
                VerifyViewMode(ViewMode.TopBottom);

                Log.Comment("Verify changing tall behavior splits bottom/top");
                SetTallModeConfiguration(TallModeConfiguration.BottomTop);
                VerifyViewMode(ViewMode.BottomTop);

                Log.Comment("Verify short height shows only priority pane");
                SetControlHeight(ControlHeight.Short);
                VerifyViewMode(ViewMode.Pane1Only);

                Log.Comment("Verify changing priority switches panes");
                SetPanePriority(PanePriority.Pane2);
                VerifyViewMode(ViewMode.Pane2Only);

                Log.Comment("Verify tall height with span shows only priority pane");
                SetControlHeight(ControlHeight.Tall);
                VerifyViewMode(ViewMode.BottomTop);
                SetTallModeConfiguration(TallModeConfiguration.SinglePane);
                VerifyViewMode(ViewMode.Pane2Only);

                Log.Comment("Verify wide width with span shows only priority pane");
                SetControlWidth(ControlWidth.Wide);
                VerifyViewMode(ViewMode.RightLeft);
                SetWideModeConfiguration(WideModeConfiguration.SinglePane);
                VerifyViewMode(ViewMode.Pane2Only);

                Log.Comment("Verify changing priority switches panes");
                SetPanePriority(PanePriority.Pane1);
                VerifyViewMode(ViewMode.Pane1Only);
            }
        }

        [TestMethod]
        public void ThresholdTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                SetControlWidth(ControlWidth.Wide);
                SetControlHeight(ControlHeight.Tall);

                Log.Comment("Verify changing min wide width updates view mode");
                SetMinWideModeWidth(c_defaultMinWideModeWidth + 100);
                VerifyViewMode(ViewMode.TopBottom);

                Log.Comment("Verify changing min tall height updates view mode");
                SetMinTallModeHeight(c_defaultMinTallModeHeight + 100);
                VerifyViewMode(ViewMode.Pane1Only);

                Log.Comment("Verify changing min wide width updates view mode");
                SetMinWideModeWidth(c_defaultMinWideModeWidth - 100);
                VerifyViewMode(ViewMode.LeftRight);
            }
        }

        [TestMethod]
        public void RegionTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                Log.Comment("Verify horizontal split regions");
                SetComboBox("SimulateComboBox", "LeftRight");

                VerifyViewMode(ViewMode.LeftRight);
                VerifyPaneSize(1, c_simulatedPaneWidth, c_simulatedPaneHeight);
                VerifyPaneSize(2, c_simulatedPaneWidth, c_simulatedPaneHeight);
                VerifyPaneSpacing(c_simulatedMiddle);

                Log.Comment("Verify vertical split regions");
                SetComboBox("SimulateComboBox", "TopBottom");

                VerifyViewMode(ViewMode.TopBottom);
                VerifyPaneSize(1, c_simulatedPaneHeight, c_simulatedPaneWidth);
                VerifyPaneSize(2, c_simulatedPaneHeight, c_simulatedPaneWidth);
                VerifyPaneSpacing(c_simulatedMiddle);
            }
        }

        [TestMethod]
        public void RegionOffsetTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                Log.Comment("Verify horizontal split regions with control offset from simulated window");
                SetComboBox("SimulateComboBox", "LeftRight");

                CheckBox marginCheckbox = new CheckBox(FindElement.ByName("AddMarginCheckBox"));
                marginCheckbox.Check();
                Wait.ForIdle();

                VerifyViewMode(ViewMode.LeftRight);
                VerifyPaneSize(1, c_simulatedPaneWidth - c_controlMargin_left,  c_simulatedPaneHeight - (c_controlMargin_top + c_controlMargin_bottom));
                VerifyPaneSize(2, c_simulatedPaneWidth - c_controlMargin_right, c_simulatedPaneHeight - (c_controlMargin_top + c_controlMargin_bottom));
                VerifyPaneSpacing(c_simulatedMiddle);

                Log.Comment("Verify vertical split regions with control offset from simulated window");
                SetComboBox("SimulateComboBox", "TopBottom");

                VerifyViewMode(ViewMode.TopBottom);
                VerifyPaneSize(1, c_simulatedPaneHeight - (c_controlMargin_left + c_controlMargin_right), c_simulatedPaneWidth - c_controlMargin_top);
                VerifyPaneSize(2, c_simulatedPaneHeight - (c_controlMargin_left + c_controlMargin_right), c_simulatedPaneWidth - c_controlMargin_bottom);
                VerifyPaneSpacing(c_simulatedMiddle);
            }
        }

        [TestMethod]
        public void SingleRegionTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                Log.Comment("Verify control acts appropriately when there are multiple regions but the control is only in one of them");

                SetComboBox("SimulateComboBox", "LeftRight");

                CheckBox marginCheckbox = new CheckBox(FindElement.ByName("OneSideCheckBox"));
                marginCheckbox.Check();
                Wait.ForIdle();

                VerifyViewMode(ViewMode.Pane1Only);
                VerifyPaneSize(1, c_simulatedPaneWidth, c_simulatedPaneHeight);

                Log.Comment("Verify vertical split when control is in a single region");
                SetMinTallModeHeight(c_simulatedPaneHeight - 10);

                VerifyViewMode(ViewMode.TopBottom);
                VerifyPaneSpacing(0);
            }
        }

        [TestMethod]
        public void InitialPanePriorityTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                Log.Comment("Verify when pane priority is set to pane 2 on the small split panel, it only loads pane 2 (bug 14486142)");

                UIObject paneContent1 = TryFindElement.ByName("SmallContent1");
                UIObject paneContent2 = TryFindElement.ByName("SmallContent2");

                Verify.IsNull(paneContent1, "Expected not to find small pane 1");
                Verify.IsNotNull(paneContent2, "Expected to find small pane2");
            }
        }

        [TestMethod]
        public void PaneLengthTest()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                SetControlWidth(ControlWidth.Wide);
                SetControlHeight(ControlHeight.Tall);

                int controlWidth = GetInt("ControlWidthText");
                int controlHeight = GetInt("ControlHeightText");
                Log.Comment("TwoPaneView size is " + controlWidth + ", " + controlHeight);

                Log.Comment("Verify changing pane 1 length to star sizing");
                SetLength(1, 1, "Star");

                VerifyPaneSize(1, controlWidth / 2.0, 0);
                VerifyPaneSize(2, controlWidth / 2.0, 0);

                Log.Comment("Verify changing pane 2 length to pixel sizing");
                SetLength(2, 199, "Pixel");

                VerifyPaneSize(1, controlWidth - 199, 0);
                VerifyPaneSize(2, 199, 0);

                Log.Comment("Verify column sizes stay the same when switching pane order");
                SetWideModeConfiguration(WideModeConfiguration.RightLeft);

                VerifyPaneSize(1, controlWidth - 199, 0);
                VerifyPaneSize(2, 199, 0);

                Log.Comment("Verify lengths apply to top/bottom configuration");
                SetControlWidth(ControlWidth.Narrow);

                VerifyPaneSize(1, 0, controlHeight - 199);
                VerifyPaneSize(2, 0, 199);
            }
        }


        // Verify that tabbing around puts us where we expect
        [TestMethod]
        public void VerifyTabKeyWorks()
        {
            using (var setup = new TestSetupHelper("TwoPaneView Tests")) // This clicks the button corresponding to the test page.
            {
                UIObject twoPaneView = TryFindElement.ByName("TwoPaneViewLarge");
                Verify.IsNotNull(twoPaneView, "TwoPaneView is present");
                twoPaneView.SetFocus();
                Wait.ForIdle();
                Verify.AreNotEqual(UIObject.Focused, twoPaneView, "TwoPaneView should not be focused");
                KeyboardHelper.PressKey(Key.Tab);
                Verify.AreNotEqual(UIObject.Focused, twoPaneView, "TwoPaneView should not be focused");
                KeyboardHelper.PressKey(Key.Tab, ModifierKey.Shift);
                Verify.AreNotEqual(UIObject.Focused, twoPaneView, "TwoPaneView should not be focused");
            }
        }

        private void VerifyViewMode(ViewMode mode)
        {
            // Verify configuration is correct for mode
            TwoPaneViewMode expectedConfiguration = TwoPaneViewMode.SinglePane;
            switch (mode)
            {
                case ViewMode.LeftRight:
                case ViewMode.RightLeft:
                    expectedConfiguration = TwoPaneViewMode.Wide;
                    break;

                case ViewMode.TopBottom:
                case ViewMode.BottomTop:
                    expectedConfiguration = TwoPaneViewMode.Tall;
                    break;
            }

            TextBlock configurationTextBlock = new TextBlock(FindElement.ByName("ConfigurationTextBlock"));
            Verify.AreEqual(expectedConfiguration.ToString(), configurationTextBlock.DocumentText);

            // Verify panes are actually being shown correctly
            ElementCache.Clear();
            UIObject paneContent1 = TryFindElement.ByName("Content1");
            UIObject paneContent2 = TryFindElement.ByName("Content2");

            if (mode != ViewMode.Pane2Only)
            {
                Verify.IsNotNull(paneContent1, "Expected to find pane1");
                Log.Comment("Content 1 dimensions: " + paneContent1.BoundingRectangle.ToString());
            }

            if (mode != ViewMode.Pane1Only)
            {
                Verify.IsNotNull(paneContent2, "Expected to find pane2");
                Log.Comment("Content 2 dimensions: " + paneContent2.BoundingRectangle.ToString());
            }

            if (mode == ViewMode.Pane2Only)
            {
                Verify.IsNull(paneContent1, "Expected not to find pane1");
            }

            if (mode == ViewMode.Pane1Only)
            {
                Verify.IsNull(paneContent2, "Expected not to find pane2");
            }

            switch (mode)
            {
                case ViewMode.LeftRight:
                case ViewMode.RightLeft:
                    Verify.AreEqual(paneContent1.BoundingRectangle.Top, paneContent2.BoundingRectangle.Top, "Verify panes are horizontally aligned");
                    if (mode == ViewMode.LeftRight) Verify.IsGreaterThanOrEqual(paneContent2.BoundingRectangle.Left, paneContent1.BoundingRectangle.Right, "Verify left/right pane placement");
                    else Verify.IsGreaterThanOrEqual(paneContent1.BoundingRectangle.Left, paneContent2.BoundingRectangle.Right, "Verify right/left pane placement");
                    break;

                case ViewMode.TopBottom:
                case ViewMode.BottomTop:
                    Verify.AreEqual(paneContent1.BoundingRectangle.Left, paneContent2.BoundingRectangle.Left, "Verify panes are vertically aligned");
                    if (mode == ViewMode.TopBottom) Verify.IsGreaterThanOrEqual(paneContent2.BoundingRectangle.Top, paneContent1.BoundingRectangle.Bottom, "Verify top/bottom pane placement");
                    else Verify.IsGreaterThanOrEqual(paneContent1.BoundingRectangle.Top, paneContent2.BoundingRectangle.Bottom, "Verify bottom/top pane placement");
                    break;
            }
        }

        private void VerifyPaneSize(int paneIndex, double width, double height)
        {
            if (width > 0)
            {
                VerifyIsPrettyClose((int)width, GetInt("WidthText" + paneIndex), "Verify pane" + paneIndex + " width");
            }
            if (height > 0)
            {
                VerifyIsPrettyClose((int)height, GetInt("HeightText" + paneIndex), "Verify pane" + paneIndex + " height");
            }
        }

        private void VerifyPaneSpacing(double spacing)
        {
            VerifyIsPrettyClose((int)spacing, GetInt("SpacingTextBox" ), "Verify spacing");
        }

        private void VerifyIsPrettyClose(int a, int b, string info)
        {
            // Due to MITA only reporting whole numbers, and some rounding on phone builds, just make sure widths/heights are pretty close.
            Verify.IsTrue(a <= b + 1 && a >= b - 1, info + ": expected " + a + ", actual " + b);
        }

        private int GetInt(string textBlockName)
        {
            TextBlock tb = new TextBlock(FindElement.ByName(textBlockName));
            Log.Comment("Parsing string '" + tb.DocumentText + "' into an int");
            return int.Parse(tb.DocumentText);
        }

        private void SetControlWidth(ControlWidth width)
        {
            SetComboBox("WidthComboBox", width.ToString());
        }

        private void SetControlHeight(ControlHeight height)
        {
            SetComboBox("HeightComboBox", height.ToString());
        }

        private void SetWideModeConfiguration(WideModeConfiguration behavior)
        {
            SetComboBox("WideModeConfigurationComboBox", behavior.ToString());
        }

        private void SetTallModeConfiguration(TallModeConfiguration behavior)
        {
            SetComboBox("TallModeConfigurationComboBox", behavior.ToString());
        }

        private void SetPanePriority(PanePriority priority)
        {
            SetComboBox("PanePriorityComboBox", priority.ToString());
        }

        private void SetMinWideModeWidth(double width)
        {
            Log.Comment("Setting min wide width to " + width);
            Edit widthTextBox = new Edit(FindElement.ByName("MinWideModeWidthTextBox"));
            widthTextBox.SetValue(width.ToString());
            Wait.ForIdle();
        }

        private void SetMinTallModeHeight(double height)
        {
            Log.Comment("Setting min tall height to " + height);
            Edit heightTextBox = new Edit(FindElement.ByName("MinTallModeHeightTextBox"));
            heightTextBox.SetValue(height.ToString());
            Wait.ForIdle();
        }

        private void SetComboBox(string comboBoxName, string item)
        {
            Log.Comment("Setting '" + comboBoxName + "' to '" + item + "'");
            ComboBox comboBox = new ComboBox(FindElement.ByName(comboBoxName));
            comboBox.SelectItemByName(item);
            Wait.ForIdle();
        }

        private void SetLength(int pane, double value, string type)
        {
            Log.Comment("Setting pane " + pane + " length to " + value + ", " + type.ToString());

            Edit valueTextBox = new Edit(FindElement.ByName("Pane" + pane + "LengthTextBox"));
            valueTextBox.SetValue(value.ToString());

            SetComboBox("Pane" + pane + "LengthComboBox", type);

            Wait.ForIdle();
        }
    }
}
