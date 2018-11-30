// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Windows.UI.ViewManagement;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class RevealScenarioSecondaryView : TestPage
    {
        public RevealScenarioSecondaryView()
        {
            this.InitializeComponent();
        }

        private async void CreateNewView(object sender, RoutedEventArgs e)
        {
            var newView = await ViewHelper.MakeSecondaryViewFromUIThread(() =>
            {
                var frame = new TestFrame(typeof(MainPage));
                frame.NavigateWithoutAnimation(typeof(RevealScenarios));
                Window secondaryWindow = Window.Current;

                secondaryWindow.Content = frame;

                ApplicationView.GetForCurrentView().Consolidated += (consolidatedSender, args) =>
                {
                    secondaryWindow.Close();
                };
            });
        }
    }
}
