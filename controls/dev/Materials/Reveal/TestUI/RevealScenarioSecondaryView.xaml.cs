// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Windows.UI.ViewManagement;
using Microsoft.UI.Xaml;
using System.Runtime.InteropServices;
using Windows.UI.Core;

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
                Window secondaryWindow = App.CurrentWindow;

                secondaryWindow.Content = frame;

                if (CoreWindow.GetForCurrentThread() != null)
                {
                    try
                    {
                        ApplicationView.GetForCurrentView().Consolidated += (consolidatedSender, args) =>
                        {
                            secondaryWindow.Close();
                        };
                    }
                    catch (COMException)
                    {
                        // ApplicationView.GetForCurrentView() will throw in desktop scenarios.
                    }
                }
            });
        }
    }
}
