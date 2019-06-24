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
#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
                {"ColorPicker", typeof(ColorPickerPage), "ColorPicker.png"},
                {"Leak", typeof(LeakTestPage)},
                {"PersonPicture", typeof(PersonPicturePage), "PersonPicture.png"},
                {"RatingControl", typeof(RatingControlPage), "RatingControl.png"},
                {"SwipeControl", typeof(SwipePage), "Swipe.png"},
                {"TreeView", typeof(TreeViewPage), "TreeView.png"},
                {"TeachingTip", typeof(TeachingTipPage), "TeachingTip.png"},
                {"TwoPaneView", typeof(TwoPaneViewPage)},
                {"PTR", typeof(PTRPage), "PullToRefresh.png"},
                {"MenuBar", typeof(MenuBarPage), "MenuBar.png"},
                {"MenuFlyout", typeof(MenuFlyoutPage), "MenuFlyout.png"},
                {"ScrollViewerAdapter", typeof(ScrollViewerAdapterPage), "ScrollViewer.png"},
                {"SplitButton", typeof(SplitButtonPage), "SplitButton.png"},
                {"DropDownButton", typeof(DropDownButtonPage), "DropdownButton.png"},
                {"CommandBarFlyout", typeof(CommandBarFlyoutMainPage), "CommandBarFlyout.png"},
                {"CommonStyles", typeof(CommonStylesPage)},
                {"Compact", typeof(CompactPage), "CompactSizing.png"},
                {"RadioButtons", typeof(RadioButtonsPage), "RadioButton.png"},
                {"RadioMenuFlyoutItem", typeof(RadioMenuFlyoutItemPage)},
                {"TabView", typeof(TabViewPage)},
                {"FlipView", typeof(FlipViewPage)},
                {"ComboBox", typeof(ComboBoxPage)},
                {"Pivot", typeof(PivotPage)},
                {"ScrollBar", typeof(ScrollBarPage)},
#endif

                // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
                // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
                // to non-public types.  So for now we'll just exclude these from the OS repo.
#if (!BUILD_WINDOWS && !BUILD_LEAN_MUX_FOR_THE_STORE_APP)
                {"ScrollViewer", typeof(ScrollViewerPage), "ScrollViewer.png"},
#if (USE_INTERNAL_SDK)
                {"ButtonInteraction", typeof(ButtonInteractionPage), "Button.png"},
                {"SliderInteraction", typeof(SliderInteractionPage), "Slider.png"},
#endif
                {"AnimatedVisualPlayer", typeof(AnimatedVisualPlayerPage), "Animations.png"},
#endif
                {"NavigationView", typeof(NavigationViewCaseBundle), "NavigationView.png"},
                {"ParallaxView", typeof(ParallaxViewPage), "ParallaxView.png"},
                {"Acrylic", typeof(AcrylicPage), "AcrylicBrush.png"},
                {"Reveal", typeof(RevealPage), "Reveal.png"},
                {"ItemsRepeater", typeof(RepeaterTestUIPage), "ListView.png"},
                {"Scroller", typeof(ScrollerPage), "ScrollViewer.png"},
                {"CornerRadius", typeof(CornerRadiusPage)},
                {"AutoSuggestBox", typeof(AutoSuggestBoxPage), "AutoSuggestBox.png"},
                {"CheckBox", typeof(CheckBoxPage), "CheckBox.png"},
                {"CalendarDatePicker", typeof(CalendarDatePickerPage), "CalendarDatePicker.png"},
                {"DatePicker", typeof(DatePickerPage), "DatePicker.png"},
                {"Slider", typeof(SliderPage), "Slider.png"},
                {"TimePicker", typeof(TimePickerPage), "TimePicker.png"},
                {"ToolTip", typeof(ToolTipPage), "ToolTip.png"},
            };

            Tests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}


