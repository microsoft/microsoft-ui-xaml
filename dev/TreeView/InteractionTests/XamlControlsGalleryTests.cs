// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Common;
using System;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Common;
using Windows.UI.Xaml.Tests.MUXControls.InteractionTests.Infra;

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
using System.Collections.Generic;
using Windows.UI.Xaml.Controls;

namespace Windows.UI.Xaml.Tests.MUXControls.InteractionTests
{
    [TestClass]
    public class XamlControlsGalleryTests
    {
        [ClassInitialize]
        [TestProperty("RunAs", "User")]
        [TestProperty("Classification", "Integration")]
        [TestProperty("TestPass:IncludeOnlyOn", "Desktop")]
        public static void ClassInitialize(TestContext testContext)
        {
            TestEnvironment.Initialize(testContext, TestType.XamlControlsGallery);
        }

        [TestCleanup]
        public void TestCleanup()
        {
            TestCleanupHelper.Cleanup();
        }

        [TestMethod]
        public void XamlControlsGalleryAcrylicPageLaunch() { LaunchAndNavigateToPage("Acrylic"); }

        [TestMethod]
        public void XamlControlsGalleryAnimatedVisualPlayerPageLaunch() { LaunchAndNavigateToPage("AnimatedVisualPlayer"); }

        [TestMethod]
        public void XamlControlsGalleryAnimation_interopPageLaunch() { LaunchAndNavigateToPage("Animation interop"); }

        [TestMethod]
        public void XamlControlsGalleryAppBarButtonPageLaunch() { LaunchAndNavigateToPage("AppBarButton"); }

        [TestMethod]
        public void XamlControlsGalleryAppBarSeparatorPageLaunch() { LaunchAndNavigateToPage("AppBarSeparator"); }

        [TestMethod]
        public void XamlControlsGalleryAppBarToggleButtonPageLaunch() { LaunchAndNavigateToPage("AppBarToggleButton"); }

        [TestMethod]
        public void XamlControlsGalleryAutoSuggestBoxPageLaunch() { LaunchAndNavigateToPage("AutoSuggestBox"); }

        [TestMethod]
        public void XamlControlsGalleryBorderPageLaunch() { LaunchAndNavigateToPage("Border"); }

        [TestMethod]
        public void XamlControlsGalleryButtonPageLaunch() { LaunchAndNavigateToPage("Button"); }

        [TestMethod]
        public void XamlControlsGalleryCalendarDatePickerPageLaunch() { LaunchAndNavigateToPage("CalendarDatePicker"); }

        [TestMethod]
        public void XamlControlsGalleryCalendarViewPageLaunch() { LaunchAndNavigateToPage("CalendarView"); }

        [TestMethod]
        public void XamlControlsGalleryCanvasPageLaunch() { LaunchAndNavigateToPage("Canvas"); }

        [TestMethod]
        public void XamlControlsGalleryCheckBoxPageLaunch() { LaunchAndNavigateToPage("CheckBox"); }

        [TestMethod]
        public void XamlControlsGalleryColorPaletteResourcesPageLaunch() { LaunchAndNavigateToPage("ColorPaletteResources"); }

        [TestMethod]
        public void XamlControlsGalleryColorPickerPageLaunch() { LaunchAndNavigateToPage("ColorPicker"); }

        [TestMethod]
        public void XamlControlsGalleryComboBoxPageLaunch() { LaunchAndNavigateToPage("ComboBox"); }

        [TestMethod]
        public void XamlControlsGalleryCommandBarPageLaunch() { LaunchAndNavigateToPage("CommandBar"); }

        [TestMethod]
        public void XamlControlsGalleryCommandBarFlyoutPageLaunch() { LaunchAndNavigateToPage("CommandBarFlyout"); }

        [TestMethod]
        public void XamlControlsGalleryCompact_SizingPageLaunch() { LaunchAndNavigateToPage("Compact Sizing"); }

        [TestMethod]
        public void XamlControlsGalleryConnected_AnimationPageLaunch() { LaunchAndNavigateToPage("Connected Animation"); }

        [TestMethod]
        public void XamlControlsGalleryContentDialogPageLaunch() { LaunchAndNavigateToPage("ContentDialog"); }

        [TestMethod]
        public void XamlControlsGalleryDataGridPageLaunch() { LaunchAndNavigateToPage("DataGrid"); }

        [TestMethod]
        public void XamlControlsGalleryDatePickerPageLaunch() { LaunchAndNavigateToPage("DatePicker"); }

        [TestMethod]
        public void XamlControlsGalleryDropDownButtonPageLaunch() { LaunchAndNavigateToPage("DropDownButton"); }

