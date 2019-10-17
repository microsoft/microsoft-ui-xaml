// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using MUXControlsTestApp.Utilities;
using Windows.UI.Xaml;

namespace MUXControlsTestApp
{
    public sealed partial class ScrollingPresenterWithBiDirectionalScrollControllerPage : TestPage
    {
        public ScrollingPresenterWithBiDirectionalScrollControllerPage()
        {
            this.InitializeComponent();
            biDirectionalScrollController.LogMessage += BiDirectionalScrollController_LogMessage;
        }

        private void BtnSetScrollingPresenterScrollControllers_Click(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.HorizontalScrollController = biDirectionalScrollController.HorizontalScrollController;
            scrollingPresenter.VerticalScrollController = biDirectionalScrollController.VerticalScrollController;
        }

        private void BtnResetScrollingPresenterScrollControllers_Click(object sender, RoutedEventArgs e)
        {
            scrollingPresenter.HorizontalScrollController = null;
            scrollingPresenter.VerticalScrollController = null;
        }

        private void ChkIsBiDirectionalScrollControllerRailing_Checked(object sender, RoutedEventArgs e)
        {
            biDirectionalScrollController.IsRailing = true;
        }

        private void ChkIsBiDirectionalScrollControllerRailing_Unchecked(object sender, RoutedEventArgs e)
        {
            biDirectionalScrollController.IsRailing = false;
        }

        private void BiDirectionalScrollController_LogMessage(BiDirectionalScrollController sender, string args)
        {
            LogMessage(args);
        }

        private void LogMessage(string message)
        {
            if (chkLog.IsChecked.Value)
            {
                lstLog.Items.Add(message);
            }
        }

        private void BtnClearLog_Click(object sender, RoutedEventArgs e)
        {
            lstLog.Items.Clear();
        }
    }
}
