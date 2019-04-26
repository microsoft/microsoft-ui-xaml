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
            Tests.Add(new TestDeclaration("ColorPicker", typeof(ColorPickerPage)));
            Tests.Add(new TestDeclaration("Leak", typeof(LeakTestPage)));
            Tests.Add(new TestDeclaration("PersonPicture", typeof(PersonPicturePage)));
            Tests.Add(new TestDeclaration("RatingControl", typeof(RatingControlPage)));
            Tests.Add(new TestDeclaration("SwipeControl", typeof(SwipePage)));
            Tests.Add(new TestDeclaration("TreeView", typeof(TreeViewPage)));
            Tests.Add(new TestDeclaration("TeachingTip", typeof(TeachingTipPage)));
            Tests.Add(new TestDeclaration("TwoPaneView", typeof(TwoPaneViewPage)));
            Tests.Add(new TestDeclaration("PTR", typeof(PTRPage)));
            Tests.Add(new TestDeclaration("MenuBar", typeof(MenuBarPage)));
            Tests.Add(new TestDeclaration("MenuFlyout", typeof(MenuFlyoutPage)));
            Tests.Add(new TestDeclaration("ScrollViewerAdapter", typeof(ScrollViewerAdapterPage)));
            Tests.Add(new TestDeclaration("SplitButton", typeof(SplitButtonPage)));
            Tests.Add(new TestDeclaration("DropDownButton", typeof(DropDownButtonPage)));
            Tests.Add(new TestDeclaration("CommandBarFlyout", typeof(CommandBarFlyoutMainPage)));
            Tests.Add(new TestDeclaration("CommonStyles", typeof(CommonStylesPage)));
            Tests.Add(new TestDeclaration("Compact", typeof(CompactPage)));
            Tests.Add(new TestDeclaration("RadioButtons", typeof(RadioButtonsPage)));
            Tests.Add(new TestDeclaration("RadioMenuFlyoutItem", typeof(RadioMenuFlyoutItemPage)));
#endif

            // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
            // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
            // to non-public types.  So for now we'll just exclude these from the OS repo.
#if (!BUILD_WINDOWS && !BUILD_LEAN_MUX_FOR_THE_STORE_APP)
            Tests.Add(new TestDeclaration("ScrollViewer", typeof(ScrollViewerPage)));
#if (USE_INTERNAL_SDK)
            Tests.Add(new TestDeclaration("ButtonInteraction", typeof(ButtonInteractionPage)));
            Tests.Add(new TestDeclaration("SliderInteraction", typeof(SliderInteractionPage)));
#endif
#if (USE_INSIDER_SDK)
            Tests.Add(new TestDeclaration("AnimatedVisualPlayer", typeof(AnimatedVisualPlayerPage)));
#endif
#endif
            Tests.Add(new TestDeclaration("NavigationView", typeof(NavigationViewCaseBundle)));
            Tests.Add(new TestDeclaration("ParallaxView", typeof(ParallaxViewPage)));
            Tests.Add(new TestDeclaration("Acrylic", typeof(AcrylicPage)));
            Tests.Add(new TestDeclaration("Reveal", typeof(RevealPage)));
            Tests.Add(new TestDeclaration("ItemsRepeater", typeof(RepeaterTestUIPage)));
            Tests.Add(new TestDeclaration("Scroller", typeof(ScrollerPage)));
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}