        [TestMethod]
        public void XamlControlsGalleryEasing_FunctionsPageLaunch() { LaunchAndNavigateToPage("Easing Functions"); }

        [TestMethod]
        public void XamlControlsGalleryFlipViewPageLaunch() { LaunchAndNavigateToPage("FlipView"); }

        [TestMethod]
        public void XamlControlsGalleryFlyoutPageLaunch() { LaunchAndNavigateToPage("Flyout"); }

        [TestMethod]
        public void XamlControlsGalleryGridPageLaunch() { LaunchAndNavigateToPage("Grid"); }

        [TestMethod]
        public void XamlControlsGalleryGridViewPageLaunch() { LaunchAndNavigateToPage("GridView"); }

        [TestMethod]
        public void XamlControlsGalleryHyperlinkButtonPageLaunch() { LaunchAndNavigateToPage("HyperlinkButton"); }

        [TestMethod]
        public void XamlControlsGalleryImagePageLaunch() { LaunchAndNavigateToPage("Image"); }

        [TestMethod]
        public void XamlControlsGalleryImplicit_TransitionsPageLaunch() { LaunchAndNavigateToPage("Implicit Transitions"); }

        [TestMethod]
        public void XamlControlsGalleryInkCanvasPageLaunch() { LaunchAndNavigateToPage("InkCanvas"); }

        [TestMethod]
        public void XamlControlsGalleryItemsRepeaterPageLaunch() { LaunchAndNavigateToPage("ItemsRepeater"); }

        [TestMethod]
        public void XamlControlsGalleryListBoxPageLaunch() { LaunchAndNavigateToPage("ListBox"); }

        [TestMethod]
        public void XamlControlsGalleryListViewPageLaunch() { LaunchAndNavigateToPage("ListView"); }

        [TestMethod]
        public void XamlControlsGalleryMediaPlayerElementPageLaunch() { LaunchAndNavigateToPage("MediaPlayerElement"); }

        [TestMethod]
        public void XamlControlsGalleryMenuBarPageLaunch() { LaunchAndNavigateToPage("MenuBar"); }

        [TestMethod]
        public void XamlControlsGalleryMenuFlyoutPageLaunch() { LaunchAndNavigateToPage("MenuFlyout"); }

        [TestMethod]
        public void XamlControlsGalleryNavigationViewPageLaunch() { LaunchAndNavigateToPage("NavigationView"); }

        [TestMethod]
        public void XamlControlsGalleryPage_TransitionsPageLaunch() { LaunchAndNavigateToPage("Page Transitions"); }

        [TestMethod]
        public void XamlControlsGalleryParallaxViewPageLaunch() { LaunchAndNavigateToPage("ParallaxView"); }

        [TestMethod]
        public void XamlControlsGalleryPasswordBoxPageLaunch() { LaunchAndNavigateToPage("PasswordBox"); }

        [TestMethod]
        public void XamlControlsGalleryPersonPicturePageLaunch() { LaunchAndNavigateToPage("PersonPicture"); }

        [TestMethod]
        public void XamlControlsGalleryPivotPageLaunch() { LaunchAndNavigateToPage("Pivot"); }

        [TestMethod]
        public void XamlControlsGalleryProgressBarPageLaunch() { LaunchAndNavigateToPage("ProgressBar"); }

        [TestMethod]
        public void XamlControlsGalleryProgressRingPageLaunch() { LaunchAndNavigateToPage("ProgressRing"); }

        [TestMethod]
        public void XamlControlsGalleryPullToRefreshPageLaunch() { LaunchAndNavigateToPage("PullToRefresh"); }

        [TestMethod]
        public void XamlControlsGalleryRadioButtonPageLaunch() { LaunchAndNavigateToPage("RadioButton"); }

        [TestMethod]
        public void XamlControlsGalleryRatingControlPageLaunch() { LaunchAndNavigateToPage("RatingControl"); }

        [TestMethod]
        public void XamlControlsGalleryRelativePanelPageLaunch() { LaunchAndNavigateToPage("RelativePanel"); }

        [TestMethod]
        public void XamlControlsGalleryRepeatButtonPageLaunch() { LaunchAndNavigateToPage("RepeatButton"); }

        [TestMethod]
        public void XamlControlsGalleryRevealPageLaunch() { LaunchAndNavigateToPage("Reveal"); }

        [TestMethod]
        public void XamlControlsGalleryReveal_FocusPageLaunch() { LaunchAndNavigateToPage("Reveal Focus"); }

