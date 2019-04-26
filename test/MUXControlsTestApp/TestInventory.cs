// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using System.Collections.Generic;

namespace MUXControlsTestApp
{
    class TestInventory
    {
        static TestInventory()
        {
            Tests = new List<TestDeclaration>();

#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
            Tests.Add(new TestDeclaration("ColorPicker", typeof(ColorPickerPage), "ms-appx:///Assets/ColorPicker.png"));
            Tests.Add(new TestDeclaration("Leak", typeof(LeakTestPage)));
            Tests.Add(new TestDeclaration("PersonPicture", typeof(PersonPicturePage), "ms-appx:///Assets/PersonPicture.png"));
            Tests.Add(new TestDeclaration("RatingControl", typeof(RatingControlPage), "ms-appx:///Assets/RatingControl.png"));
            Tests.Add(new TestDeclaration("SwipeControl", typeof(SwipePage), "ms-appx:///Assets/Swipe.png"));
            Tests.Add(new TestDeclaration("TreeView", typeof(TreeViewPage), "ms-appx:///Assets/TreeView.png"));
            Tests.Add(new TestDeclaration("TeachingTip", typeof(TeachingTipPage), "ms-appx:///Assets/TeachingTip.png"));
            Tests.Add(new TestDeclaration("TwoPaneView", typeof(TwoPaneViewPage)));
            Tests.Add(new TestDeclaration("PTR", typeof(PTRPage), "ms-appx:///Assets/PullToRefresh.png"));
            Tests.Add(new TestDeclaration("MenuBar", typeof(MenuBarPage), "ms-appx:///Assets/MenuBar.png"));
            Tests.Add(new TestDeclaration("MenuFlyout", typeof(MenuFlyoutPage), "ms-appx:///Assets/MenuFlyout.png"));
            Tests.Add(new TestDeclaration("ScrollViewerAdapter", typeof(ScrollViewerAdapterPage), "ms-appx:///Assets/ScrollViewer.png"));
            Tests.Add(new TestDeclaration("SplitButton", typeof(SplitButtonPage), "ms-appx:///Assets/SplitButton.png"));
            Tests.Add(new TestDeclaration("DropDownButton", typeof(DropDownButtonPage), "ms-appx:///Assets/DropdownButton.png"));
            Tests.Add(new TestDeclaration("CommandBarFlyout", typeof(CommandBarFlyoutMainPage), "ms-appx:///Assets/CommandBarFlyout.png"));
            Tests.Add(new TestDeclaration("CommonStyles", typeof(CommonStylesPage)));
            Tests.Add(new TestDeclaration("Compact", typeof(CompactPage), "ms-appx:///Assets/CompactSizing.png"));
            Tests.Add(new TestDeclaration("RadioButtons", typeof(RadioButtonsPage), "ms-appx:///Assets/RadioButton.png"));
            Tests.Add(new TestDeclaration("RadioMenuFlyoutItem", typeof(RadioMenuFlyoutItemPage)));
#endif

            // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
            // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
            // to non-public types.  So for now we'll just exclude these from the OS repo.
#if (!BUILD_WINDOWS && !BUILD_LEAN_MUX_FOR_THE_STORE_APP)
            Tests.Add(new TestDeclaration("ScrollViewer", typeof(ScrollViewerPage), "ms-appx:///Assets/ScrollViewer.png"));
#if (USE_INTERNAL_SDK)
            Tests.Add(new TestDeclaration("ButtonInteraction", typeof(ButtonInteractionPage), "ms-appx:///Assets/Button.png"));
            Tests.Add(new TestDeclaration("SliderInteraction", typeof(SliderInteractionPage), "ms-appx:///Assets/Slider.png"));
#endif
#if (USE_INSIDER_SDK)
            Tests.Add(new TestDeclaration("AnimatedVisualPlayer", typeof(AnimatedVisualPlayerPage), "ms-appx:///Assets/Animations.png"));
#endif
#endif
            Tests.Add(new TestDeclaration("NavigationView", typeof(NavigationViewCaseBundle), "ms-appx:///Assets/NavigationView.png"));
            Tests.Add(new TestDeclaration("ParallaxView", typeof(ParallaxViewPage), "ms-appx:///Assets/ParallaxView.png"));
            Tests.Add(new TestDeclaration("Acrylic", typeof(AcrylicPage), "ms-appx:///Assets/AcrylicBrush.png"));
            Tests.Add(new TestDeclaration("Reveal", typeof(RevealPage), "ms-appx:///Assets/Reveal.png"));
            Tests.Add(new TestDeclaration("ItemsRepeater", typeof(RepeaterTestUIPage), "ms-appx:///Assets/ListView.png"));
            Tests.Add(new TestDeclaration("Scroller", typeof(ScrollerPage), "ms-appx:///Assets/ScrollViewer.png"));
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}

