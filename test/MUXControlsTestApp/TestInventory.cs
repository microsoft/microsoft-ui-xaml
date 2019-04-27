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
            Tests.Add(new TestDeclaration("ColorPicker", typeof(ColorPickerPage), "ColorPicker.png"));
            Tests.Add(new TestDeclaration("Leak", typeof(LeakTestPage)));
            Tests.Add(new TestDeclaration("PersonPicture", typeof(PersonPicturePage), "PersonPicture.png"));
            Tests.Add(new TestDeclaration("RatingControl", typeof(RatingControlPage), "RatingControl.png"));
            Tests.Add(new TestDeclaration("SwipeControl", typeof(SwipePage), "Swipe.png"));
            Tests.Add(new TestDeclaration("TreeView", typeof(TreeViewPage), "TreeView.png"));
            Tests.Add(new TestDeclaration("TeachingTip", typeof(TeachingTipPage), "TeachingTip.png"));
            Tests.Add(new TestDeclaration("TwoPaneView", typeof(TwoPaneViewPage)));
            Tests.Add(new TestDeclaration("PTR", typeof(PTRPage), "PullToRefresh.png"));
            Tests.Add(new TestDeclaration("MenuBar", typeof(MenuBarPage), "MenuBar.png"));
            Tests.Add(new TestDeclaration("MenuFlyout", typeof(MenuFlyoutPage), "MenuFlyout.png"));
            Tests.Add(new TestDeclaration("ScrollViewerAdapter", typeof(ScrollViewerAdapterPage), "ScrollViewer.png"));
            Tests.Add(new TestDeclaration("SplitButton", typeof(SplitButtonPage), "SplitButton.png"));
            Tests.Add(new TestDeclaration("DropDownButton", typeof(DropDownButtonPage), "DropdownButton.png"));
            Tests.Add(new TestDeclaration("CommandBarFlyout", typeof(CommandBarFlyoutMainPage), "CommandBarFlyout.png"));
            Tests.Add(new TestDeclaration("CommonStyles", typeof(CommonStylesPage)));
            Tests.Add(new TestDeclaration("Compact", typeof(CompactPage), "CompactSizing.png"));
            Tests.Add(new TestDeclaration("RadioButtons", typeof(RadioButtonsPage), "RadioButton.png"));
            Tests.Add(new TestDeclaration("RadioMenuFlyoutItem", typeof(RadioMenuFlyoutItemPage)));
#endif

            // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
            // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
            // to non-public types.  So for now we'll just exclude these from the OS repo.
#if (!BUILD_WINDOWS && !BUILD_LEAN_MUX_FOR_THE_STORE_APP)
            Tests.Add(new TestDeclaration("ScrollViewer", typeof(ScrollViewerPage), "ScrollViewer.png"));
#if (USE_INTERNAL_SDK)
            Tests.Add(new TestDeclaration("ButtonInteraction", typeof(ButtonInteractionPage), "Button.png"));
            Tests.Add(new TestDeclaration("SliderInteraction", typeof(SliderInteractionPage), "Slider.png"));
#endif
#if (USE_INSIDER_SDK)
            Tests.Add(new TestDeclaration("AnimatedVisualPlayer", typeof(AnimatedVisualPlayerPage), "Animations.png"));
#endif
#endif
            Tests.Add(new TestDeclaration("NavigationView", typeof(NavigationViewCaseBundle), "NavigationView.png"));
            Tests.Add(new TestDeclaration("ParallaxView", typeof(ParallaxViewPage), "ParallaxView.png"));
            Tests.Add(new TestDeclaration("Acrylic", typeof(AcrylicPage), "AcrylicBrush.png"));
            Tests.Add(new TestDeclaration("Reveal", typeof(RevealPage), "Reveal.png"));
            Tests.Add(new TestDeclaration("ItemsRepeater", typeof(RepeaterTestUIPage), "ListView.png"));
            Tests.Add(new TestDeclaration("Scroller", typeof(ScrollerPage), "ScrollViewer.png"));

            Tests.Sort((a, b) =>
            {
                return a.Name.CompareTo(b.Name);
            });
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}

