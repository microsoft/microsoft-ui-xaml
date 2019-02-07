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

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    class TeachingTipTestPageElements
    {
        public ListBox GetLstTeachingTipEvents()
        {
            if (lstTeachingTipEvents == null)
            {
                Log.Comment("Find the lstTeachingTipEvents");
                lstTeachingTipEvents = new ListBox(FindElement.ByName("lstTeachingTipEvents"));
                Verify.IsNotNull(lstTeachingTipEvents);
            }
            return lstTeachingTipEvents;
        }
        private ListBox lstTeachingTipEvents;

        public TextBlock GetEffectivePlacementTextBlock()
        {
            if (effectivePlacementTextBlock == null)
            {
                Log.Comment("Find the EffectivePlacementTextBlock");
                effectivePlacementTextBlock = new TextBlock(FindElement.ByName("EffectivePlacementTextBlock"));
                Verify.IsNotNull(effectivePlacementTextBlock);
            }
            return effectivePlacementTextBlock;
        }
        private TextBlock effectivePlacementTextBlock;

        public Button GetTargetBoundsButton()
        {
            if (targetBoundsButton == null)
            {
                Log.Comment("Find the GetTargetBoundsButton");
                targetBoundsButton = new Button(FindElement.ByName("GetTargetBoundsButton"));
                Verify.IsNotNull(targetBoundsButton);
            }
            return targetBoundsButton;
        }
        private Button targetBoundsButton;

        public TextBlock GetTargetXOffsetTextBlock()
        {
            if (targetXOffsetTextBlock == null)
            {
                Log.Comment("Find the TargetXOffsetTextBlock");
                targetXOffsetTextBlock = new TextBlock(FindElement.ByName("TargetXOffsetTextBlock"));
                Verify.IsNotNull(targetXOffsetTextBlock);
            }
            return targetXOffsetTextBlock;
        }
        private TextBlock targetXOffsetTextBlock;

        public TextBlock GetTargetYOffsetTextBlock()
        {
            if (targetYOffsetTextBlock == null)
            {
                Log.Comment("Find the TargetYOffsetTextBlock");
                targetYOffsetTextBlock = new TextBlock(FindElement.ByName("TargetYOffsetTextBlock"));
                Verify.IsNotNull(targetYOffsetTextBlock);
            }
            return targetYOffsetTextBlock;
        }
        private TextBlock targetYOffsetTextBlock;

        public TextBlock GetTargetWidthTextBlock()
        {
            if (targetWidthTextBlock == null)
            {
                Log.Comment("Find the TargetWidthTextBlock");
                targetWidthTextBlock = new TextBlock(FindElement.ByName("TargetWidthTextBlock"));
                Verify.IsNotNull(targetWidthTextBlock);
            }
            return targetWidthTextBlock;
        }
        private TextBlock targetWidthTextBlock;

        public TextBlock GetTargetHeightTextBlock()
        {
            if (targetHeightTextBlock == null)
            {
                Log.Comment("Find the TargetHeightTextBlock");
                targetHeightTextBlock = new TextBlock(FindElement.ByName("TargetHeightTextBlock"));
                Verify.IsNotNull(targetHeightTextBlock);
            }
            return targetYOffsetTextBlock;
        }
        private TextBlock targetHeightTextBlock;

        public CheckBox GetUseTestWindowBoundsCheckbox()
        {
            if (useTestWindowBoundsCheckbox == null)
            {
                Log.Comment("Find the UseTestWindowBoundsCheckbox");
                useTestWindowBoundsCheckbox = new CheckBox(FindElement.ByName("UseTestWindowBoundsCheckbox"));
                Verify.IsNotNull(useTestWindowBoundsCheckbox);
            }
            return useTestWindowBoundsCheckbox;
        }
        private CheckBox useTestWindowBoundsCheckbox;

        public Edit GetTestWindowBoundsXTextBox()
        {
            if (testWindowBoundsXTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsXTextBox");
                testWindowBoundsXTextBox = new Edit(FindElement.ByName("TestWindowBoundsXTextBox"));
                Verify.IsNotNull(testWindowBoundsXTextBox);
            }
            return testWindowBoundsXTextBox;
        }
        private Edit testWindowBoundsXTextBox;

        public Edit GetTestWindowBoundsYTextBox()
        {
            if (testWindowBoundsYTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsYTextBox");
                testWindowBoundsYTextBox = new Edit(FindElement.ByName("TestWindowBoundsYTextBox"));
                Verify.IsNotNull(testWindowBoundsYTextBox);
            }
            return testWindowBoundsYTextBox;
        }
        private Edit testWindowBoundsYTextBox;

        public Edit GetTestWindowBoundsWidthTextBox()
        {
            if (testWindowBoundsWidthTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsWidthTextBox");
                testWindowBoundsWidthTextBox = new Edit(FindElement.ByName("TestWindowBoundsWidthTextBox"));
                Verify.IsNotNull(testWindowBoundsWidthTextBox);
            }
            return testWindowBoundsWidthTextBox;
        }
        private Edit testWindowBoundsWidthTextBox;

        public Edit GetTestWindowBoundsHeightTextBox()
        {
            if (testWindowBoundsHeightTextBox == null)
            {
                Log.Comment("Find the TestWindowBoundsHeightTextBox");
                testWindowBoundsHeightTextBox = new Edit(FindElement.ByName("TestWindowBoundsHeightTextBox"));
                Verify.IsNotNull(testWindowBoundsHeightTextBox);
            }
            return testWindowBoundsHeightTextBox;
        }
        private Edit testWindowBoundsHeightTextBox;

        public TextBlock GetTipWidthTextBlock()
        {
            if (tipWidthTextBlock == null)
            {
                Log.Comment("Find the TipWidthTextBlock");
                tipWidthTextBlock = new TextBlock(FindElement.ByName("TipWidthTextBlock"));
                Verify.IsNotNull(tipWidthTextBlock);
            }
            return tipWidthTextBlock;
        }
        private TextBlock tipWidthTextBlock;

        public TextBlock GetScrollViewerStateTextBox()
        {
            if (scrollViewerStateTextBox == null)
            {
                Log.Comment("Find the ScrollViewerStateTextBox");
                scrollViewerStateTextBox = new TextBlock(FindElement.ByName("ScrollViewerStateTextBox"));
                Verify.IsNotNull(scrollViewerStateTextBox);
            }
            return scrollViewerStateTextBox;
        }
        private TextBlock scrollViewerStateTextBox;

        public Edit GetScrollViewerOffsetTextBox()
        {
            if (scrollViewerOffsetTextBox == null)
            {
                Log.Comment("Find the ScrollViewerOffsetTextBox");
                scrollViewerOffsetTextBox = new Edit(FindElement.ByName("ScrollViewerOffsetTextBox"));
                Verify.IsNotNull(scrollViewerOffsetTextBox);
            }
            return scrollViewerOffsetTextBox;
        }
        private Edit scrollViewerOffsetTextBox;

        public Button GetScrollViewerOffsetButton()
        {
            if (scrollViewerOffsetButton == null)
            {
                Log.Comment("Find the ScrollViewerOffsetButton");
                scrollViewerOffsetButton = new Button(FindElement.ByName("ScrollViewerOffsetButton"));
                Verify.IsNotNull(scrollViewerOffsetButton);
            }
            return scrollViewerOffsetButton;
        }
        private Button scrollViewerOffsetButton;

        public TextBlock GetPopupVerticalOffsetTextBlock()
        {
            if (popupVerticalOffsetTextBlock == null)
            {
                Log.Comment("Find the PopupVerticalOffsetTextBlock");
                popupVerticalOffsetTextBlock = new TextBlock(FindElement.ByName("PopupVerticalOffsetTextBlock"));
                Verify.IsNotNull(popupVerticalOffsetTextBlock);
            }
            return popupVerticalOffsetTextBlock;
        }
        private TextBlock popupVerticalOffsetTextBlock;

        public ComboBox GetBleedingContentComboBox()
        {
            if (bleedingContentComboBox == null)
            {
                Log.Comment("Find the BleedingContentComboBox");
                bleedingContentComboBox = new ComboBox(FindElement.ByName("BleedingContentComboBox"));
                Verify.IsNotNull(bleedingContentComboBox);
            }
            return bleedingContentComboBox;
        }
        private ComboBox bleedingContentComboBox;

        public Button GetSetBleedingContentButton()
        {
            if (setBleedingContentButton == null)
            {
                Log.Comment("Find the SetBleedingContentButton");
                setBleedingContentButton = new Button(FindElement.ByName("SetBleedingContentButton"));
                Verify.IsNotNull(setBleedingContentButton);
            }
            return setBleedingContentButton;
        }
        private Button setBleedingContentButton;

        public ComboBox GetPlacementComboBox()
        {
            if (placementComboBox == null)
            {
                Log.Comment("Find the PlacementComboBox");
                placementComboBox = new ComboBox(FindElement.ByName("PlacementComboBox"));
                Verify.IsNotNull(placementComboBox);
            }
            return placementComboBox;
        }
        private ComboBox placementComboBox;

        public Button GetSetPlacementButton()
        {
            if (setPlacementButton == null)
            {
                Log.Comment("Find the SetPlacementButton");
                setPlacementButton = new Button(FindElement.ByName("SetPlacementButton"));
                Verify.IsNotNull(setPlacementButton);
            }
            return setPlacementButton;
        }
        private Button setPlacementButton;

        public ComboBox GetIsLightDismissEnabledComboBox()
        {
            if (isLightDismissEnabledComboBox == null)
            {
                Log.Comment("Find the IsLightDismissEnabledComboBox");
                isLightDismissEnabledComboBox = new ComboBox(FindElement.ByName("IsLightDismissEnabledComboBox"));
                Verify.IsNotNull(isLightDismissEnabledComboBox);
            }
            return isLightDismissEnabledComboBox;
        }
        private ComboBox isLightDismissEnabledComboBox;

        public Button GetIsLightDismissEnabledButton()
        {
            if (isLightDismissEnabledButton == null)
            {
                Log.Comment("Find the IsLightDismissEnabledButton");
                isLightDismissEnabledButton = new Button(FindElement.ByName("IsLightDismissEnabledButton"));
                Verify.IsNotNull(isLightDismissEnabledButton);
            }
            return isLightDismissEnabledButton;
        }
        private Button isLightDismissEnabledButton;

        public Button GetShowButton()
        {
            if (showButton == null)
            {
                Log.Comment("Find the ShowButton");
                showButton = new Button(FindElement.ByName("ShowButton"));
                Verify.IsNotNull(showButton);
            }
            return showButton;
        }
        private Button showButton;

        public Button GetCloseButton()
        {
            if (closeButton == null)
            {
                Log.Comment("Find the CloseButton");
                closeButton = new Button(FindElement.ByName("CloseButton"));
                Verify.IsNotNull(closeButton);
            }
            return closeButton;
        }
        private Button closeButton;

        public CheckBox GetIsOpenCheckBox()
        {
            if (isOpenCheckBox == null)
            {
                Log.Comment("Find the IsOpenCheckBox");
                isOpenCheckBox = new CheckBox(FindElement.ByName("IsOpenCheckBox"));
                Verify.IsNotNull(isOpenCheckBox);
            }
            return isOpenCheckBox;
        }
        private CheckBox isOpenCheckBox;

        public CheckBox GetIsIdleCheckBox()
        {
            if (isIdleCheckBox == null)
            {
                Log.Comment("Find the IsIdleCheckBox");
                isIdleCheckBox = new CheckBox(FindElement.ByName("IsIdleCheckBox"));
                Verify.IsNotNull(isIdleCheckBox);
            }
            return isIdleCheckBox;
        }
        private CheckBox isIdleCheckBox;

        public Button GetBringTargetIntoViewButton()
        {
            if (bringTargetIntoViewButton == null)
            {
                Log.Comment("Find the BringTargetIntoViewButton");
                bringTargetIntoViewButton = new Button(FindElement.ByName("BringTargetIntoViewButton"));
                Verify.IsNotNull(bringTargetIntoViewButton);
            }
            return bringTargetIntoViewButton;
        }
        private Button bringTargetIntoViewButton;

        public CheckBox GetTipFollowsTargetCheckBox()
        {
            if (tipFollowsTargetCheckBox == null)
            {
                Log.Comment("Find the TipFollowsTargetCheckBox");
                tipFollowsTargetCheckBox = new CheckBox(FindElement.ByName("TipFollowsTargetCheckBox"));
                Verify.IsNotNull(tipFollowsTargetCheckBox);
            }
            return tipFollowsTargetCheckBox;
        }
        private CheckBox tipFollowsTargetCheckBox;

        public UIObject GetTeachingTip()
        {
            if (teachingTip == null)
            {
                Log.Comment("Find the TeachingTip");
                teachingTip = FindElement.ByName("TeachingTip");
                Verify.IsNotNull(teachingTip);
            }
            return teachingTip;
        }
        private UIObject teachingTip;

        public enum PlacementOptions
        {
            Top,
            Bottom,
            Left,
            Right,
            TopEdgeAlignedRight,
            TopEdgeAlignedLeft,
            BottomEdgeAlignedRight,
            BottomEdgeAlignedLeft,
            LeftEdgeAlignedTop,
            LeftEdgeAlignedBottom,
            RightEdgeAlignedTop,
            RightEdgeAlignedBottom,
            Auto
        }

        public enum BleedingContentOptions
        {
            RedSquare,
            BlueSquare,
            Image,
            NoContent
        }
    }
}
