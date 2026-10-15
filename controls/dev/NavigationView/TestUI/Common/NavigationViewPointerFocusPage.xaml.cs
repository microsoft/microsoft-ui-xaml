// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    public sealed partial class NavigationViewPointerFocusPage : TestPage
    {
        private int m_pointerPressedCount;

        public NavigationViewPointerFocusPage()
        {
            InitializeComponent();
            TopMode.Checked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Top; };
            TopMode.Unchecked += delegate { NavView.PaneDisplayMode = NavigationViewPaneDisplayMode.Left; };
            // An ordinary subscription must keep receiving navigation item presses.
            NavView.PointerPressed += delegate { PointerPressedCount.Text = (++m_pointerPressedCount).ToString(); };
            CaptureTarget.PointerPressed += (_, args) =>
                CaptureSucceeded.Text = CaptureTarget.CapturePointer(args.Pointer).ToString();
            CaptureTarget.PointerReleased += (_, args) => CaptureTarget.ReleasePointerCapture(args.Pointer);
        }
    }
}
