using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Common;
using System;

#if USING_TAEF
using WEX.TestExecution;
using WEX.TestExecution.Markup;
using WEX.Logging.Interop;
#else
using Microsoft.VisualStudio.TestTools.UnitTesting;
using Microsoft.VisualStudio.TestTools.UnitTesting.Logging;
#endif

#if BUILD_WINDOWS
using System.Windows.Automation;
using MS.Internal.Mita.Foundation;
using MS.Internal.Mita.Foundation.Controls;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
#endif

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TeachingTipTests
    {
        struct rect
        {
            public double x;
            public double y;
            public double width;
            public double height;
        };
        
        private struct TeachingTipTestPageElements
        {
            public ListBox lstTeachingTipEvents;

            public TextBlock effectivePlacementTextBlock;

            public Button getTargetBoundsButton;
            public TextBlock targetXOffsetTextBlock;
            public TextBlock targetYOffsetTextBlock;
            public TextBlock targetWidthTextBlock;
            public TextBlock targetHeightTextBlock;

            public CheckBox useTestWindowBoundsCheckbox;
            public Edit testWindowBoundsXTextBox;
            public Edit testWindowBoundsYTextBox;
            public Edit testWindowBoundsWidthTextBox;
            public Edit testWindowBoundsHeightTextBox;

            public TextBlock tipWidthTextBlock;

            public Edit scrollViewerOffsetTextBox;
            public Button scrollViewerOffsetButton;

            public TextBlock popupVerticalOffsetTextBlock;

            public ComboBox bleedingContentComboBox;
            public Button setBleedingContentButton;

            public ComboBox placementComboBox;
            public Button setPlacementButton;

            public ComboBox isLightDismissEnabledComboBox;
            public Button isLightDismissEnabledButton;

            public Button showButton;
            public Button closeButton;
            public CheckBox isOpenCheckBox;
            public CheckBox isIdleCheckBox;

            public Button bringIntoViewButton;
        }

        var elements;

        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("Platform", "Any")]
        [TestProperty("MUXControlsTestSuite", "SuiteB")]
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
        public void CloseReasonIsAccurate()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();
                ScrollTargetIntoView();
                OpenTeachingTip();
                CloseTeachingTipProgrammatically();
                var message0 = getMessage(0);
                Verify.IsTrue(message0.ToString().Contains("Programmatic"));

                SetBleedingContent(3);
                OpenTeachingTip();
                PressXCloseButton();
                var message2 = getMessage(2);
                var message4 = getMessage(4);
                Verify.IsTrue(message2.ToString().Contains("Close Button Clicked"));
                Verify.IsTrue(message4.ToString().Contains("CloseButton"));

                EnableLightDismiss(true);
                OpenTeachingTip();
                CloseTeachingTipProgrammatically();
                var message6 = getMessage(6);
                Verify.IsTrue(message6.ToString().Contains("Programmatic"));

                OpenTeachingTip();
                message2.Tap();
                var message7 = getMessage(7);
                Verify.IsTrue(message7.ToString().Contains("LightDismiss"));
            }
        }

        [TestMethod]
        public void TipFollowsTarget()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();

                ScrollTargetIntoView();
                OpenTeachingTip();
                double initialVerticalOffset = getVerticalOffset();
                scrollBy(10);
                Verify.IsLessThan(getVerticalOffset(), initialVerticalOffset);
                scrollBy(-20);
                Verify.IsGreaterThan(getVerticalOffset(), initialVerticalOffset);
            }
        }

        [TestMethod]
        public void AutoPlacement()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();

                ScrollTargetIntoView();
                scrollBy(10);
                var targetRect = getTargetBounds();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 304, targetRect.width + 656, targetRect.height + 608);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Top"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 300, targetRect.width + 656, targetRect.height + 608);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Bottom"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 300, targetRect.width + 656, targetRect.height + 603);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("RightEdgeAlignedTop"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 100, targetRect.width + 656, targetRect.height + 403);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("RightEdgeAlignedBottom"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 100, targetRect.width + 643, targetRect.height + 403);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("LeftEdgeAlignedBottom"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 300, targetRect.width + 643, targetRect.height + 603);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("LeftEdgeAlignedTop"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 304, targetRect.width + 348, targetRect.height + 608);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("TopEdgeAlignedLeft"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 20, targetRect.y - 304, targetRect.width + 348, targetRect.height + 608);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("TopEdgeAlignedRight"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 328, targetRect.y - 100, targetRect.width + 348, targetRect.height + 408);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("BottomEdgeAlignedLeft"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 20, targetRect.y - 100, targetRect.width + 348, targetRect.height + 408);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("BottomEdgeAlignedRight"));
                CloseTeachingTipProgrammatically();

                //remove the bleeding content;
                SetBleedingContent(3);

                useTestWindowBounds(targetRect.x - 328, targetRect.y - 100, targetRect.width + 348, targetRect.height + 20);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Left"));
                CloseTeachingTipProgrammatically();
                useTestWindowBounds(targetRect.x - 20, targetRect.y - 100, targetRect.width + 348, targetRect.height + 20);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Right"));
                CloseTeachingTipProgrammatically();
            }
        }

        [TestMethod]
        public void SpecifiedPlacement()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();

                ScrollTargetIntoView();
                scrollBy(10);

                SetPlacement(0);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Top"));
                CloseTeachingTipProgrammatically();
                
                SetPlacement(1);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Bottom"));
                CloseTeachingTipProgrammatically();

                SetPlacement(2);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Left"));
                CloseTeachingTipProgrammatically();

                SetPlacement(3);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("Right"));
                CloseTeachingTipProgrammatically();
                
                SetPlacement(4);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("TopEdgeAlignedRight"));
                CloseTeachingTipProgrammatically();

                SetPlacement(5);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("TopEdgeAlignedLeft"));
                CloseTeachingTipProgrammatically();

                SetPlacement(6);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("BottomEdgeAlignedRight"));
                CloseTeachingTipProgrammatically();

                SetPlacement(7);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("BottomEdgeAlignedLeft"));
                CloseTeachingTipProgrammatically();

                SetPlacement(8);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("LeftEdgeAlignedTop"));
                CloseTeachingTipProgrammatically();

                SetPlacement(9);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("LeftEdgeAlignedBottom"));
                CloseTeachingTipProgrammatically();

                SetPlacement(10);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("RightEdgeAlignedTop"));
                CloseTeachingTipProgrammatically();

                SetPlacement(11);
                OpenTeachingTip();
                Verify.IsTrue(getEffectivePlacement().Equals("RightEdgeAlignedBottom"));
                CloseTeachingTipProgrammatically();
            }
        }

        private void ScrollTargetIntoView()
        {
            if (elements.bringIntoViewButton == null)
            {
                Log.Comment("Find the BringTargetIntoViewButton");
                elements.bringIntoViewButton = new Button(FindElement.ByName("BringTargetIntoViewButton"));
                Verify.IsNotNull(elements.bringIntoViewButton);
            }
            elements.bringIntoViewButton.Invoke();
        }

        private void OpenTeachingTip()
        {
            if (elements.showButton == null)
            {
                Log.Comment("Find the ShowButton");
                elements.showButton = new Button(FindElement.ByName("ShowButton"));
                Verify.IsNotNull(elements.showButton);
            }
            if (elements.isOpenCheckBox == null)
            {
                Log.Comment("Find the IsOpenCheckBox");
                elements.isOpenCheckBox = new CheckBox(FindElement.ByName("IsOpenCheckBox"));
                Verify.IsNotNull(elements.isOpenCheckBox);
            }
            if (elements.isIdleCheckBox == null)
            {
                Log.Comment("Find the IsIdleCheckBox");
                elements.isIdleCheckBox = new CheckBox(FindElement.ByName("IsIdleCheckBox"));
                Verify.IsNotNull(elements.isIdleCheckBox);
            }
            if(elements.isOpenCheckBox.ToggleState != ToggleState.On)
            {
                elements.showButton.Invoke();
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    WaitForUnchecked(elements.isIdleCheckBox);
                }
                WaitForChecked(elements.isOpenCheckBox);
                WaitForChecked(elements.isIdleCheckBox);
            }
        }

        private void CloseTeachingTipProgrammatically()
        {
            if (elements.closeButton == null)
            {
                Log.Comment("Find the CloseButton");
                elements.closeButton = new Button(FindElement.ByName("CloseButton"));
                Verify.IsNotNull(elements.closeButton);
            }
            if (elements.isOpenCheckBox == null)
            {
                Log.Comment("Find the IsOpenCheckBox");
                elements.isOpenCheckBox = new CheckBox(FindElement.ByName("IsOpenCheckBox"));
                Verify.IsNotNull(elements.isOpenCheckBox);
            }
            if (elements.isIdleCheckBox == null)
            {
                Log.Comment("Find the IsIdleCheckBox");
                elements.isIdleCheckBox = new CheckBox(FindElement.ByName("IsIdleCheckBox"));
                Verify.IsNotNull(elements.isIdleCheckBox);
            }
            if (elements.isOpenCheckBox.ToggleState != ToggleState.Off)
            {
                elements.closeButton.Invoke();
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    WaitForUnchecked(elements.isIdleCheckBox);
                }
                WaitForUnchecked(elements.isOpenCheckBox);
                WaitForChecked(elements.isIdleCheckBox);
            }
        }

        private void PressXCloseButton()
        {
            if(elements.tipWidthTextBlock == null)
            {
                Log.Comment("Find the tipWidthTextBlock");
                elements.tipWidthTextBlock = new TextBlock(FindElement.ByName("TipWidth"));
                Verify.IsNotNull(elements.tipWidthTextBlock);
            }
            if (elements.isOpenCheckBox == null)
            {
                Log.Comment("Find the IsOpenCheckBox");
                elements.isOpenCheckBox = new CheckBox(FindElement.ByName("IsOpenCheckBox"));
                Verify.IsNotNull(elements.isOpenCheckBox);
            }
            if (elements.isIdleCheckBox == null)
            {
                Log.Comment("Find the IsIdleCheckBox");
                elements.isIdleCheckBox = new CheckBox(FindElement.ByName("IsIdleCheckBox"));
                Verify.IsNotNull(elements.isIdleCheckBox);
            }
            Log.Comment("Find the teachingTip");
            var teachingTip = FindElement.ByName("TeachingTip");
            Verify.IsNotNull(teachingTip);

            InputHelper.Tap(teachingTip, double.Parse(elements.tipWidthTextBlock.GetText()) + 90, 110);
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                WaitForUnchecked(elements.isIdleCheckBox);
            }
            WaitForUnchecked(elements.isOpenCheckBox);
            WaitForChecked(elements.isIdleCheckBox);
        }

        private void EnableLightDismiss(bool enable)
        {
            if (elements.isLightDismissEnabledComboBox == null)
            {
                Log.Comment("Find the IsLightDismissEnabledComboBox");
                elements.isLightDismissEnabledComboBox = new ComboBox(FindElement.ByName("IsLightDismissEnabledComboBox"));
                Verify.IsNotNull(elements.isLightDismissEnabledComboBox);
            }
            if (elements.isLightDismissEnabledButton == null)
            {
                Log.Comment("Find the IsLightDismissEnabledButton");
                elements.isLightDismissEnabledButton = new Button(FindElement.ByName("IsLightDismissEnabledButton"));
                Verify.IsNotNull(elements.isLightDismissEnabledButton);
            }

            if(enable)
            {
                elements.isLightDismissEnabledComboBox.SelectItemByName("True");
            }
            else
            {
                elements.isLightDismissEnabledComboBox.SelectItemByName("False");
            }
            elements.isLightDismissEnabledButton.Invoke();
        }

        private void SetPlacement(int dropDownValue)
        {
            if (elements.placementComboBox == null)
            {
                Log.Comment("Find the PlacementComboBox");
                elements.placementComboBox = new ComboBox(FindElement.ByName("PlacementComboBox"));
                Verify.IsNotNull(elements.placementComboBox);
            }

            if (elements.setPlacementButton == null)
            {
                Log.Comment("Find the SetPlacementButton");
                elements.setPlacementButton = new Button(FindElement.ByName("SetPlacementButton"));
                Verify.IsNotNull(elements.setPlacementButton);
            }

            switch (dropDownValue)
            {
                case 0:
                    elements.placementComboBox.SelectItemByName("Top");
                    break;
                case 1:
                    elements.placementComboBox.SelectItemByName("Bottom");
                    break;
                case 2:
                    elements.placementComboBox.SelectItemByName("Left");
                    break;
                case 3:
                    elements.placementComboBox.SelectItemByName("Right");
                    break;
                case 4:
                    elements.placementComboBox.SelectItemByName("TopEdgeAlignedRight");
                    break;
                case 5:
                    elements.placementComboBox.SelectItemByName("TopEdgeAlignedLeft");
                    break;
                case 6:
                    elements.placementComboBox.SelectItemByName("BottomEdgeAlignedRight");
                    break;
                case 7:
                    elements.placementComboBox.SelectItemByName("BottomEdgeAlignedLeft");
                    break;
                case 8:
                    elements.placementComboBox.SelectItemByName("LeftEdgeAlignedTop");
                    break;
                case 9:
                    elements.placementComboBox.SelectItemByName("LeftEdgeAlignedBottom");
                    break;
                case 10:
                    elements.placementComboBox.SelectItemByName("RightEdgeAlignedTop");
                    break;
                case 11:
                    elements.placementComboBox.SelectItemByName("RightEdgeAlignedBottom");
                    break;
                default:
                    elements.placementComboBox.SelectItemByName("Auto");
                    break;
            }
            elements.setPlacementButton.Invoke();
        }

        private void SetBleedingContent(int dropDownValue)
        {
            if (elements.bleedingContentComboBox == null)
            {
                Log.Comment("Find the BleedingContentComboBox");
                elements.bleedingContentComboBox = new ComboBox(FindElement.ByName("BleedingContentComboBox"));
                Verify.IsNotNull(elements.bleedingContentComboBox);
            }

            if (elements.setBleedingContentButton == null)
            {
                Log.Comment("Find the SetBleedingContentButton");
                elements.setBleedingContentButton = new Button(FindElement.ByName("SetBleedingContentButton"));
                Verify.IsNotNull(elements.setBleedingContentButton);
            }

            switch (dropDownValue)
            {
                case 0:
                    elements.bleedingContentComboBox.SelectItemByName("Red Square");
                    break;
                case 1:
                    elements.bleedingContentComboBox.SelectItemByName("Blue Square");
                    break;
                case 2:
                    elements.bleedingContentComboBox.SelectItemByName("Image");
                    break;
                default:
                    elements.bleedingContentComboBox.SelectItemByName("No Content");
                    break;
            }
            elements.setBleedingContentButton.Invoke();
        }

        private double GetVerticalOffset()
        {
            if (elements.popupVerticalOffsetTextBlock == null)
            {
                Log.Comment("Find the PopupVerticalOffsetTextBlock");
                elements.popupVerticalOffsetTextBlock = new TextBlock(FindElement.ByName("PopupVerticalOffsetTextBlock"));
                Verify.IsNotNull(elements.popupVerticalOffsetTextBlock);
            }
            return double.Parse(elements.popupVerticalOffsetTextBlock.GetText());
        }

        private void ScrollBy(double ammount)
        {
            if (elements.scrollViewerOffsetTextBox == null)
            {
                Log.Comment("Find the ScrollViewerOffsetTextBox");
                elements.scrollViewerOffsetTextBox = new Edit(FindElement.ByName("ScrollViewerOffsetTextBox"));
                Verify.IsNotNull(elements.scrollViewerOffsetTextBox);
            }

            if (elements.scrollViewerOffsetButton == null)
            {
                Log.Comment("Find the ScrollViewerOffsetButton");
                elements.scrollViewerOffsetButton = new Button(FindElement.ByName("ScrollViewerOffsetButton"));
                Verify.IsNotNull(elements.scrollViewerOffsetButton);
            }

            double initialOffset = double.Parse(elements.scrollViewerOffsetTextBox.GetText());
            elements.scrollViewerOffsetTextBox.SetValue((initialOffset + ammount).ToString());
            elements.scrollViewerOffsetButton.Invoke();
        }
        
        private void UseTestWindowBounds(double x, double y, double width, double height)
        {
            if (elements.useTestWindowBoundsCheckbox == null)
            {
                Log.Comment("Find the UseTestWindowBoundsCheckbox");
                elements.useTestWindowBoundsCheckbox = new CheckBox(FindElement.ByName("UseTestWindowBoundsCheckbox"));
                Verify.IsNotNull(elements.useTestWindowBoundsCheckbox);
            }

            if (elements.testWindowBoundsXTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsXTextBox");
                elements.testWindowBoundsXTextBox = new Edit(FindElement.ByName("TestWindowBoundsXTextBox"));
                Verify.IsNotNull(elements.testWindowBoundsXTextBox);
            }

            if (elements.testWindowBoundsYTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsYTextBox");
                elements.testWindowBoundsYTextBox = new Edit(FindElement.ByName("TestWindowBoundsYTextBox"));
                Verify.IsNotNull(elements.testWindowBoundsYTextBox);
            }

            if (elements.testWindowBoundsWidthTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsWidthTextBox");
                elements.testWindowBoundsWidthTextBox = new Edit(FindElement.ByName("TestWindowBoundsWidthTextBox"));
                Verify.IsNotNull(elements.testWindowBoundsWidthTextBox);
            }

            if (elements.testWindowBoundsHeightTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsHeightTextBox");
                elements.testWindowBoundsHeightTextBox = new Edit(FindElement.ByName("TestWindowBoundsHeightTextBox"));
                Verify.IsNotNull(elements.testWindowBoundsHeightTextBox);
            }

            elements.testWindowBoundsXTextBox.SetValue(x.ToString());
            elements.testWindowBoundsYTextBox.SetValue(y.ToString());
            elements.testWindowBoundsWidthTextBox.SetValue(width.ToString());
            elements.testWindowBoundsHeightTextBox.SetValue(height.ToString());

            elements.useTestWindowBoundsCheckbox.Uncheck();
            elements.useTestWindowBoundsCheckbox.Check();
        }

        private rect GetTargetBounds()
        {
            if (elements.getTargetBoundsButton == null)
            {
                Log.Comment("Find the GetTargetBoundsButton");
                elements.getTargetBoundsButton = new Button(FindElement.ByName("GetTargetBoundsButton"));
                Verify.IsNotNull(elements.getTargetBoundsButton);
            }

            if (elements.targetXOffsetTextBlock == null)
            {
                Log.Comment("Find the TargetXOffsetTextBlock");
                elements.targetXOffsetTextBlock = new TextBlock(FindElement.ByName("TargetXOffsetTextBlock"));
                Verify.IsNotNull(elements.targetXOffsetTextBlock);
            }

            if (elements.targetYOffsetTextBlock == null)
            {
                Log.Comment("Find the TargetYOffsetTextBlock");
                elements.targetYOffsetTextBlock = new TextBlock(FindElement.ByName("TargetYOffsetTextBlock"));
                Verify.IsNotNull(elements.targetYOffsetTextBlock);
            }

            if (elements.targetWidthTextBlock == null)
            {
                Log.Comment("Find the TargetWidthTextBlock");
                elements.targetWidthTextBlock = new TextBlock(FindElement.ByName("TargetWidthTextBlock"));
                Verify.IsNotNull(elements.targetWidthTextBlock);
            }

            if (elements.targetHeightTextBlock == null)
            {
                Log.Comment("Find the TargetHeightTextBlock");
                elements.targetHeightTextBlock = new TextBlock(FindElement.ByName("TargetHeightTextBlock"));
                Verify.IsNotNull(elements.targetHeightTextBlock);
            }

            elements.getTargetBoundsButton.Invoke();

            var retVal = new rect();
            retVal.x = double.Parse(elements.targetXOffsetTextBlock.GetText());
            retVal.y = double.Parse(elements.targetYOffsetTextBlock.GetText());
            retVal.width = double.Parse(elements.targetWidthTextBlock.GetText());
            retVal.height = double.Parse(elements.targetHeightTextBlock.GetText());
            return retVal;
        }

        private string GetEffectivePlacement()
        {
            if (elements.effectivePlacementTextBlock == null)
            {
                Log.Comment("Find the EffectivePlacementTextBlock");
                elements.effectivePlacementTextBlock = new TextBlock(FindElement.ByName("EffectivePlacementTextBlock"));
                Verify.IsNotNull(elements.effectivePlacementTextBlock);
            }
            return elements.effectivePlacementTextBlock.GetText();
        }

        private ListBoxItem GetMessage(int index)
        {
            if (elements.lstTeachingTipEvents == null)
            {
                Log.Comment("Find the lstTeachingTipEvents");
                elements.lstTeachingTipEvents = new ListBox(FindElement.ByName("lstTeachingTipEvents"));
                Verify.IsNotNull(elements.lstTeachingTipEvents);
            }
            return elements.lstTeachingTipEvents.Items[index];
        }

        private bool WaitForChecked(CheckBox checkBox, double millisecondsTimeout = 2000, bool throwOnError = true)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.On, millisecondsTimeout, throwOnError);
        }

        private bool WaitForUnchecked(CheckBox checkBox, double millisecondsTimeout = 2000, bool throwOnError = true)
        {
            return WaitForCheckBoxUpdated(checkBox, ToggleState.Off, millisecondsTimeout, throwOnError);
        }

        private bool WaitForCheckBoxUpdated(CheckBox checkBox, ToggleState state, double millisecondsTimeout, bool throwOnError)
        {
            Log.Comment(checkBox.Name + " Checked: " + checkBox.ToggleState);
            if (checkBox.ToggleState == state)
            {
                return true;
            }
            else
            {
                Log.Comment("Waiting for toggle state to change");
                checkBox.GetToggledWaiter().TryWait(TimeSpan.FromMilliseconds(millisecondsTimeout));
            }
            if (checkBox.ToggleState != state)
            {
                Log.Warning(checkBox.Name + " value never changed");
                if (throwOnError)
                {
                    throw new MS.Internal.Mita.Foundation.Waiters.WaiterException();
                }
                else
                {
                    return false;
                }
            }
            return true;
        }
    }
}
