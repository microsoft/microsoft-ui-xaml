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

        public ComboBox GetBleedingContentComboBox()
        {
            return GetElement(ref bleedingContentComboBox, "BleedingContentComboBox");
        }
        private ComboBox bleedingContentComboBox;

        public Button GetSetBleedingContentButton()
        {
            return GetElement(ref setBleedingContentButton, "SetBleedingContentButton");
        }
        private Button setBleedingContentButton;

        public ComboBox GetPlacementComboBox()
        {
            return GetElement(ref placementComboBox, "PlacementComboBox");
        }
        private ComboBox placementComboBox;

        public Button GetSetPlacementButton()
        {
            return GetElement(ref setPlacementButton, "SetPlacementButton");
        }
        private Button setPlacementButton;

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

        public Button GetIsLightDismissEnabledButton()
        {
            return GetElement(ref isLightDismissEnabledButton, "IsLightDismissEnabledButton");
        }
        private Button isLightDismissEnabledButton;

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

        public UIObject GetTeachingTipAlternateCloseButton()
        {
            ElementCache.Clear();
            var element = GetElement(ref teachingTipAlternateCloseButton, "Close");
            teachingTipAlternateCloseButton = null;
            return element;
        }
        private UIObject teachingTipAlternateCloseButton;

        private T GetElement<T>(ref T element, string elementName) where T : UIObject
        {
            if (element == null)
            {
                Log.Comment("Find the " + elementName);
                element = FindElement.ByName<T>(elementName);
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
    }
}
