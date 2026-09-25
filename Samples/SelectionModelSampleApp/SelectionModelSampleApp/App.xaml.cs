// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
using Microsoft.UI.Xaml;

namespace SelectionModelSampleApp
{
    public partial class App : Application
    {
        public App()
        {
            this.InitializeComponent();
            this.UnhandledException += OnUnhandledException;
        }

        private void OnUnhandledException(object sender, Microsoft.UI.Xaml.UnhandledExceptionEventArgs e)
        {
            if (!string.IsNullOrEmpty(MainWindow.CapturePath))
            {
                System.IO.File.WriteAllText(MainWindow.CapturePath + ".error.log", e.Exception?.ToString() ?? e.Message);
            }
        }

        protected override void OnLaunched(LaunchActivatedEventArgs args)
        {
            ParseScenarioArgument();

            m_window = new MainWindow();
            m_window.Activate();
        }

        private static void ParseScenarioArgument()
        {
            foreach (var arg in System.Environment.GetCommandLineArgs())
            {
                if (arg.StartsWith("out=", System.StringComparison.OrdinalIgnoreCase))
                {
                    MainWindow.CapturePath = arg.Substring("out=".Length);
                    continue;
                }

                int separator = arg.IndexOf(':');
                if (separator > 0 && !arg.Contains('\\') && !arg.Contains('/'))
                {
                    MainWindow.StartupTag = arg.Substring(0, separator);
                    MainWindow.StartupScenario = arg.Substring(separator + 1);
                }
            }
        }

        private Window m_window;
    }
}
