// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using System;
using Windows.UI.Xaml.Automation.Peers;
using Windows.UI.Xaml.Automation.Provider;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollPresenterAccessibilityPage : TestPage
    {
        private IScrollProvider scrollPresenterAutomationPeer;

        public ScrollPresenterAccessibilityPage()
        {
            this.InitializeComponent();
        }

        private void BtnSetScrollPercent_Click(object sender, Windows.UI.Xaml.RoutedEventArgs e)
        {
            EnsureAutomationPeer();

            if (scrollPresenterAutomationPeer != null)
            {
                scrollPresenterAutomationPeer.SetScrollPercent(
                    Math.Min(100.0, scrollPresenterAutomationPeer.HorizontalScrollPercent + 10.0),
                    Math.Min(100.0, scrollPresenterAutomationPeer.VerticalScrollPercent + 15.0));
            }
        }

        private void EnsureAutomationPeer()
        {
            if (scrollPresenterAutomationPeer == null)
            {
                scrollPresenterAutomationPeer = FrameworkElementAutomationPeer.CreatePeerForElement(scrollPresenter) as IScrollProvider;
            }
        }
    }
}
