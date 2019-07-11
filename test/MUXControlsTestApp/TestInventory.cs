// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    static class Extensions
    {
        public static void Add(this List<TestDeclaration> list, string name, Type pageType)
        {
            list.Add(new TestDeclaration() {
                Name = name,
                AutomationName = name + " Tests",
                PageType = pageType
            });
        }

        public static void Add(this List<TestDeclaration> list, string name, Type pageType, string icon)
        {
            list.Add(new TestDeclaration() {
                Name = name,
                AutomationName = name + " Tests",
                PageType = pageType,
                Icon = "ms-appx:///Assets/" + icon
            });
        }
    }

    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>()
            {
// #if FEATURE_SCROLLER_ENABLED && FEATURE_COLOR_PICKER_ENABLED && FEATURE_NAVIGATION_VIEW_ENABLED && FEATURE_RATING_CONTROL_ENABLED
//              {"Leak", typeof(LeakTestPage)},
//#endif

#if FEATURE_COLOR_PICKER_ENABLED
                {"ColorPicker", typeof(ColorPickerPage), "ColorPicker.png"},
#endif

#if FEATURE_PERSON_PICTURE_ENABLED
                {"PersonPicture", typeof(PersonPicturePage), "PersonPicture.png"},
#endif

#if FEATURE_RATING_CONTROL_ENABLED
                {"RatingControl", typeof(RatingControlPage), "RatingControl.png"},
#endif

#if FEATURE_SWIPE_CONTROL_ENABLED
                {"SwipeControl", typeof(SwipePage), "Swipe.png"},
#endif

#if FEATURE_TREEVIEW_ENABLED
                {"TreeView", typeof(TreeViewPage), "TreeView.png"},
#endif

#if FEATURE_TEACHING_TIP_ENABLED
                {"TeachingTip", typeof(TeachingTipPage), "TeachingTip.png"},
#endif

#if FEATURE_TWO_PANE_VIEW_ENABLED
                {"TwoPaneView", typeof(TwoPaneViewPage)},
#endif

#if FEATURE_PULL_TO_REFRESH_ENABLED
                {"PTR", typeof(PTRPage), "PullToRefresh.png"},
#endif

#if FEATURE_MENUBAR_ENABLED
                {"MenuBar", typeof(MenuBarPage), "MenuBar.png"},
#endif

#if FEATURE_MENU_FLYOUT_ENABLED
                {"MenuFlyout", typeof(MenuFlyoutPage), "MenuFlyout.png"},
#endif

#if FEATURE_SCROLLVIEWER_ENABLED
                {"ScrollViewerAdapter", typeof(ScrollViewerAdapterPage), "ScrollViewer.png"},
#endif

#if FEATURE_SPLIT_BUTTON_ENABLED
                {"SplitButton", typeof(SplitButtonPage), "SplitButton.png"},
#endif

#if FEATURE_DROPDOWN_BUTTON_ENABLED
                {"DropDownButton", typeof(DropDownButtonPage), "DropdownButton.png"},
#endif

#if FEATURE_COMMANDBAR_FLYOUT_ENABLED
                {"CommandBarFlyout", typeof(CommandBarFlyoutMainPage), "CommandBarFlyout.png"},
#endif

#if FEATURE_COMMON_STYLES_ENABLED
                {"CommonStyles", typeof(CommonStylesPage)},
                {"Compact", typeof(CompactPage), "CompactSizing.png"},
                {"CornerRadius", typeof(CornerRadiusPage)},
#endif

#if FEATURE_RADIOBUTTONS_ENABLED
                {"RadioButtons", typeof(RadioButtonsPage), "RadioButton.png"},
#endif

#if FEATURE_RADIO_MENU_FLYOUT_ITEM_ENABLED
                {"RadioMenuFlyoutItem", typeof(RadioMenuFlyoutItemPage)},
#endif

#if FEATURE_TABVIEW_ENABLED
                {"TabView", typeof(TabViewPage)},
#endif

#if FEATURE_FLIPVIEW_ENABLED
                {"FlipView", typeof(FlipViewPage), "FlipView.png"},
#endif

#if FEATURE_COMBOBOX_ENABLED
                {"ComboBox", typeof(ComboBoxPage), "ComboBox.png"},
#endif

#if FEATURE_PIVOT_ENABLED
                {"Pivot", typeof(PivotPage), "Pivot.png"},
#endif

#if FEATURE_SCROLL_BAR_ENABLED
                {"ScrollBar", typeof(ScrollBarPage)},
#endif


                // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
                // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
                // to non-public types.  So for now we'll just exclude these from the OS repo.
#if FEATURE_SCROLLVIEWER_ENABLED
                {"ScrollViewer", typeof(ScrollViewerPage), "ScrollViewer.png"},
#endif

#if FEATURE_INTERACTIONS_ENABLED && USE_INTERNAL_SDK
                {"ButtonInteraction", typeof(ButtonInteractionPage), "Button.png"},
                {"SliderInteraction", typeof(SliderInteractionPage), "Slider.png"},
#endif

#if FEATURE_ANIMATED_VISUAL_PLAYER_ENABLED
                {"AnimatedVisualPlayer", typeof(AnimatedVisualPlayerPage), "Animations.png"},
#endif

#if FEATURE_NAVIGATION_VIEW_ENABLED
                {"NavigationView", typeof(NavigationViewCaseBundle), "NavigationView.png"},
#endif

#if FEATURE_PARALLAX_VIEW_ENABLED
                {"ParallaxView", typeof(ParallaxViewPage), "ParallaxView.png"},
#endif

#if FEATURE_MATERIALS_ENABLED
                {"Acrylic", typeof(AcrylicPage), "AcrylicBrush.png"},
                {"Reveal", typeof(RevealPage), "Reveal.png"},
#endif

#if FEATURE_REPEATER_ENABLED
                {"ItemsRepeater", typeof(RepeaterTestUIPage), "ListView.png"},
#endif

#if FEATURE_SCROLLER_ENABLED
                {"Scroller", typeof(ScrollerPage), "ScrollViewer.png"},
#endif

#if FEATURE_AUTOSUGGESTBOX_ENABLED
                {"AutoSuggestBox", typeof(AutoSuggestBoxPage), "AutoSuggestBox.png"},
#endif

#if FEATURE_CHECKBOX_ENABLED
                {"CheckBox", typeof(CheckBoxPage), "CheckBox.png"},
#endif

#if FEATURE_CALENDAR_DATE_PICKER_ENABLED
                {"CalendarDatePicker", typeof(CalendarDatePickerPage), "CalendarDatePicker.png"},
#endif

#if FEATURE_DATE_PICKER_ENABLED
                {"DatePicker", typeof(DatePickerPage), "DatePicker.png"},
#endif

#if FEATURE_SLIDER_ENABLED
                {"Slider", typeof(SliderPage), "Slider.png"},
#endif

#if FEATURE_TIME_PICKER_ENABLED
                {"TimePicker", typeof(TimePickerPage), "TimePicker.png"},
#endif

#if FEATURE_TOOL_TIP_ENABLED
                {"ToolTip", typeof(ToolTipPage), "ToolTip.png"},
#endif
            };

            Tests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}


