// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterAccessibilityPage : TestPage
    {
        private IScrollProvider scrollingPresenterAutomationPeer;

        public ScrollingPresenterAccessibilityPage()
        {
            this.InitializeComponent();
        }

        private void BtnSetScrollPercent_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            EnsureAutomationPeer();

            if (scrollingPresenterAutomationPeer != null)
            {
                scrollingPresenterAutomationPeer.SetScrollPercent(
                    Math.Min(100.0, scrollingPresenterAutomationPeer.HorizontalScrollPercent + 10.0),
                    Math.Min(100.0, scrollingPresenterAutomationPeer.VerticalScrollPercent + 15.0));
            }
        }

        private void EnsureAutomationPeer()
        {
            if (scrollingPresenterAutomationPeer == null)
            {
                scrollingPresenterAutomationPeer = FrameworkElementAutomationPeer.CreatePeerForElement(scrollingPresenter) as IScrollProvider;
            }
        }
    }
}