        [TestMethod]
        public void XamlControlsGalleryRichEditBoxPageLaunch() { LaunchAndNavigateToPage("RichEditBox"); }

        [TestMethod]
        public void XamlControlsGalleryRichTextBlockPageLaunch() { LaunchAndNavigateToPage("RichTextBlock"); }

        [TestMethod]
        public void XamlControlsGalleryScrollViewerPageLaunch() { LaunchAndNavigateToPage("ScrollViewer"); }

        [TestMethod]
        public void XamlControlsGallerySemanticZoomPageLaunch() { LaunchAndNavigateToPage("SemanticZoom"); }

        [TestMethod]
        public void XamlControlsGallerySliderPageLaunch() { LaunchAndNavigateToPage("Slider"); }

        [TestMethod]
        public void XamlControlsGallerySoundPageLaunch() { LaunchAndNavigateToPage("Sound"); }

        [TestMethod]
        public void XamlControlsGallerySplitButtonPageLaunch() { LaunchAndNavigateToPage("SplitButton"); }

        [TestMethod]
        public void XamlControlsGallerySplitViewPageLaunch() { LaunchAndNavigateToPage("SplitView"); }

        [TestMethod]
        public void XamlControlsGalleryStackPanelPageLaunch() { LaunchAndNavigateToPage("StackPanel"); }

        [TestMethod]
        public void XamlControlsGalleryStandardUICommandPageLaunch() { LaunchAndNavigateToPage("StandardUICommand"); }

        [TestMethod]
        public void XamlControlsGallerySwipeControlPageLaunch() { LaunchAndNavigateToPage("SwipeControl"); }

        [TestMethod]
        public void XamlControlsGalleryTabViewPageLaunch() { LaunchAndNavigateToPage("TabView"); }

        [TestMethod]
        public void XamlControlsGalleryTeachingTipPageLaunch() { LaunchAndNavigateToPage("TeachingTip"); }

        [TestMethod]
        public void XamlControlsGalleryTextBlockPageLaunch() { LaunchAndNavigateToPage("TextBlock"); }

        [TestMethod]
        public void XamlControlsGalleryTextBoxPageLaunch() { LaunchAndNavigateToPage("TextBox"); }

        [TestMethod]
        public void XamlControlsGalleryTheme_TransitionsPageLaunch() { LaunchAndNavigateToPage("Theme Transitions"); }

        [TestMethod]
        public void XamlControlsGalleryTimePickerPageLaunch() { LaunchAndNavigateToPage("TimePicker"); }

        [TestMethod]
        public void XamlControlsGalleryToggleButtonPageLaunch() { LaunchAndNavigateToPage("ToggleButton"); }

        [TestMethod]
        public void XamlControlsGalleryToggleSplitButtonPageLaunch() { LaunchAndNavigateToPage("ToggleSplitButton"); }

        [TestMethod]
        public void XamlControlsGalleryToggleSwitchPageLaunch() { LaunchAndNavigateToPage("ToggleSwitch"); }

        [TestMethod]
        public void XamlControlsGalleryToolTipPageLaunch() { LaunchAndNavigateToPage("ToolTip"); }

        [TestMethod]
        public void XamlControlsGalleryTreeViewPageLaunch() { LaunchAndNavigateToPage("TreeView"); }

        [TestMethod]
        public void XamlControlsGalleryVariableSizedWrapGridPageLaunch() { LaunchAndNavigateToPage("VariableSizedWrapGrid"); }

        [TestMethod]
        public void XamlControlsGalleryViewBoxPageLaunch() { LaunchAndNavigateToPage("ViewBox"); }

        [TestMethod]
        public void XamlControlsGalleryWebViewPageLaunch() { LaunchAndNavigateToPage("WebView"); }

        [TestMethod]
        public void XamlControlsGalleryXamlUICommandPageLaunch() { LaunchAndNavigateToPage("XamlUICommand"); }


        private void LaunchAndNavigateToPage(string pageName)
        {
            using (var setup = new TestSetupHelper(new List<string>(), "", true))
            {
                InvokeItem("All controls", "ListViewItem");
                InvokeItem(pageName, "GridViewItem");

                Wait.ForIdle();
                KeyboardHelper.PressKey(Key.Tab);
                Wait.ForIdle();
            }
        }

        private void InvokeItem(string name, string className)
        {
            var item = FindElement.ByNameAndClassName(name, className);
            item.SetFocus();
            AutomationElement ae = AutomationElement.FocusedElement;
            InvokePattern invokePattern = ae.GetCurrentPattern(InvokePattern.Pattern) as InvokePattern;
            invokePattern.Invoke();
        }
    }
}