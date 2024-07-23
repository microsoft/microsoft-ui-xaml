// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

using Microsoft.UI.Xaml;
using Microsoft.UI.Xaml.Controls;

namespace MUXControlsTestApp
{
    [TopLevelTestPage(Name="Window")]
    public sealed partial class WindowPage : TestPage
    {
        public WindowPage()
        {
            this.InitializeComponent();
        }

        private void BtnOpenNewWindowRootPage_Click(object sender, RoutedEventArgs e)
        {
            Window newWindow = new Window() { Title = "MUXControlsTestApp.Desktop - Secondary NewWindowRootPage" };
            TestFrame rootFrame = new TestFrame(typeof(NewWindowRootPage));
            newWindow.Content = rootFrame;
            rootFrame.NavigateWithoutAnimation(typeof(NewWindowRootPage));
            newWindow.Activate();
        }

        private void BtnOpenNewMainPage_Click(object sender, RoutedEventArgs e)
        {
            Window newWindow = new Window() { Title = "MUXControlsTestApp.Desktop - Secondary MainPage" };
            TestFrame rootFrame = new TestFrame(typeof(MainPage));
            newWindow.Content = rootFrame;
            rootFrame.NavigateWithoutAnimation(typeof(MainPage));
            newWindow.Activate();
        }
    }
}
