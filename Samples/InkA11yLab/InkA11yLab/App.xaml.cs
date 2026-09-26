// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

using Microsoft.UI.Xaml;

namespace InkA11yLab
{
    public partial class App : Application
    {
        // The system-composition switcher (CompositionEngine.TrySetProcessEngine) only exists in the
        // ADO lift repo, not here, so this app always runs on the lifted compositor.
        public static string CompositorStatus => "lifted";

        public App()
        {
            this.InitializeComponent();
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            new MainWindow().Activate();
        }
    }
}
