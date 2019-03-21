using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using System;
using System.Numerics;
using Common;

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
using MS.Internal.Mita.Foundation.Waiters;
#else
using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Microsoft.Windows.Apps.Test.Foundation;
#endif 

using static Windows.UI.Xaml.Tests.MUXControls.InteractionTests.TeachingTipTestPageElements;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class TeachingTipTests
    {
        // The longest observed animated view change took 5.4 seconds, so 9 seconds is picked
        // as the default timeout so there is a reasonable margin for reliability.
        const double defaultAnimatedViewChangeTimeout = 9000;

        TeachingTipTestPageElements elements;

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
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    var message0 = GetTeachingTipDebugMessage(0);
                    var message1 = GetTeachingTipDebugMessage(1);
                    Verify.IsTrue(message0.ToString().Contains("Closing"));
                    Verify.IsTrue(message0.ToString().Contains("Programmatic"));
                    Verify.IsTrue(message1.ToString().Contains("Closed"));
                    Verify.IsTrue(message1.ToString().Contains("Programmatic"));

                    SetHeroContent(HeroContentOptions.NoContent);
                    OpenTeachingTip();
                    PressXCloseButton();
                    var message2 = GetTeachingTipDebugMessage(2);
                    var message3 = GetTeachingTipDebugMessage(3);
                    var message4 = GetTeachingTipDebugMessage(4);
                    Verify.IsTrue(message2.ToString().Contains("Close Button Clicked"));
                    Verify.IsTrue(message3.ToString().Contains("Closing"));
                    Verify.IsTrue(message3.ToString().Contains("CloseButton"));
                    Verify.IsTrue(message4.ToString().Contains("Closed"));
                    Verify.IsTrue(message4.ToString().Contains("CloseButton"));

                    EnableLightDismiss(true);
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    var message5 = GetTeachingTipDebugMessage(5);
                    var message6 = GetTeachingTipDebugMessage(6);
                    Verify.IsTrue(message5.ToString().Contains("Closing"));
                    Verify.IsTrue(message5.ToString().Contains("Programmatic"));
                    Verify.IsTrue(message6.ToString().Contains("Closed"));
                    Verify.IsTrue(message6.ToString().Contains("Programmatic"));

                    OpenTeachingTip();
                    CloseTeachingTipByLightDismiss();
                    var message7 = GetTeachingTipDebugMessage(7);
                    var message8 = GetTeachingTipDebugMessage(8);
                    Verify.IsTrue(message7.ToString().Contains("Closing"));
                    Verify.IsTrue(message7.ToString().Contains("LightDismiss"));
                    Verify.IsTrue(message8.ToString().Contains("Closed"));
                    Verify.IsTrue(message8.ToString().Contains("LightDismiss"));

                    SetCloseButtonContent(CloseButtonContentOptions.ShortText);
                    OpenTeachingTip();
                    PressTipCloseButton();
                    var message9 = GetTeachingTipDebugMessage(9);
                    var message10 = GetTeachingTipDebugMessage(10);
                    var message11 = GetTeachingTipDebugMessage(11);
                    Verify.IsTrue(message9.ToString().Contains("Close Button Clicked"));
                    Verify.IsTrue(message10.ToString().Contains("Closing"));
                    Verify.IsTrue(message10.ToString().Contains("CloseButton"));
                    Verify.IsTrue(message11.ToString().Contains("Closed"));
                    Verify.IsTrue(message11.ToString().Contains("CloseButton"));

                    ClearTeachingTipDebugMessages();
                }
            }
        }

        [TestMethod]
        public void TipCanFollowTarget()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    Wait.ForIdle();
                    OpenTeachingTip();
                    double initialTipVerticalOffset = GetTipVerticalOffset();
                    double initialScrollViewerVerticalOffset = GetScrollViewerVerticalOffset();

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);

                    SetTipFollowsTarget(true);

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Verify.IsLessThan(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Verify.IsGreaterThan(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);

                    SetTipFollowsTarget(false);

                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset + 10);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(-20);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset - 10);
                    Wait.ForIdle();
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);
                    ScrollBy(10);
                    WaitForOffsetUpdated(initialScrollViewerVerticalOffset);
                    Equals(GetTipVerticalOffset(), initialTipVerticalOffset);
                }
            }
        }

        [TestMethod]
        public void AutoPlacement()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    var targetRect = GetTargetBounds();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 340, targetRect.Y + 656, targetRect.Z + 680);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Top"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 336, targetRect.Y + 656, targetRect.Z + 680);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Bottom"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 318, targetRect.Y + 658, targetRect.Z + 640);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("RightEdgeAlignedTop"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 100, targetRect.Y + 658, targetRect.Z + 403);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("RightEdgeAlignedBottom"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 100, targetRect.Y + 643, targetRect.Z + 403);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("LeftEdgeAlignedBottom"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 300, targetRect.Y + 643, targetRect.Z + 603);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("LeftEdgeAlignedTop"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 328, targetRect.X - 340, targetRect.Y + 348, targetRect.Z + 608);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("TopEdgeAlignedLeft"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 20, targetRect.X - 340, targetRect.Y + 348, targetRect.Z + 608);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("TopEdgeAlignedRight"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 328, targetRect.X - 100, targetRect.Y + 348, targetRect.Z + 444);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("BottomEdgeAlignedLeft"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 20, targetRect.X - 100, targetRect.Y + 348, targetRect.Z + 444);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("BottomEdgeAlignedRight"));
                    CloseTeachingTipProgrammatically();

                    // Remove the hero content;
                    SetHeroContent(HeroContentOptions.NoContent);

                    UseTestWindowBounds(targetRect.W - 329, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 20);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Left"));
                    CloseTeachingTipProgrammatically();
                    UseTestWindowBounds(targetRect.W - 20, targetRect.X - 100, targetRect.Y + 349, targetRect.Z + 20);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Right"));
                    CloseTeachingTipProgrammatically();
                }
            }
        }

        [TestMethod]
        public void SpecifiedPlacement()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();

                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);

                    SetPreferredPlacement(PlacementOptions.Top);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Top"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.Bottom);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Bottom"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.Left);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Left"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.Right);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("Right"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.TopEdgeAlignedRight);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("TopEdgeAlignedRight"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.TopEdgeAlignedLeft);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("TopEdgeAlignedLeft"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.BottomEdgeAlignedRight);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("BottomEdgeAlignedRight"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.BottomEdgeAlignedLeft);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("BottomEdgeAlignedLeft"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.LeftEdgeAlignedTop);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("LeftEdgeAlignedTop"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.LeftEdgeAlignedBottom);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("LeftEdgeAlignedBottom"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.RightEdgeAlignedTop);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("RightEdgeAlignedTop"));
                    CloseTeachingTipProgrammatically();

                    SetPreferredPlacement(PlacementOptions.RightEdgeAlignedBottom);
                    OpenTeachingTip();
                    Verify.IsTrue(GetEffectivePlacement().Equals("RightEdgeAlignedBottom"));
                    CloseTeachingTipProgrammatically();
                }
            }
        }

        [TestMethod]
        public void NoIconDoesNotCrash()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();

                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);

                    SetIcon(IconOptions.NoIcon);
                    OpenTeachingTip();
                    CloseTeachingTipProgrammatically();
                    SetIcon(IconOptions.People);
                    OpenTeachingTip();
                    SetIcon(IconOptions.NoIcon);
                }
            }
        }

        [TestMethod]
        public void AutomationNameIsForwordedToPopup()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();
                foreach(TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    OpenTeachingTip();
                    Verify.IsNotNull(FindElement.ByNameAndClassName(location == TipLocationOptions.VisualTree ? "TeachingTipInVisualTree" : "TeachingTipInResources", "Popup"));
                    SetAutomationName(AutomationNameOptions.None);
                    Verify.IsNotNull(FindElement.ByNameAndClassName("We've Added Auto Saving!", "Popup"));
                    SetAutomationName(location == TipLocationOptions.VisualTree ? AutomationNameOptions.VisualTree : AutomationNameOptions.Resources);
                    CloseTeachingTipProgrammatically();
                }
            }
        }

        [TestMethod]
        public void F6PutsFocusOnCloseButton()
        {
            using (var setup = new TestSetupHelper("TeachingTip Tests"))
            {
                elements = new TeachingTipTestPageElements();
                foreach (TipLocationOptions location in Enum.GetValues(typeof(TipLocationOptions)))
                {
                    SetTeachingTipLocation(location);

                    ScrollTargetIntoView();
                    ScrollBy(10);
                    OpenTeachingTip();
                    CloseOpenAndCloseWithJustKeyboardViaF6();
                    SetCloseButtonContent(CloseButtonContentOptions.ShortText);
                    OpenTeachingTip();
                    CloseOpenAndCloseWithJustKeyboardViaF6();
                }
            }
        }

        private void CloseOpenAndCloseWithJustKeyboardViaF6()
        {
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipClosed();
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipOpened();
            KeyboardHelper.PressKey(Key.F6);
            KeyboardHelper.PressKey(Key.Enter);
            WaitForTipClosed();
        }

        private void ScrollTargetIntoView()
        {
            elements.GetBringTargetIntoViewButton().Invoke();
            Wait.ForIdle();
        }

        private void OpenTeachingTip()
        {
            if(elements.GetIsOpenCheckBox().ToggleState != ToggleState.On)
            {
                elements.GetShowButton().Tap();
                if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
                {
                    WaitForUnchecked(elements.GetIsIdleCheckBox());
                }
                WaitForChecked(elements.GetIsOpenCheckBox());
                WaitForChecked(elements.GetIsIdleCheckBox());
            }
        }

        private void CloseTeachingTipProgrammatically()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                elements.GetCloseButton().Invoke();
                WaitForTipClosed();
            }
        }

        private void CloseTeachingTipByLightDismiss()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                elements.GetLstTeachingTipEvents().Tap();
                WaitForTipClosed();
            }
        }

        private void PressXCloseButton()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                InputHelper.Tap(elements.GetTeachingTipAlternateCloseButton());
                WaitForTipClosed();
            }
        }

        private void PressTipCloseButton()
        {
            if (elements.GetIsOpenCheckBox().ToggleState != ToggleState.Off)
            {
                InputHelper.Tap(elements.GetTeachingTipCloseButton());
                WaitForTipClosed();
            }
        }

        private void WaitForTipOpened()
        {
            if (PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                WaitForUnchecked(elements.GetIsIdleCheckBox());
            }
            WaitForChecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());
        }

        private void WaitForTipClosed()
        {
            if (!PlatformConfiguration.IsOsVersionGreaterThanOrEqual(OSVersion.Redstone5))
            {
                WaitForUnchecked(elements.GetIsIdleCheckBox());
            }
            WaitForUnchecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());
        }

        private void SetTeachingTipLocation(TipLocationOptions location)
        {
            switch(location)
            {
                case TipLocationOptions.ResourceDictionary:
                    elements.GetTipLocationComboBox().SelectItemByName("Resources");
                    break;
                default:
                    elements.GetTipLocationComboBox().SelectItemByName("VisualTree");
                    break;
            }
            elements.GetSetTipLocationButton().Invoke();
            //If a tip was open this action would cause that tip to close, so wait until that happens
            WaitForUnchecked(elements.GetIsOpenCheckBox());
            WaitForChecked(elements.GetIsIdleCheckBox());

            SetTipIsTargeted(true);
        }

        private void EnableLightDismiss(bool enable)
        {
            if(enable)
            {
                elements.GetIsLightDismissEnabledComboBox().SelectItemByName("True");
            }
            else
            {
                elements.GetIsLightDismissEnabledComboBox().SelectItemByName("False");
            }
            elements.GetIsLightDismissEnabledButton().Invoke();
        }

        private void SetCloseButtonContent(CloseButtonContentOptions closeButtonContent)
        {
            switch(closeButtonContent)
            {
                case CloseButtonContentOptions.NoText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("No text");
                    break;
                case CloseButtonContentOptions.ShortText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("Small text");
                    break; 
                case CloseButtonContentOptions.LongText:
                    elements.GetCloseButtonContentComboBox().SelectItemByName("Long text");
                    break;
            }
            elements.GetSetCloseButtonContentButton().Invoke();
        }

        private void SetPreferredPlacement(PlacementOptions placement)
        {
            switch (placement)
            {
                case PlacementOptions.Top:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Top");
                    break;
                case PlacementOptions.Bottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Bottom");
                    break;
                case PlacementOptions.Left:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Left");
                    break;
                case PlacementOptions.Right:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Right");
                    break;
                case PlacementOptions.TopEdgeAlignedRight:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("TopEdgeAlignedRight");
                    break;
                case PlacementOptions.TopEdgeAlignedLeft:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("TopEdgeAlignedLeft");
                    break;
                case PlacementOptions.BottomEdgeAlignedRight:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("BottomEdgeAlignedRight");
                    break;
                case PlacementOptions.BottomEdgeAlignedLeft:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("BottomEdgeAlignedLeft");
                    break;
                case PlacementOptions.LeftEdgeAlignedTop:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("LeftEdgeAlignedTop");
                    break;
                case PlacementOptions.LeftEdgeAlignedBottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("LeftEdgeAlignedBottom");
                    break;
                case PlacementOptions.RightEdgeAlignedTop:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("RightEdgeAlignedTop");
                    break;
                case PlacementOptions.RightEdgeAlignedBottom:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("RightEdgeAlignedBottom");
                    break;
                default:
                    elements.GetPreferredPlacementComboBox().SelectItemByName("Auto");
                    break;
            }
            elements.GetSetPreferredPlacementButton().Invoke();
        }

        private void SetHeroContent(HeroContentOptions heroContent)
        {
            switch (heroContent)
            {
                case HeroContentOptions.RedSquare:
                    elements.GetHeroContentComboBox().SelectItemByName("Red Square");
                    break;
                case HeroContentOptions.BlueSquare:
                    elements.GetHeroContentComboBox().SelectItemByName("Blue Square");
                    break;
                case HeroContentOptions.Image:
                    elements.GetHeroContentComboBox().SelectItemByName("Image");
                    break;
                default:
                    elements.GetHeroContentComboBox().SelectItemByName("No Content");
                    break;
            }
            elements.GetSetHeroContentButton().Invoke();
        }

        private void SetTipIsTargeted(bool targeted)
        {
            if(targeted)
            {
                elements.GetSetTargetButton().Invoke();
            }
            else
            {
                elements.GetRemoveTargetButton().Invoke();
            }
        }

        private void SetIcon(IconOptions icon)
        {
            switch(icon)
            {
                case IconOptions.People:
                    elements.GetIconComboBox().SelectItemByName("People Icon");
                    break;
                default:
                    elements.GetIconComboBox().SelectItemByName("No Icon");
                    break;
            }
            elements.GetSetIconButton().Invoke();
        }

        private double GetTipVerticalOffset()
        {
            return double.Parse(elements.GetPopupVerticalOffsetTextBlock().GetText());
        }

        private double GetScrollViewerVerticalOffset()
        {
            return double.Parse(elements.GetScrollViewerOffsetTextBox().GetText());
        }

        private void ScrollBy(double ammount)
        {
            double initialOffset = double.Parse(elements.GetScrollViewerOffsetTextBox().GetText());
            elements.GetScrollViewerOffsetTextBox().SetValue((initialOffset + ammount).ToString());
            elements.GetScrollViewerOffsetButton().Invoke();
        }
        
        private void UseTestWindowBounds(double x, double y, double width, double height)
        {
            elements.GetTestWindowBoundsXTextBox().SetValue(x.ToString());
            elements.GetTestWindowBoundsYTextBox().SetValue(y.ToString());
            elements.GetTestWindowBoundsWidthTextBox().SetValue(width.ToString());
            elements.GetTestWindowBoundsHeightTextBox().SetValue(height.ToString());

            elements.GetUseTestWindowBoundsCheckbox().Uncheck();
            elements.GetUseTestWindowBoundsCheckbox().Check();
        }

        private void SetTipFollowsTarget(bool tipFollowsTarget)
        {
            if(tipFollowsTarget)
            {
                elements.GetTipFollowsTargetCheckBox().Check();
            }
            else
            {
                elements.GetTipFollowsTargetCheckBox().Uncheck();
            }
        }

        Vector4 GetTargetBounds()
        {
            elements.GetTargetBoundsButton().Invoke();

            var retVal = new Vector4();
            retVal.W = (int)Math.Floor(double.Parse(elements.GetTargetXOffsetTextBlock().GetText()));
            retVal.X = (int)Math.Floor(double.Parse(elements.GetTargetYOffsetTextBlock().GetText()));
            retVal.Y = (int)Math.Floor(double.Parse(elements.GetTargetWidthTextBlock().GetText()));
            retVal.Z = (int)Math.Floor(double.Parse(elements.GetTargetHeightTextBlock().GetText()));
            return retVal;
        }

        private string GetEffectivePlacement()
        {
            return elements.GetEffectivePlacementTextBlock().GetText();
        }

        private void SetAutomationName(AutomationNameOptions automationName)
        {
            switch(automationName)
            {
                case AutomationNameOptions.VisualTree:
                    elements.GetAutomationNameComboBox().SelectItemByName("TeachingTipInVisualTree");
                    break;
                case AutomationNameOptions.Resources:
                    elements.GetAutomationNameComboBox().SelectItemByName("TeachingTipInResources");
                    break;
                default:
                    elements.GetAutomationNameComboBox().SelectItemByName("None");
                    break;
            }
            elements.GetSetAutomationNameButton().Invoke();
        }

        // The test UI has a list box which the teaching tip populates with messages about which events have fired and other useful
        // Debugging info. This method returns the message at the provided index, which helps testing that events were received in
        // the expected order.
        private ListBoxItem GetTeachingTipDebugMessage(int index)
        {
            return elements.GetLstTeachingTipEvents().Items[index];
        }

        private void ClearTeachingTipDebugMessages()
        {
            elements.GetBtnClearTeachingTipEvents().Invoke();
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
                    throw new WaiterException();
                }
                else
                {
                    return false;
                }
            }
            return true;
        }

        private int WaitForOffsetUpdated(
            double expectedValue,
            double millisecondsTimeout = defaultAnimatedViewChangeTimeout,
            bool failOnError = true)
        {
            Log.Comment("WaitForOffsetUpdated with expectedValue: " + expectedValue);

            int warningCount = 0;
            bool success = WaitForOffsetToSettle(elements.GetScrollViewerOffsetTextBox(), millisecondsTimeout, failOnError);
            double value = Convert.ToDouble(elements.GetScrollViewerOffsetTextBox().GetText());
            bool goodValue = value == expectedValue;
            Verify.IsTrue(goodValue);
            return warningCount;
        }

        private bool WaitForOffsetToSettle(Edit text, double millisecondsTimeout, bool failOnError)
        {
            Wait.ForIdle();

            const double millisecondsNormalStepTimeout = 100;
            const double millisecondsIdleStepTimeout = 600;
            ValueChangedEventWaiter waiter = new ValueChangedEventWaiter(text);
            int unsuccessfulWaits = 0;
            int maxUnsuccessfulWaits = (int)(millisecondsIdleStepTimeout / millisecondsNormalStepTimeout);

            Log.Comment("Original State: " + elements.GetScrollViewerStateTextBox().GetText());
            Log.Comment("Original Offset: " + text.Value);

            // When the initial State is still Idle, use a longer timeout to allow it to transition out of Idle.
            double millisecondsWait = (elements.GetScrollViewerStateTextBox().GetText() == "Idle") ? millisecondsIdleStepTimeout : millisecondsNormalStepTimeout;
            double millisecondsCumulatedWait = 0;

            do
            {
                Log.Comment("Waiting for Offset change.");
                waiter.Reset();
                if (waiter.TryWait(TimeSpan.FromMilliseconds(millisecondsWait)))
                {
                    unsuccessfulWaits = 0;
                }
                else
                {
                    unsuccessfulWaits++;
                }
                millisecondsCumulatedWait += millisecondsWait;
                millisecondsWait = millisecondsNormalStepTimeout;

                Log.Comment("Current State: " + elements.GetScrollViewerStateTextBox().GetText());
                Log.Comment("Current Offset: " + text.Value);

                Wait.ForIdle();
            }
            while (elements.GetScrollViewerStateTextBox().GetText() != "Idle" &&
                   millisecondsCumulatedWait < millisecondsTimeout &&
                   unsuccessfulWaits <= maxUnsuccessfulWaits);

            if (elements.GetScrollViewerStateTextBox().GetText() == "Idle")
            {
                Log.Comment("Idle State reached after " + millisecondsCumulatedWait + " out of " + millisecondsTimeout + " milliseconds. Final Offset: " + text.Value);
                return true;
            }
            else
            {
                string message = unsuccessfulWaits > maxUnsuccessfulWaits ?
                    "Offset has not changed within " + millisecondsIdleStepTimeout + " milliseconds outside of Idle State." :
                    "Idle State was not reached within " + millisecondsTimeout + " milliseconds.";
                if (failOnError)
                {
                    Log.Error(message);
                }
                else
                {
                    Log.Warning(message);
                }

                return false;
            }
        }
    }
}
