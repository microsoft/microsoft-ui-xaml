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

using Microsoft.Windows.Apps.Test.Automation;
using Microsoft.Windows.Apps.Test.Foundation.Controls;
using Microsoft.Windows.Apps.Test.Foundation.Waiters;
using Microsoft.Windows.Apps.Test.Foundation;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    class TeachingTipTestPageElements
    {
        public ListBox GetLstTeachingTipEvents()
        {
            return GetElement(ref lstTeachingTipEvents, "lstTeachingTipEvents");
        }
        private ListBox lstTeachingTipEvents;

        public Button GetBtnClearTeachingTipEvents()
        {
            return GetElement(ref btnClearTeachingTipEvents, "btnClearTeachingTipEvents");
        }
        private Button btnClearTeachingTipEvents;

        public ComboBox GetTipLocationComboBox()
        {
            return GetElement(ref tipLocationComboBox, "TipLocationComboBox");
        }
        private ComboBox tipLocationComboBox;

        public Button GetSetTipLocationButton()
        {
            return GetElement(ref setTipLocationButton, "SetTipLocationButton");
        }
        private Button setTipLocationButton;

        public TextBlock GetEffectivePlacementTextBlock()
        {
            return GetElement(ref effectivePlacementTextBlock, "EffectivePlacementTextBlock");
        }
        private TextBlock effectivePlacementTextBlock;

        public Button GetTargetBoundsButton()
        {
            return GetElement(ref targetBoundsButton, "GetTargetBoundsButton");
        }
        private Button targetBoundsButton;

        public TextBlock GetTargetXOffsetTextBlock()
        {
            return GetElement(ref targetXOffsetTextBlock, "TargetXOffsetTextBlock");
        }
        private TextBlock targetXOffsetTextBlock;

        public TextBlock GetTargetYOffsetTextBlock()
        {
            return GetElement(ref targetYOffsetTextBlock, "TargetYOffsetTextBlock");
        }
        private TextBlock targetYOffsetTextBlock;

        public TextBlock GetTargetWidthTextBlock()
        {
            return GetElement(ref targetWidthTextBlock, "TargetWidthTextBlock");
        }
        private TextBlock targetWidthTextBlock;

        public TextBlock GetTargetHeightTextBlock()
        {
            return GetElement(ref targetHeightTextBlock, "TargetHeightTextBlock");
        }
        private TextBlock targetHeightTextBlock;

        public CheckBox GetUseTestWindowBoundsCheckbox()
        {
            return GetElement(ref useTestWindowBoundsCheckbox, "UseTestWindowBoundsCheckbox");
        }
        private CheckBox useTestWindowBoundsCheckbox;

        public Edit GetTestWindowBoundsXTextBox()
        {
            return GetElement(ref testWindowBoundsXTextBox, "TestWindowBoundsXTextBox");
        }
        private Edit testWindowBoundsXTextBox;

        public Edit GetTestWindowBoundsYTextBox()
        {
            return GetElement(ref testWindowBoundsYTextBox, "TestWindowBoundsYTextBox");
        }
        private Edit testWindowBoundsYTextBox;

        public Edit GetTestWindowBoundsWidthTextBox()
        {
            return GetElement(ref testWindowBoundsWidthTextBox, "TestWindowBoundsWidthTextBox");
        }
        private Edit testWindowBoundsWidthTextBox;

        public Edit GetTestWindowBoundsHeightTextBox()
        {
            return GetElement(ref testWindowBoundsHeightTextBox, "TestWindowBoundsHeightTextBox");
        }
        private Edit testWindowBoundsHeightTextBox;

        public CheckBox GetUseTestScreenBoundsCheckbox()
        {
            return GetElement(ref useTestScreenBoundsCheckbox, "UseTestScreenBoundsCheckbox");
        }
        private CheckBox useTestScreenBoundsCheckbox;

        public Edit GetTestScreenBoundsXTextBox()
        {
            return GetElement(ref testScreenBoundsXTextBox, "TestScreenBoundsXTextBox");
        }
        private Edit testScreenBoundsXTextBox;

        public Edit GetTestScreenBoundsYTextBox()
        {
            return GetElement(ref testScreenBoundsYTextBox, "TestScreenBoundsYTextBox");
        }
        private Edit testScreenBoundsYTextBox;

        public Edit GetTestScreenBoundsWidthTextBox()
        {
            return GetElement(ref testScreenBoundsWidthTextBox, "TestScreenBoundsWidthTextBox");
        }
        private Edit testScreenBoundsWidthTextBox;

        public Edit GetTestScreenBoundsHeightTextBox()
        {
            return GetElement(ref testScreenBoundsHeightTextBox, "TestScreenBoundsHeightTextBox");
        }
        private Edit testScreenBoundsHeightTextBox;

        public TextBlock GetTipWidthTextBlock()
        {
            return GetElement(ref tipWidthTextBlock, "TipWidthTextBlock");
        }
        private TextBlock tipWidthTextBlock;

        public TextBlock GetScrollViewerStateTextBox()
        {
            return GetElement(ref scrollViewerStateTextBox, "ScrollViewerStateTextBox");
        }
        private TextBlock scrollViewerStateTextBox;

        public Edit GetScrollViewerOffsetTextBox()
        {
            return GetElement(ref scrollViewerOffsetTextBox, "ScrollViewerOffsetTextBox");
        }
        private Edit scrollViewerOffsetTextBox;

        public Button GetScrollViewerOffsetButton()
        {
            return GetElement(ref scrollViewerOffsetButton, "ScrollViewerOffsetButton");
        }
        private Button scrollViewerOffsetButton;

        public TextBlock GetPopupVerticalOffsetTextBlock()
        {
            return GetElement(ref popupVerticalOffsetTextBlock, "PopupVerticalOffsetTextBlock");
        }
        private TextBlock popupVerticalOffsetTextBlock;

        public ComboBox GetHeroContentComboBox()
        {
            return GetElement(ref heroContentComboBox, "HeroContentComboBox");
        }
        private ComboBox heroContentComboBox;

        public Button GetSetHeroContentButton()
        {
            return GetElement(ref setHeroContentButton, "SetHeroContentButton");
        }
        private Button setHeroContentButton;

        public ComboBox GetPreferredPlacementComboBox()
        {
            return GetElement(ref preferredPlacementComboBox, "PreferredPlacementComboBox");
        }
        private ComboBox preferredPlacementComboBox;

        public Button GetSetPreferredPlacementButton()
        {
            return GetElement(ref setPreferredPlacementButton, "SetPreferredPlacementButton");
        }
        private Button setPreferredPlacementButton;

        public Button GetSetTargetButton()
        {
            return GetElement(ref setTargetButton, "SetTargetButton");
        }
        private Button setTargetButton;

        public Button GetRemoveTargetButton()
        {
            return GetElement(ref removeTargetButton, "RemoveTargetButton");
        }
        private Button removeTargetButton;

        public ComboBox GetIsLightDismissEnabledComboBox()
        {
            return GetElement(ref isLightDismissEnabledComboBox, "IsLightDismissEnabledComboBox");
        }
        private ComboBox isLightDismissEnabledComboBox;

        public ComboBox GetCloseButtonContentComboBox()
        {
            return GetElement(ref closeButtonContentComboBox, "CloseButtonContentComboBox");
        }
        private ComboBox closeButtonContentComboBox;

        public Button GetSetCloseButtonContentButton()
        {
            return GetElement(ref setCloseButtonContentButton, "SetCloseButtonContentButton");
        }
        private Button setCloseButtonContentButton;

        public Button GetIsLightDismissEnabledButton()
        {
            return GetElement(ref isLightDismissEnabledButton, "IsLightDismissEnabledButton");
        }
        private Button isLightDismissEnabledButton;

        public ComboBox GetShouldConstrainToRootBoundsComboBox()
        {
            return GetElement(ref shouldConstrainToRootBoundsComboBox, "ShouldConstrainToRootBoundsComboBox");
        }
        private ComboBox shouldConstrainToRootBoundsComboBox;

        public Button GetShouldConstrainToRootBoundsButton()
        {
            return GetElement(ref shouldConstrainToRootBoundsButton, "ShouldConstrainToRootBoundsButton");
        }
        private Button shouldConstrainToRootBoundsButton;

        public Button GetShowButton()
        {
            return GetElement(ref showButton, "ShowButton");
        }
        private Button showButton;

        public Button GetCloseButton()
        {
            return GetElement(ref closeButton, "CloseButton");
        }
        private Button closeButton;

        public CheckBox GetIsOpenCheckBox()
        {
            return GetElement(ref isOpenCheckBox, "IsOpenCheckBox");
        }
        private CheckBox isOpenCheckBox;

        public CheckBox GetIsIdleCheckBox()
        {
            return GetElement(ref isIdleCheckBox, "IsIdleCheckBox");
        }
        private CheckBox isIdleCheckBox;

        public Button GetBringTargetIntoViewButton()
        {
            return GetElement(ref bringTargetIntoViewButton, "BringTargetIntoViewButton");
        }
        private Button bringTargetIntoViewButton;

        public CheckBox GetTipFollowsTargetCheckBox()
        {
            return GetElement(ref tipFollowsTargetCheckBox, "TipFollowsTargetCheckBox");
        }
        private CheckBox tipFollowsTargetCheckBox;

        public CheckBox GetReturnTopForOutOfWindowPlacementCheckBox()
        {
            return GetElement(ref returnTopForOutOfWindowPlacementCheckBox, "ReturnTopForOutOfWindowPlacementCheckBox");
        }
        private CheckBox returnTopForOutOfWindowPlacementCheckBox;

        public ComboBox GetTitleComboBox()
        {
            return GetElement(ref titleComboBox, "TitleComboBox");
        }
        private ComboBox titleComboBox;

        public Button GetSetTitleButton()
        {
            return GetElement(ref setTitleButton, "SetTitleButton");
        }
        private Button setTitleButton;

        public ComboBox GetSubtitleComboBox()
        {
            return GetElement(ref subtitleComboBox, "SubtitleComboBox");
        }
        private ComboBox subtitleComboBox;

        public Button GetSetSubtitleButton()
        {
            return GetElement(ref setSubtitleButton, "SetSubtitleButton");
        }
        private Button setSubtitleButton;

        public ComboBox GetIconComboBox()
        {
            return GetElement(ref iconComboBox, "IconComboBox");
        }
        private ComboBox iconComboBox;

        public Button GetSetIconButton()
        {
            return GetElement(ref setIconButton, "SetIconButton");
        }
        private Button setIconButton;

        public ComboBox GetActionButtonContentComboBox()
        {
            return GetElement(ref actionButtonContentComboBox, "ActionButtonContentComboBox");
        }
        private ComboBox actionButtonContentComboBox;

        public Button GetSetActionButtonContentButton()
        {
            return GetElement(ref setActionContentButton, "SetActionContentButton");
        }
        private Button setActionContentButton;

        public ComboBox GetThemingComboBox()
        {
            return GetElement(ref themingComboBox, "PageThemeComboBox");
        }
        private ComboBox themingComboBox;

        public UIObject GetTeachingTipAlternateCloseButton()
        {
            ElementCache.Clear();
            var element = GetElement(ref teachingTipAlternateCloseButton, "Close");
            teachingTipAlternateCloseButton = null;
            return element;
        }
        private UIObject teachingTipAlternateCloseButton;

        public UIObject GetTeachingTipCloseButton()
        {
            ElementCache.Clear();
            var element = GetElement(ref teachingTipCloseButton, "C:Short Text.");
            teachingTipCloseButton = null;
            return element;
        }
        private UIObject teachingTipCloseButton;

        public UIObject GetTitleVisibilityTextBlock()
        {
            return GetElement(ref titleVisibilityTextBlock, "TitleVisibilityTextBlock");
        }
        private UIObject titleVisibilityTextBlock;

        public UIObject GetSubtitleVisibilityTextBlock()
        {
            return GetElement(ref subtitleVisibilityTextBlock, "SubtitleVisibilityTextBlock");
        }
        private UIObject subtitleVisibilityTextBlock;

        public ComboBox GetAutomationNameComboBox()
        {
            return GetElement(ref automationNameComboBox, "AutomationNameComboBox");
        }
        private ComboBox automationNameComboBox;

        public Button GetSetAutomationNameButton()
        {
            return GetElement(ref setAutomationNameButton, "SetAutomationNameButton");
        }
        private Button setAutomationNameButton;

        public TextBlock GetEffectiveForegroundOfTeachingTipButtonTextBlock()
        {
            return GetElement(ref effectiveForegroundOfTeachingTipButtonTextBlock, "EffectiveForegroundOfTeachingTipButton");
        }
        private TextBlock effectiveForegroundOfTeachingTipButtonTextBlock;

        public TextBlock GetEffectiveForegroundOfTeachingTipContentTextBlock()
        {
            return GetElement(ref effectiveForegroundOfTeachingTipContentTextBlock, "EffectiveForegroundOfTeachingTipContent");
        }
        private TextBlock effectiveForegroundOfTeachingTipContentTextBlock;

        public Button GetRemoveTeachingTipButton()
        {
            return GetElement(ref effectiveRemoveTeachingTipButton, "RemoveTeachingTipButton");
        }
        private Button effectiveRemoveTeachingTipButton;

        public TextBlock GetTeachingTipContent()
        {
            return GetElement(ref effectiveTeachingTipContent, "TeachingTipContentTextBlock");
        }
        private TextBlock effectiveTeachingTipContent;

        public CheckBox GetTeachingTipContentUnloadedCheck()
        {
            return GetElement(ref effectiveTeachingTipContentUnloadedCheckbox, "VisualTreeTeachingTipContentTextBlockUnloaded");
        }
        private CheckBox effectiveTeachingTipContentUnloadedCheckbox;

        public Button GetRemoveOpenButtonFromVisualTreeButton()
        {
            return GetElement(ref effectiveRemoveOpenButton, "RemoveButtonFromVisualTreeButton");
        }
        private Button effectiveRemoveOpenButton;

        public Button GetOpenTeachingTipOnEdgeButton()
        {
            return GetElement(ref effectiveOpenTeachingTipOnEdgeButton, "TargetButtonRightEdge");
        }
        private Button effectiveOpenTeachingTipOnEdgeButton;

        public Button GetTeachingTipOnEdgeOffsetButton()
        {
            return GetElement(ref effectiveTeachingTipOnEdgeOffsetButton, "GetEdgeTeachingTipOffset");
        }
        private Button effectiveTeachingTipOnEdgeOffsetButton;

        public TextBlock GetTeachingTipOnEdgeOffsetTextblock()
        {
            return GetElement(ref effectiveTeachingTipOnEdgeOffsetTextblock, "EdgeTeachingTipOffset");
        }
        private TextBlock effectiveTeachingTipOnEdgeOffsetTextblock;

        public double GetTeachingTipOnEdgeHorizontalOffset()
        {
            return double.Parse(GetTeachingTipOnEdgeOffsetTextblock().GetText().Split(";")[0]);
        }
        public double GetTeachingTipOnEdgeVerticalOffset()
        {
            return double.Parse(GetTeachingTipOnEdgeOffsetTextblock().GetText().Split(";")[1]);
        }

        private T GetElement<T>(ref T element, string elementName) where T : UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);
                element = FindElement.ByNameOrId<T>(elementName);
                Verify.IsNotNull(element);
            }
            return element;
        }

        public enum PlacementOptions
        {
            Top,
            Bottom,
            Left,
            Right,
            TopRight,
            TopLeft,
            BottomRight,
            BottomLeft,
            LeftTop,
            LeftBottom,
            RightTop,
            RightBottom,
            Center,
            Auto
        }

        public enum HeroContentOptions
        {
            RedSquare,
            BlueSquare,
            Image,
            NoContent
        }

        public enum TitleContentOptions
        {
            No,
            Small,
            Long
        }

        public enum SubtitleContentOptions
        {
            No,
            Small,
            Long
        }

        public enum IconOptions
        {
            People,
            NoIcon
        }

        public enum TipLocationOptions
        {
            ResourceDictionary,
            VisualTree
        }

        public enum CloseButtonContentOptions
        {
            NoText,
            ShortText,
            LongText
        }

        public enum AutomationNameOptions
        {
            VisualTree,
            Resources,
            None
        }
    }
}
