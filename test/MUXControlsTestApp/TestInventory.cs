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

            Tests.Add(new TestDeclaration("Sample Tests", typeof(SampleTestUIPage)));
#if !BUILD_LEAN_MUX_FOR_THE_STORE_APP
            Tests.Add(new TestDeclaration("ColorPicker Tests", typeof(ColorPickerPage)));
            Tests.Add(new TestDeclaration("Leak Tests", typeof(LeakTestPage)));
            Tests.Add(new TestDeclaration("PersonPicture Tests", typeof(PersonPicturePage)));
            Tests.Add(new TestDeclaration("RatingControl Tests", typeof(RatingControlPage)));
            Tests.Add(new TestDeclaration("SwipeControl Tests", typeof(SwipePage)));
            Tests.Add(new TestDeclaration("TreeView Tests", typeof(TreeViewPage)));
            Tests.Add(new TestDeclaration("TeachingTip Tests", typeof(TeachingTipPage)));
            Tests.Add(new TestDeclaration("TwoPaneView Tests", typeof(TwoPaneViewPage)));
            Tests.Add(new TestDeclaration("PTR Tests", typeof(PTRPage)));
            Tests.Add(new TestDeclaration("MenuBar Tests", typeof(MenuBarPage)));
            Tests.Add(new TestDeclaration("MenuFlyout Tests", typeof(MenuFlyoutPage)));
            Tests.Add(new TestDeclaration("ScrollViewerAdapter Tests", typeof(ScrollViewerAdapterPage)));
            Tests.Add(new TestDeclaration("SplitButton Tests", typeof(SplitButtonPage)));
            Tests.Add(new TestDeclaration("DropDownButton Tests", typeof(DropDownButtonPage)));
            Tests.Add(new TestDeclaration("CommandBarFlyout Tests", typeof(CommandBarFlyoutMainPage)));
            Tests.Add(new TestDeclaration("CommonStyles Tests", typeof(CommonStylesPage)));
            Tests.Add(new TestDeclaration("RadioButtons Tests", typeof(RadioButtonsPage)));
            Tests.Add(new TestDeclaration("RadioMenuFlyoutItem Tests", typeof(RadioMenuFlyoutItemPage)));
            Tests.Add(new TestDeclaration("RadialGradientBrush Tests", typeof(RadialGradientBrushPage)));
#endif

            // These two depend on the type InteractionBase, which is behind the Velocity feature Feature_Xaml2018 in the OS repo.
            // We can't compile them without attaching the same feature annotation, and MIDL doesn't let us attach feature attributes
            // to non-public types.  So for now we'll just exclude these from the OS repo.
#if (!BUILD_WINDOWS && !BUILD_LEAN_MUX_FOR_THE_STORE_APP)
            Tests.Add(new TestDeclaration("ScrollViewer Tests", typeof(ScrollViewerPage)));
#if (USE_INTERNAL_SDK)
            Tests.Add(new TestDeclaration("ButtonInteraction Tests", typeof(ButtonInteractionPage)));
            Tests.Add(new TestDeclaration("SliderInteraction Tests", typeof(SliderInteractionPage)));
#endif
#if (USE_INSIDER_SDK)
            Tests.Add(new TestDeclaration("AnimatedVisualPlayer Tests", typeof(AnimatedVisualPlayerPage)));
#endif
#endif
            Tests.Add(new TestDeclaration("NavigationView Tests", typeof(NavigationViewCaseBundle)));
            Tests.Add(new TestDeclaration("ParallaxView Tests", typeof(ParallaxViewPage)));
            Tests.Add(new TestDeclaration("Acrylic Tests", typeof(AcrylicPage)));
            Tests.Add(new TestDeclaration("Reveal Tests", typeof(RevealPage)));
            Tests.Add(new TestDeclaration("ItemsRepeater Tests", typeof(RepeaterTestUIPage)));
            Tests.Add(new TestDeclaration("Scroller Tests", typeof(ScrollerPage)));
        }

        public static List<TestDeclaration> Tests { get; private set; }
    }
}


