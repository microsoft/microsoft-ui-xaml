// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollerAccessibilityPage : TestPage
    {
        private IScrollProvider scrollerAutomationPeer;

        public ScrollerAccessibilityPage()
        {
            this.InitializeComponent();
        }

        private void BtnSetScrollPercent_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            EnsureAutomationPeer();

            if (scrollerAutomationPeer != null)
            {
                scrollerAutomationPeer.SetScrollPercent(
                    Math.Min(100.0, scrollerAutomationPeer.HorizontalScrollPercent + 10.0),
                    Math.Min(100.0, scrollerAutomationPeer.VerticalScrollPercent + 15.0));
            }
        }

        private void EnsureAutomationPeer()
        {
            if (scrollerAutomationPeer == null)
            {
                scrollerAutomationPeer = FrameworkElementAutomationPeer.CreatePeerForElement(scroller) as IScrollProvider;
            }
        }
    }
}
